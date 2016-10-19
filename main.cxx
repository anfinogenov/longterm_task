#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cctype>
#include <pthread.h>

const char	playerChar	= 'A';
const char	obstacleChar	= '#';
const char 	wallChar	= 'X';
const int	wallsWidth	= 49; // only mod2 = 1
const long int	startTime	= time(NULL);
const int	fps		= 20;
const int	minLines	= 30; // exit if less or equal
const int	dif_modifier	= 20; // 1 obstacle in (dif_modifier/difficulty) lines
const int	counterFirstLn  = 3;

static std::ofstream fout;
static int global_player_x = 10;
static int global_player_y = 10;
static int difficulty = 5;
static int counter = 0;
static int points = 0;
static bool exitFlag = false;

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
////// end of vars
	
	pthread_t move_thread;
	pthread_create(&move_thread, NULL, multithread_movement, NULL);

	while (!exitFlag) {
		if (!counter % 20) { clear(); refresh(); }
                points = counter*difficulty*difficulty/dif_modifier;
		mvaddch(local_player_x, local_player_y, ' ');
		printWalls(); //adds walls to refresh buffer
		refresh(); //puts obstacles on screen

		checkPlayer(local_player_x, local_player_y);
		if (local_player_x != global_player_x || local_player_y != global_player_y) {	
			local_player_x = global_player_x;
			local_player_y = global_player_y;
		}
                mvaddch(local_player_x, local_player_y, playerChar | A_BOLD);
		refresh(); //put changes on screen

		napms(1000/fps); //make moves fps times per second
		flushinp(); //remove any unattended input
	}
	pthread_cancel(move_thread);
        if(scoreAndExit()) { endwin(); system ("./dd3o.exe"); }
	endwin(); //closes curses screen
        logCounter();
	logMessage (fout, "program exited normally", 'n');
	fout.close();

	return 0;
}

void* multithread_movement (void* arg) {
	int key;
        while (key = getch()) {
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
                movePlayer (key);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		if (tolower(key) =='q') exitFlag = true;
		flushinp(); // removes any unattended input
		napms(1000/fps);
	}
	return NULL;
}

void checkPlayer (int & local_x, int & local_y) {
	move (local_x, local_y);	
	if (inch() != ' ') global_player_x++;
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
        mvprintw(counterFirstLn+1, leftWall - 13, "%d", points);
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
	if(inch() != ' ') return 1;
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
        if (COLS < (wallsWidth+15) || LINES < minLines) {
		logMessage (fout, "incorrect screen size", 'e');
		endwin();
		exit (4);
	}
	logMessage (fout, "screen size is correct", 'n');
}

void logCounter () {
        char count[20] = {0};
        sprintf(count, "%s %d", "counter is", counter);
        logMessage (fout, count, 'n');
}

int scoreAndExit() {
        char exitkey;
        mvprintw (LINES/2 - 1, COLS/2 - 17, "   Game over: Your score is %d   ", points);
        mvprintw (LINES/2    , COLS/2 - 11, "   Press 'q' to exit   ");
        mvprintw (LINES/2 + 1, COLS/2 - 15, "   or press 'r' to restart   ");
        while (tolower(exitkey = getch()) != 'q') if(exitkey == 'r') return 1; //awaits for 'q' to  exit
        return 0;
}
