#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <cctype>
#include <pthread.h>
#include <unistd.h>

const char	playerChar	= 'A';
const char	obstacleChar	= '#';
const char 	wallChar	= 'X';
const int	wallsWidth	= 49; // only mod2 = 1
const long int	startTime	= time(NULL);
const int	minLines	= 35; // exit if less or equal
const int	dif_modifier	= 20; // 1 obstacle in (dif_modifier/difficulty) lines
const int	counterFirstLn  = 3;

static int fps = 15;
static std::ofstream fout;
static int global_player_x = 10;
static int global_player_y = 10;
static int difficulty = 0;
static int counter = 0;
static float points = 0;
static bool exitFlag = false;
static bool pauseFlag = false;

static int leftWall;
static int rightWall;

void*	multithread_movement (void* arg); 
int 	movePlayer (const int & key);
void	checkPlayer (int & local_x, int & local_y);
void	logMessage (std::ofstream & fout, const std::string & msg, char msgType);
void	printWalls (void);
void    logCounter (void);
void	generateNewLine (void);
int	isWall (const int &x, const int &y);
void	checkScreen (void);
void    insertCounter (void);
int     scoreAndExit (void);
void    checkPause (void);

namespace bonus {
    static int rate = 13;
    const int rateModifier = 1;

    const char slowdownChar = 'S';
    const int slowFPS = 5;
    const int slowTimeSeconds = 10; //time of slowing down in seconds
    const int slowTimeLines = 30; //time of slowing down in generated lines
    void slowdown (void);

    const char shootChar = 'F';
    void shoot (void);

    const char moveThroughChar = 'M';
    void moveThrough (void);

    const char hpUpChar = 'U';
    const int hpUpBonus = 5;
    void hpUp (void);

    const char hpDownChar = 'D';
    const int hpDownBonus = 5;
    void hpDown (void);

    const char teleportChar = 'T';
    void teleport (void);

    void generateBonus (void);
}

int main () {

////// initialization routines	
	srand(time(NULL));

	fout.open("log.txt");
	if(!fout.is_open()) {
		std::cerr << "cannot open log file\n";
		exit(1);
	}
	if(!initscr()) {
		logMessage (fout, "initscr() failed", 'e');
		exit(2);
	}
	logMessage (fout, "initscr executed normally", 'n');
	noecho(); curs_set(0); cbreak();
	keypad(stdscr, TRUE);
        checkScreen();
        if(!has_colors()) {
          endwin();
          logMessage(fout, "colors isn't allowed", 'e');
          exit(5);
        }
        start_color();
////// end of routines

////// variable initialization
        leftWall = (COLS/2) - wallsWidth/2 - 1;
        rightWall = (COLS/2) + wallsWidth/2 + 1;
        global_player_x = LINES - 16;
	global_player_y = COLS/2;
	int local_player_x = global_player_x;
	int local_player_y = global_player_y;
        init_pair(1, COLOR_YELLOW, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_BLUE, COLOR_BLACK);
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(5, COLOR_GREEN, COLOR_BLACK);
////// end of vars
	
	pthread_t move_thread;
	pthread_create(&move_thread, NULL, multithread_movement, NULL);

        clear(); refresh();

        while (!exitFlag) {
                checkPause();
                if (!pauseFlag) {
                        if (!(counter % 300)) { difficulty++; bonus::rate += bonus::rateModifier; }
                        points += (float)(difficulty*difficulty)/dif_modifier;
                        mvaddch(local_player_x, local_player_y, ' ');
                        printWalls(); //adds walls to refresh buffer
                        refresh(); //puts obstacles on screen
                        checkPlayer(local_player_x, local_player_y);
                        if (local_player_x != global_player_x || local_player_y != global_player_y) {
                                local_player_x = global_player_x;
                                local_player_y = global_player_y;
                        }
                        mvaddch(local_player_x, local_player_y, playerChar | A_BOLD);
                }
                refresh(); //put changes on screen
		napms(1000/fps); //make moves fps times per second
		flushinp(); //remove any unattended input
	}
	pthread_cancel(move_thread);
        if(scoreAndExit()) {
            endwin();
            logCounter();
            fout.close();
            system("./dd3o.exe");
        }
	endwin(); //closes curses screen
        logCounter();
	logMessage (fout, "program exited normally", 'n');
	fout.close();
        printf("\x1b[0m");

	return 0;
}

