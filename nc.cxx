#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>

const char playerChar = 'A';
const char wallChar = 'X';
const int wallsWidth = 10;
const long int startTime = time(NULL);

int 	movePlayer (int key, int &x, int &y);
void 	printWalls (void);
int 	isWall (const int &x, const int &y);
void	logMessage (std::ofstream & fout, const std::string & msg, char msgType);

int main (int argc, char* argv[]) {

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

	int x, y, key;
	x = 2*LINES/3; y = COLS/2;
	while ((key = getch()) != 'q') {
		clear();
		printWalls();
//		mvaddch(x, y, ' ');
		if (movePlayer(key, x, y)) logMessage (fout, "pressed non-arrow button", 'w');
		mvaddch(x, y, playerChar);
		refresh();
	}
	endwin();
	logMessage (fout, "program exited normally", 'n');
	fout.close();

	return 0;
}

int movePlayer (int key, int &x, int &y) {
	int temp;
	if (key == KEY_DOWN) {
		temp = (x + 1) % LINES;
		if (!isWall(temp, y)) x = temp;
	}
	else if (key == KEY_UP) {
		temp = (x + LINES - 1) % LINES;
		if (!isWall(temp, y)) x = temp;
	}
	else if (key == KEY_RIGHT) {
		temp = (y + 1) % COLS;
		if (!isWall(x, temp)) y = temp;
	}
	else if (key == KEY_LEFT) {
		temp = (y + COLS - 1) % COLS;
		if (!isWall(x, temp)) y = temp;
	}
	else return 1;
	return 0;
}

void printWalls () {
	for (int i = 0; i < LINES; i++) {
		mvaddch(i, (COLS/2) - wallsWidth, wallChar);
		mvaddch(i, (COLS/2) + wallsWidth, wallChar);
	}
}

int isWall (const int &x, const int &y) {
	move(x ,y);
	if(inch() != ' ') return 1;
	return 0;
}

void logMessage (std::ofstream & fout, const std::string & msg, char msgType) {
	std::string type;
	switch (msgType) {
		case 'n': type = "okay"; break;
		case 'w': type = "warn"; break;
		case 'e': type = "err "; break;
		default : type = "unkn"; break;
	}
	fout << type + " [" << time(NULL) - startTime << "]: " << msg << std::endl;
}
