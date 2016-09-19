#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>

const char outChar = 'A';
const int wallsWidth = 10;

int 	movePlayer (int key, int &x, int &y);
void 	printWalls (void);
int 	isWall (const int &x, const int &y);

int main (int argc, char* argv[]) {
	const long int startTime = time(NULL);

	std::ofstream fout("log.txt");
	if(!fout.is_open()) {
		std::cerr << "cannot open log file\n";
		exit(1);
	}

	if(!initscr()) {
		fout << "initscr() failure\n";
		exit(2);
	}

	noecho(); curs_set(0); cbreak();
	keypad(stdscr, TRUE);

	int x, y, key;
	x = LINES/2; y = COLS/2;
	while ((key = getch()) != 'q') {
		clear();
		printWalls();
//		mvaddch(x, y, ' ');
		if (movePlayer(key, x, y)) fout << '[' << time(NULL) - startTime << "] pressed non-arrow button\n";
		mvaddch(x, y, outChar);
		refresh();
	}
	endwin();

	fout << '[' << time(NULL) - startTime << "] program exited normally\n";
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
		mvaddch(i, (COLS/2) - wallsWidth, 'B');
		mvaddch(i, (COLS/2) + wallsWidth, 'B');
	}
}

int isWall (const int &x, const int &y) {
//	if (mvinch(y, x) != ' ') return 0;
	return 0;
}
