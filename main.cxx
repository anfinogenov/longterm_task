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
const char 	wallChar	= '|';
const int	wallsWidth	= 49; //only mod2 = 1
const long int	startTime	= time(NULL);
const int	fps		= 30;
const int	minLines	= 15; //exit if less or equal
const int	dif_modifier	= 10;

static std::ofstream fout;
static int global_player_x = 10;
static int global_player_y = 10;
static int difficulty = 1;
static int current_points;
static int counter = 0;
static bool exitFlag = false;

void*	multithread_movement (void* arg); 
int 	movePlayer (const int & key);
void	checkPlayer (int & local_x, int & local_y);
void	logMessage (std::ofstream & fout, const std::string & msg, char msgType);
void	printWalls (void);
void	generateNewLine (void);
int	isWall (const int &x, const int &y);
void	checkScreen (void);

int main (int argc, char* argv[]) {

////// inititalization routines	
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
////// end of routines

	global_player_x = 2*LINES/3; 
	global_player_y = COLS/2;
	int local_player_x = global_player_x;
	int local_player_y = global_player_y;

	pthread_t move_thread;
	pthread_create(&move_thread, NULL, multithread_movement, NULL);

	while (!exitFlag) {
		
		mvaddch(local_player_x, local_player_y, ' ');
		printWalls(); //adds walls to refresh buffer
		refresh(); //puts obstacles on screen

		checkPlayer(local_player_x, local_player_y);
		if (local_player_x != global_player_x || local_player_y != global_player_y) {	
			local_player_x = global_player_x;
			local_player_y = global_player_y;
		}
		mvaddch(local_player_x, local_player_y, playerChar);
		refresh(); //put changes on screen

		napms(1000/fps); //make moves fps times per second
		flushinp(); //remove any unattended input
	}
	pthread_cancel(move_thread);
	endwin(); //closes curses screen
	logMessage (fout, "program exited normally", 'n');
	fout.close();

	return 0;
}

void* multithread_movement (void* arg) {
	int key;
	while (true) {
		key = getch();
		
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
	if (global_player_x >= LINES) { //global
		logMessage (fout, "player fell out", 'n');
		exitFlag = true;
	}
}

int movePlayer (const int & key) {
	int new_coord; //temp variable to store coordinate where player wants to move in
	switch (key) {
		case KEY_DOWN:
			new_coord = global_player_x + 1;
			if (!isWall(new_coord, global_player_y)) global_player_x = new_coord;
			break;
		case KEY_UP:
			new_coord = global_player_x - 1;
			if (!isWall(new_coord, global_player_y)) global_player_x = new_coord;
			break;
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
		mvaddch(i, (COLS/2) - wallsWidth/2 - 1, wallChar);
		mvaddch(i, (COLS/2) + wallsWidth/2 + 1, wallChar);
	}
}

void generateNewLine () {
	move(0,0); insertln();
	int obstacle[wallsWidth] = {0};
	int len = rand()%wallsWidth/2;
	int start = rand()%wallsWidth + COLS/2 - wallsWidth/2;
	if (!(counter++ % (difficulty * dif_modifier)))
		for (int i = 0; i < len; i++)
			mvaddch(0, start + i, obstacleChar);
}

int isWall (const int &x, const int &y) {
	move(x ,y);
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
	if (COLS < (wallsWidth+5) || LINES < minLines) {
		logMessage (fout, "incorrect screen size", 'e');
		endwin();
		exit (4);
	}
	logMessage (fout, "screen size is correct", 'n');
}
