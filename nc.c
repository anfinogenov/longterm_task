#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
const char outChar = 'A';
const int wallsWidth = 10;

int movePlayer (int key, int *x, int *y);
void printWalls (void);

int main (int argc, char* argv[]) {
	const long int startTime = time(NULL);

	FILE* fout;
	if((fout = fopen("log.txt", "w")) == NULL) {
		fprintf(stderr, "cannot open log file\n");
		exit(1);
	}

	if(!initscr()) {
		fprintf(fout, "initscr() failure\n");
		exit(2);
	}
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	int x, y, key;
	x = LINES/2; y = COLS/2;
	while ((key = getch()) != 'q') {
		clear();
		printWalls();
//		mvaddch(x, y, ' ');
		if (movePlayer(key, &x, &y)) fprintf(fout, "[%ld] pressed non-arrow button\n", time(NULL) - startTime);
		mvaddch(x, y, outChar);
		refresh();
	}
	endwin();

	fprintf(fout, "[%ld] program exited normally\n", time(NULL) - startTime);
	fclose(fout);

	return 0;
}

int movePlayer (int key, int *x, int *y) {
	int temp;
	if (key == KEY_DOWN) { temp = (*x + 1) % LINES; if (1);}
	else if (key == KEY_UP) { *x = (*x + LINES - 1) % LINES; }
	else if (key == KEY_RIGHT) { *y = (*y + 1) % COLS; }
	else if (key == KEY_LEFT) { *y = (*y + COLS - 1) % COLS; }
	else return 1;
	return 0;
}

void printWalls () {
	for (int i = 0; i < LINES; i++) {
		mvaddch(i, (COLS/2) - wallsWidth, 'B');
		mvaddch(i, (COLS/2) + wallsWidth, 'B');
	}
}
