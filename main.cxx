#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cctype>
#include <pthread.h>

const char	playerChar	= 'A';
const char 	wallChar	= '|';
const int	wallsWidth	= 19; //only mod2 = 1
const long int	startTime	= time(NULL);
const int	fps		= 20;
const int	minLines	= 15; //exit if less or equal

std::ofstream fout;

static int global_player_x = 10;
static int global_player_y = 10;

void*	multithread_movement (void* arg); 
int 	movePlayer (const int & key);
void	logMessage (std::ofstream & fout, const std::string & msg, char msgType);
void	printWalls (void);
int	isWall (const int &x, const int &y);
void	checkScreen (void);

int main (int argc, char* argv[]) {
	
	srand(time(NULL));

	fout.open("log.txt");
	if(!fout.is_open()) {
		std::cerr << "cannot open log file\n";
		exit(1);
	}

	if(!initscr()) {
		logMessage (fout, "initscr() failure", 'e');
		exit(2);
	}
	logMessage (fout, "initscr executed normally", 'n');
	noecho(); curs_set(0); cbreak();
	keypad(stdscr, TRUE);

	checkScreen();

	global_player_x = 2*LINES/3; 
	global_player_y = COLS/2;
	int local_player_x = global_player_x;
	int local_player_y = global_player_y;

	pthread_t move_thread;
	pthread_create(&move_thread, NULL, multithread_movement, NULL);

	while (global_player_x > -1) {
		mvaddch(local_player_x, local_player_y, ' ');

		printWalls(); //adds walls to refresh buffer
		
		if (local_player_x != global_player_x || local_player_y != global_player_y) {	
			local_player_x = global_player_x;
			local_player_y = global_player_y;
		}
		
		mvaddch(local_player_x, local_player_y, playerChar);
				
		refresh(); //put changes on screen

		napms(1000/fps); //make moves 30 times per second
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
		napms(1000/fps);
		key = getch();
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		movePlayer (key);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		if (tolower(key) =='q') global_player_x = -1;
		flushinp(); // removes any unattended input
	}
	return NULL;
}

int movePlayer (const int & key) {
	int new_coord; //temp variable to store coordinate where player wants to move in
	switch (key) {
		case KEY_DOWN:
			new_coord = (global_player_x + 1) % LINES;
			if (!isWall(new_coord, global_player_y)) global_player_x = new_coord;
			break;
		case KEY_UP:
			new_coord = (global_player_x + LINES - 1) % LINES;
			if (!isWall(new_coord, global_player_y)) global_player_x = new_coord;
			break;
		case KEY_RIGHT:
			new_coord = (global_player_y + 1) % COLS;
			if (!isWall(global_player_x, new_coord)) global_player_y = new_coord;
			break;
		case KEY_LEFT:
			new_coord = (global_player_y + COLS - 1) % COLS;
			if (!isWall(global_player_x, new_coord)) global_player_y = new_coord;
			break;
		default: break;
	}
}

void printWalls () {
	move(0, 0); insertln();
	for (int i = 0; i < LINES; i++) {
		mvaddch(i, (COLS/2) - wallsWidth/2 - 1, wallChar);
		mvaddch(i, (COLS/2) + wallsWidth/2 + 1, wallChar);
	}
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
