#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>
#include <string>
#include <cctype>
#include <pthread.h>
#include <unistd.h>
#include "bass.h"

const char playerChar     = 'A';
const char obstacleChar   = '#';
const char wallChar       = 'X';
const int  wallsWidth     = 49;
const long startTime      = time(NULL);
const int  minLines       = 35; // exit if less or equal
const int  dif_modifier   = 20; // 1 obstacle in (dif_modifier/difficulty) lines
const int  counterFirstLn = 3;
const int  playerLives    = 15;
const int  fps = 15;

namespace color {
    const int yellow = 1;
    const int red = 2;
    const int cyan = 3;
    const int green = 4;
    const int black = 5;
}

static std::ofstream fout;
static int global_player_x = 0;
static int global_player_y = 0;
static int difficulty = 0;
static int counter = 0;
static float points = 0;
static bool exitFlag = false;
static bool pauseFlag = false;

HSTREAM back_stream, game_over_stream, hp_up_stream, hp_down_stream, shoot_stream, lvl_stream;
static int leftWall;
static int rightWall;

void    log_open_s (void);
void    ncurses_init_s (void);
void    variable_init (void);
void    exit_s (char*, char);
void    music_init_s (void);
void    music_gameover (void);

void*   multithread_movement (void*);
int     movePlayer (const int &);
void    checkPlayer (int &, int &);
void    log_out (const std::string &, char);
void    printWalls (void);
void    logCounter (void);
void    generateNewLine (void);
bool    isWall (const int &, const int &);
void    checkScreen (void);
void    insertCounter (void);
int     scoreAndExit (void);
bool    isPause (void);

namespace bonus {
    static int rate = 13;

    const char shootChar = 'S';
    void shoot (void);

    const char hpUpChar = 'U';
    const int hpUpBonus = 5;
    void hpUp (void);

    const char hpDownChar = 'D';
    const int hpDownBonus = 5;
    void hpDown (void);

    void generateBonus (void);
}

int main () {

    srand(time(nullptr));
    log_open_s();
    ncurses_init_s();
    variable_init();
    music_init_s();

    int local_player_x = global_player_x;
    int local_player_y = global_player_y;
    int startLines = LINES;
    int startCols = COLS;

    pthread_t move_thread;
    pthread_create(&move_thread, NULL, multithread_movement, NULL); //starts movePlayer-func in other thread

    clear();
    log_out("main logic starts", 'n');
    while (!exitFlag) {
        if (startLines != LINES || startCols != COLS) exit_s((char*)"changed screen size", 'e');
        if (!isPause()) {
            if (!(counter % 300)) { //if counter%300 == 0 increase difficulty
                clear();
                BASS_ChannelPlay(lvl_stream,TRUE);
                mvprintw(counterFirstLn, rightWall + 3, "Level %d!", ++difficulty);
                char msg[27] = {0};
                sprintf(msg, "reached level %d", difficulty);
                log_out(msg,'n');
            }
            points += (float)(difficulty*difficulty)/dif_modifier;
            //every tick increases points, depending on current difficulty
            mvaddch(local_player_x, local_player_y, ' '); //erases old player
            printWalls(); //adds walls to refresh buffer
            refresh(); //puts buffer on screen
            checkPlayer(local_player_x, local_player_y); //checks if there are any collisions with player
            if (local_player_x != global_player_x || local_player_y != global_player_y) {
                local_player_x = global_player_x;
                local_player_y = global_player_y; //changes player coords if not equal
            }
            mvaddch(local_player_x, local_player_y, playerChar | A_BOLD); //prints new player
        }
        refresh(); //put changes on screen
        napms(1000/fps); //make moves fps times per second
        flushinp(); //remove any unattended input
    }
    log_out("main logic ends", 'n');
    pthread_cancel(move_thread);
    BASS_ChannelStop(back_stream);
    BASS_StreamFree(back_stream);
    music_gameover();
    if(scoreAndExit()) {
        endwin();
        logCounter();
        log_out("", 'q');
        fout.close();
        system("./dd3o.exe");
    }
    exit_s((char*)"program exited normally", 'n');
    return 0;
}