void* multithread_movement (void* arg) {
	int key;
        while (key = getch()) {
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
                if (!pauseFlag) movePlayer (key);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		if (tolower(key) =='q') exitFlag = true;
                if (tolower(key) == ' ') pauseFlag = !pauseFlag;
		flushinp(); // removes any unattended input
		napms(1000/fps);
	}
	return NULL;
}

void checkPlayer (int & local_x, int & local_y) {
        move (local_x, local_y);
        char checkedPoint = inch();
        if (checkedPoint != ' ')
                switch (checkedPoint) {
                case bonus::hpUpChar:
                        addch(' ');
                        bonus::hpUp();
                        break;
                case bonus::hpDownChar:
                        addch(' ');
                        bonus::hpDown();
                        break;
                case bonus::shootChar:
                        addch(' ');
                        bonus::shoot();
                        break;
                case obstacleChar:
                        global_player_x++;
                        break;
                default:
                        break;
                }

        if (global_player_x >= LINES - 1) { //global
                logMessage (fout, "player fell out", 'n');
		exitFlag = true;
	}
}

int movePlayer (const int & key) {
	int new_coord; //temp variable to store coordinate where player wants to move in
        switch (key) {
		case KEY_RIGHT:
			new_coord = global_player_y + 1;
			if (!isWall(global_player_x, new_coord)) global_player_y = new_coord;
			break;
		case KEY_LEFT:
			new_coord = global_player_y - 1;
			if (!isWall(global_player_x, new_coord)) global_player_y = new_coord;
			break;
		default: break;
	}
}

void printWalls () {
	generateNewLine();
	for (int i = 0; i < LINES; i++) {
                mvaddch(i, leftWall, wallChar | COLOR_PAIR(1));
                mvaddch(i, rightWall, wallChar | COLOR_PAIR(1));
	}
        for (int i = 1; i <= wallsWidth; i++)
                mvaddch(LINES-1,i+leftWall, wallChar | COLOR_PAIR(2) | A_DIM);
}

void generateNewLine () {
        move(0, 0); insertln();
        insertCounter();
        if (!(counter++ % (dif_modifier/difficulty))) {
                int len = rand()%wallsWidth/4;
                int start = rand()%wallsWidth + leftWall;
                for (int i = 0; i < len; i++) {
                        if (start + i < rightWall) mvaddch(0, start + i, obstacleChar);
                        if (start - i > leftWall) mvaddch(0, start - i, obstacleChar);
                }
        }
        else if (!(counter % bonus::rate)) bonus::generateBonus();
}

void insertCounter () {
        int secondLn = counterFirstLn+1;
        for (int i = 0; i < leftWall; i++) {
            mvaddch(secondLn, i, ' ');
            mvaddch(secondLn+1, i, ' ');
            mvaddch(secondLn+3, i, ' ');
            mvaddch(secondLn+4, i, ' ');
            mvaddch(secondLn+6, i, ' ');
            mvaddch(secondLn+7, i, ' ');
            mvaddch(secondLn+9, i, ' ');
            mvaddch(secondLn+10, i, ' ');
        }
        mvprintw(counterFirstLn, leftWall - 13, "Points:");
        mvprintw(counterFirstLn+1, leftWall - 13, "%d", (int)points);
        mvprintw(counterFirstLn+3, leftWall - 13, "Time:");
        mvprintw(counterFirstLn+4, leftWall - 13, "%d", time(NULL) - startTime);
        mvprintw(counterFirstLn+6, leftWall - 13, "Difficulty:");
        mvprintw(counterFirstLn+7, leftWall - 13, "%d", difficulty);
        mvprintw(counterFirstLn+9, leftWall - 13, "Lives:");
        mvprintw(counterFirstLn+10, leftWall - 13, "%d", LINES - global_player_x - 1);

        return;
}

