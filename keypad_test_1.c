#include <ncurses.h>
const char out = '*';

int main (int argc, char* argv[]) {
	if(!initscr()) fprintf(stderr, "initscr() failure");
	keypad(stdscr, TRUE);
	int x, y, key;
	x = 0; y = 0;
	while ((key = getch()) != 'q') {
		mvaddch(x, y, ' ');
		if (key == KEY_DOWN) x = (x + 1) % LINES;
		if (key == KEY_UP) x = (x + LINES - 1) % LINES;
		if (key == KEY_RIGHT) y = (y + 1) % COLS;
		if (key == KEY_LEFT) y = (y + COLS - 1) % COLS;
		mvaddch(x, y, out);
		refresh();
	}
	endwin();
	return 0;
}