void* multithread_movement (void* arg) {
    int key;
    while (key = getch()) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if (!pauseFlag) movePlayer (key); //disable moving if paused
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        if (tolower(key) =='q') exitFlag = true;
        else if (tolower(key) == ' ') pauseFlag ^= 1; //inverts pauseFlag
        flushinp(); // removes any unattended input
        napms(1000/fps);
    }
    return nullptr;
}

void checkPlayer (int & local_x, int & local_y) {
    move (local_x, local_y);
    char checkedPoint = inch();
    if (checkedPoint != ' ')
        switch (checkedPoint) {
            case bonus::hpUpChar:
                addch(' ');
                bonus::hpUp();
                BASS_ChannelPlay(hp_up_stream,TRUE);
                break;
            case bonus::hpDownChar:
                addch(' ');
                bonus::hpDown();
                BASS_ChannelPlay(hp_down_stream,TRUE);
                break;
            case bonus::shootChar:
                addch(' ');
                bonus::shoot();
                BASS_ChannelPlay(shoot_stream,TRUE);
                break;
            case obstacleChar:
                global_player_x++;
                break;
            default:
                break;
        }
    if (global_player_x >= LINES - 1) { //if player touches red line on bottom sets exitflag and logs msg 
        log_out("player fell out", 'n');
        exitFlag = true;
    }
}

int movePlayer (const int & key) {
    int new_coord; //temp variable to store coordinate where player wants to move in
    switch (key) {
        case KEY_RIGHT:
            new_coord = global_player_y + 1;
            if (!isWall(global_player_x, new_coord)) //checks if there is no wall
                global_player_y = new_coord;
            break;
        case KEY_LEFT:
            new_coord = global_player_y - 1;
            if (!isWall(global_player_x, new_coord))
                global_player_y = new_coord;
            break;
        default: break;
    }
    return 0;
}

void printWalls () {
    generateNewLine();
    for (int i = 0; i < LINES; i++) {
        mvaddch(i, leftWall, wallChar | COLOR_PAIR(color::yellow)); //draws left orange wall
        mvaddch(i, rightWall, wallChar | COLOR_PAIR(color::yellow)); //draws right orange wall
    }
    for (int i = 1; i <= wallsWidth; i++)
        mvaddch(LINES-1,i+leftWall, wallChar | COLOR_PAIR(color::red) | A_DIM); //draws red bottom
}

void generateNewLine () {
    move(0, 0); insertln();  //adds new empty line at the top of the screen
    insertCounter(); //prints counter
    if (dif_modifier == difficulty) { //avoids zero-division on 21 level, just ends the game
        log_out("reached level 21", 'e');
        exitFlag = true;
        return;
    }
    if (!(counter++ % (dif_modifier/difficulty))) { //generate new obstacle 1 time in dif/dif lines
        int len = rand()%wallsWidth/4; //generate len of obstacle
        int start = rand()%wallsWidth + leftWall; //generate starting point inside walls
        for (int i = 0; i < len; i++) { //prints obstacle in two directions
            if (start + i < rightWall) mvaddch(0, start + i, obstacleChar);
            if (start - i > leftWall) mvaddch(0, start - i, obstacleChar);
        }
    }
    else if (!(counter % bonus::rate)) bonus::generateBonus(); //if there is no obstacle on new line, try to generate bonus
}

void insertCounter () {
    int secondLn = counterFirstLn+1;
    for (int i = 0; i < leftWall; i++) { //clears space from left side of the screen to left wall
        mvaddch(secondLn, i, ' ');
        mvaddch(secondLn+1, i, ' ');
        mvaddch(secondLn+3, i, ' ');
        mvaddch(secondLn+4, i, ' ');
        mvaddch(secondLn+6, i, ' ');
        mvaddch(secondLn+7, i, ' ');
        mvaddch(secondLn+9, i, ' ');
        mvaddch(secondLn+10, i, ' ');
    }
    mvprintw(counterFirstLn,    leftWall - 13, "Points:"); //then puts there some information for player
    mvprintw(counterFirstLn+1,  leftWall - 13, "%d", (int)points);
    mvprintw(counterFirstLn+3,  leftWall - 13, "Time:");
    mvprintw(counterFirstLn+4,  leftWall - 13, "%d", time(NULL) - startTime);
    mvprintw(counterFirstLn+6,  leftWall - 13, "Difficulty:");
    mvprintw(counterFirstLn+7,  leftWall - 13, "%d", difficulty);
    mvprintw(counterFirstLn+9,  leftWall - 13, "Lives:");
    mvprintw(counterFirstLn+10, leftWall - 13, "%d", LINES - global_player_x - 1);
    return;
}