int isWall (const int &x, const int &y) {
        move(x, y);
        char checkedPoint = inch();
        if(checkedPoint == wallChar || checkedPoint == obstacleChar) return 1;
        return 0;
}

void logMessage (std::ofstream & fout, const std::string & msg, char msgType) {
	std::string type;
	switch (msgType) { //converting char msgType to string
		case 'n': type = "okay"; break;
		case 'w': type = "warn"; break;
		case 'e': type = "err "; break;
		default : type = "unkn"; break;
	}
	fout << type + " [" << time(NULL) - startTime << "]: " << msg << std::endl;
	//out message with msgType and time
}

void checkScreen () {
        if (COLS < (wallsWidth+30) || LINES < minLines) {
		logMessage (fout, "incorrect screen size", 'e');
		endwin();
		exit (4);
	}
	logMessage (fout, "screen size is correct", 'n');
}

void logCounter () {
        char count[20] = {0};
        sprintf(count, "counter is %d", counter);
        logMessage (fout, count, 'n');
}

int scoreAndExit() {
        char exitkey;
        mvprintw (LINES/2 - 1, COLS/2 - 17, "   Game over: Your score is %d   ", (int)points);
        mvprintw (LINES/2    , COLS/2 - 11, "   Press 'q' to exit   ");
        mvprintw (LINES/2 + 1, COLS/2 - 15, "   or press 'r' to restart   ");
        while (tolower(exitkey = getch()) != 'q') if (exitkey == 'r') return 1; //awaits for 'q' to  exit
        return 0;
}

void bonus::slowdown () {
    const int oldFps = fps;
    fps = slowFPS;
    const int counterStart = counter;
    long int timeStart = time(NULL);
    while (true)
        if ((time(NULL) - timeStart) > slowTimeSeconds || (counter - counterStart) > slowTimeLines)
            return;
}

void bonus::shoot () {
    for (int i = global_player_x-1; i > 0; i--) {
        move (i, global_player_y);
        if(inch() == obstacleChar)
            addch (' ');
        move (i, global_player_y-1);
        if(inch() == obstacleChar)
            addch (' ');
        move (i, global_player_y+1);
        if(inch() == obstacleChar)
            addch (' ');
    }
}

void bonus::moveThrough () {
}

void bonus::hpUp () {
    global_player_x -= hpUpBonus;
}

void bonus::hpDown () {
    global_player_x += hpDownBonus;
}

void bonus::generateBonus () {
    switch (rand()%5) {
    case 0:
        //mvaddch(0, leftWall + rand()%wallsWidth, bonus::slowdownChar | COLOR_PAIR(3));
        break;
    case 1:
        mvaddch(0, leftWall + rand()%wallsWidth, bonus::shootChar | COLOR_PAIR(4));
        break;
    case 2:
        //mvaddch(0, leftWall + rand()%wallsWidth, bonus::moveThroughChar | COLOR_PAIR(1));
        break;
    case 3:
        mvaddch(0, leftWall + rand()%wallsWidth, bonus::hpUpChar | COLOR_PAIR(5));
        break;
    case 4:
        mvaddch(0, leftWall + rand()%wallsWidth, bonus::hpDownChar | COLOR_PAIR(2));
        break;
    default:
        break;
    }
}

void checkPause () {
    if (!pauseFlag)
        for (int i = 0; i < leftWall; i++) {
            mvprintw (LINES/2-1, leftWall - 13, "Space   ");
            mvprintw (LINES/2,   leftWall - 13, "to pause");
            mvprintw (LINES/2+1, leftWall - 13, "        ");
        }
    else {
        mvprintw (LINES/2,   leftWall - 13, "Paused      ");
        mvprintw (LINES/2+1, leftWall - 13, "            ");
    }
    refresh();
}
