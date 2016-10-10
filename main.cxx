#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cctype>
#include <pthread.h>

const char	playerChar	= 'A';
const char	wallChar	= 'x';
const int	wallsWidth	= 19; //only mod2 = 1
const long int	startTime	= time(NULL);
const int	fps		= 10;

static int player_x = 10;
static int player_y = 10;

void*	multithread_movement (void* arg); 
int 	movePlayer (int key);
void	logMessage (std::ofstream & fout, const std::string & msg, char msgType);
void	printWalls (void);
int	isWall (const int &x, const int &y);

int main (int argc, char* argv[]) {

	srand(time(NULL));
	std::ofstream fout("log.txt");
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

	player_x = 2*LINES/3; 
	player_y = COLS/2;

	pthread_t move_thread;
	pthread_create(&move_thread, NULL, multithread_movement, NULL);

	while (true) {
		clear(); //clears screen before next refresh()
		printWalls(); //adds walls to refresh buffer
		mvaddch(player_x, player_y, playerChar);

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
		key = getch();
		napms(1000/fps);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		movePlayer (key);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		flushinp(); // removes any unattended input
	}
	return NULL;
}

int movePlayer (int key) {
	int new_coord; //temp variable to store coordinate where player wants to move in
	switch (key) {
		case KEY_DOWN:
			new_coord = (player_x+1) % LINES;
			if (!isWall(new_coord, player_y)) player_x = new_coord;
			break;
		case KEY_UP:
			new_coord = (player_x-1) % LINES;
			if (!isWall(new_coord, player_y)) player_x = new_coord;
			break;
		case KEY_RIGHT:
			new_coord = (player_y+1) % COLS;
			if (!isWall(player_x, new_coord)) player_y = new_coord;
			break;
		case KEY_LEFT:
			new_coord = (player_y-1) % COLS;
			if (!isWall(player_x, new_coord)) player_y = new_coord;
			break;
		default: break;
	}
}

void printWalls () {
	border (wallChar, wallChar, wallChar, wallChar, 'x', 'x', 'x', 'x');
	for (int i = 0; i < LINES; i++) {
		mvaddch(i, (COLS/2) - wallsWidth/2 - 1, wallChar);
		mvaddch(i, (COLS/2) + wallsWidth/2 + 1, wallChar);
	}

	//for (int i = 0; i < LINES; i++)
	//	for (int j = 0; j < COLS; j++) if (rand()%10 == 1) mvaddch(i, j, wallChar);
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