bool isWall(const int &x, const int &y) {
    move(x, y); //moves cursor to coordinates
    char checkedPoint = inch(); //get char from there
    if(checkedPoint == wallChar || checkedPoint == obstacleChar) 
        return true; //if it is wall under cursor - returns true
    return false; //else there is no wall
}

void log_out (const std::string & msg, char msgType) {
    static char msg_type[5];
    switch (msgType) { //converting char msgType to string
        case 'n':
            strcpy(msg_type, "okay");
            break;
        case 'w':
            strcpy(msg_type, "warn");
            break;
        case 'e':
            strcpy(msg_type, "err ");
            break;
        case 'q':
            fout << "-- END OF LOG --" << std::endl;
            return;
        case 's':
            fout << "-- START OF LOG --" << std::endl;
            return;
        default :
            strcpy(msg_type, "unkn");
            break;
    }
    fout << msg_type << " [" << time(NULL) - startTime << "]: " << msg << std::endl;
    //out message with msgType and time from start
}

void checkScreen () {
    if (COLS < (wallsWidth+30) || LINES < minLines) {
        char msg[100] = {0};
        sprintf(msg, "incorrect screen size\n\t  your:\t%dh\t\t%dw\n\t  correct: %dh min.\t%dw min.", LINES, COLS, minLines, wallsWidth+30);
        exit_s(msg, 'e'); //if screen is smaller than minimum puts error message to log file and close app
    }
    log_out("screen size is correct", 'n'); //else puts okay msg to log and continue
}

void logCounter () { //logs lines counter and current points to log file
    char temp[25] = {0};
    sprintf(temp, "counter is %d", counter);
    log_out(temp, 'n');
    sprintf(temp, "points is %d", (int)points);
    log_out(temp, 'n');
}

void bonus::shoot () {
    for (int i = global_player_x-1; i > 0; i--) {
        for (int y = global_player_y-1; y <= global_player_y+1; y++) {
            move (i, y);
            if(inch() == obstacleChar) { //clears vertical 3-char-wide line on screen
                points += (float)difficulty; //and gives points for every obstacle
                addch (' ');
            }
        }
    }
}

void bonus::hpUp () {
    global_player_x -= hpUpBonus; //puts player 5 lines above
    points += (float)difficulty*difficulty;
}

void bonus::hpDown () {
    global_player_x += hpDownBonus; //puts player 5 lines below
}

void bonus::generateBonus () {
    int bonus_x = leftWall + rand()%wallsWidth; //generate point inside walls
    switch (rand()%3) { //generates bonus to place
        case 0:
            mvaddch(0, bonus_x, bonus::shootChar | COLOR_PAIR(color::cyan));
            break;
        case 1:
            mvaddch(0, bonus_x, bonus::hpUpChar | COLOR_PAIR(color::green));
            break;
        case 2:
            mvaddch(0, bonus_x, bonus::hpDownChar | COLOR_PAIR(color::red));
            break;
        default:
            break;
    }
}

bool isPause () {
    if (pauseFlag) {
        mvprintw (LINES/2,   leftWall - 13, "Paused      ");
        mvprintw (LINES/2+1, leftWall - 13, "            ");
        return true;
    }
    else {
        mvprintw (LINES/2-1, leftWall - 13, "Space   ");
        mvprintw (LINES/2,   leftWall - 13, "to pause");
        mvprintw (LINES/2+1, leftWall - 13, "        ");
        return false;
    }
}

void log_open_s (void) {
    char *temp = new char[40];
    sprintf(temp, "logs/log-%lu.txt", time(NULL));
    fout.open(temp);
    if(!fout.is_open()) {
        std::cerr << "cannot open log file\n"; exit(1);
    }
    log_out("", 's');
    sprintf(temp, "start time is %lu", startTime);
    log_out(temp, 'n');
    delete[] temp;
}

void screen_init_s (void) {
    if(!initscr()) exit_s((char*)"initscr() failed", 'e');
    log_out("initscr executed normally", 'n');
}

void color_init_s (void) {
    if(!has_colors()) {
        endwin();
        exit_s((char*)"colors isn't allowed in this terminal", 'e');
    }
    start_color();
    init_pair(color::yellow, COLOR_YELLOW, COLOR_BLACK); // color pairs initialization
    init_pair(color::red, COLOR_RED, COLOR_BLACK);
    init_pair(color::cyan, COLOR_CYAN, COLOR_BLACK);
    init_pair(color::green, COLOR_GREEN, COLOR_BLACK);
}

void ncurses_init_s (void) {
    screen_init_s();
    checkScreen();
    noecho();
    curs_set(0);
    raw();
    keypad(stdscr, 1);
    color_init_s();
}

void variable_init (void) {
    leftWall = (COLS/2) - wallsWidth/2 - 1;
    rightWall = (COLS/2) + wallsWidth/2 + 1;
    global_player_x = LINES - playerLives - 1; //player starts with playerLives lines above bottom
    global_player_y = COLS/2;
}

void exit_s (char* msg, char msg_type) {
    logCounter();
    endwin();
    log_out(msg, msg_type);
    log_out("", 'q');
    fout.close();
    BASS_Free();
    printf("\x1b[0m");
    exit(1);
}

void music_init_s (void) {
    if (HIWORD(BASS_GetVersion())!=BASSVERSION)
        exit_s((char*)"BASS version err", 'e');
    if (!BASS_Init (-1, 22050, BASS_DEVICE_3D , 0, NULL))
        exit_s((char*)"BASS init failure", 'e');

    char background[] = "music/background.mp3";
    char game_over[] = "music/off1.mp3";
    char hp_up[] = "music/beep1.mp3";
    char hp_down[] = "music/beep2.mp3";
    char shoot[] = "music/shoot1.mp3";
    char lvl[] = "music/level1.mp3";

    back_stream = BASS_StreamCreateFile(FALSE, background, 0, 0, 0);
    game_over_stream = BASS_StreamCreateFile(FALSE, game_over, 0, 0, 0);
    hp_up_stream = BASS_StreamCreateFile(FALSE, hp_up, 0, 0, 0);
    hp_down_stream = BASS_StreamCreateFile(FALSE, hp_down, 0, 0, 0);
    shoot_stream = BASS_StreamCreateFile(FALSE, shoot, 0, 0, 0);
    lvl_stream = BASS_StreamCreateFile(FALSE, lvl, 0, 0, 0);

    if (!back_stream) exit_s((char*)"background BASS stream err", 'e');
    if (!game_over_stream) exit_s((char*)"gameover BASS stream err", 'e');
    if (!hp_up_stream) exit_s((char*)"hpup BASS stream err", 'e');
    if (!hp_down_stream) exit_s((char*)"hpdown BASS stream err", 'e');
    if (!shoot_stream) exit_s((char*)"shoot BASS stream err", 'e');
    if (!lvl_stream) exit_s((char*)"lvl BASS stream err", 'e');

    BASS_ChannelPlay(back_stream,TRUE);
}

void music_gameover (void) {
    BASS_ChannelPlay(game_over_stream,TRUE);
    napms(1500);
    BASS_ChannelStop(game_over_stream);
    BASS_StreamFree(game_over_stream);
}

void print_score (void) {
    mvprintw (LINES/2 - 1, COLS/2 - 17, "   Game over: Your score is %d   ", (int)points);
    mvprintw (LINES/2    , COLS/2 - 11, "   Press 'q' to exit   ");
    mvprintw (LINES/2 + 1, COLS/2 - 15, "   or press 'r' to restart   ");
}

int scoreAndExit() {
    char exitkey; //temp variable to store user key
    print_score();
    while (tolower(exitkey = getch()) != 'q') if (exitkey == 'r') return 1; //awaits for 'q' to  exit
    return 0;
}
