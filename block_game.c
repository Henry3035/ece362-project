#include <stdio.h>
#include <Windows.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <locale.h>

#define LEFT 75
#define RIGHT 77
#define UP 72
#define DOWN 80

int Score(int score);
int game_over();
int stage_level(int a);
int display_bar(int left, int right);
int three_block(int n);

// Direction modifiers for the ball
int ax = -1, ay = -1;
// Ball's current position
int ab_x, ab_y;
// Ball's previous position
int pre_x, pre_y;
// Position of the bar
int bar_left, bar_right, bar_up_down = 62;
// Game speed
int Time;
// Game score
int score_i = 0;
// Additional score for consecutive brick hits
int score_plus = 0;
// Game restart flag; if 1, then restart the game
int return_gameover = 0;
// Stage selection at game start
int select_stage;
// Number of lives
int life = 1;
// Game board array
int block_array[64][32];
enum { HIDDEN, SHOW };

void CursorView(char show) {
    HANDLE hConsole;
    CONSOLE_CURSOR_INFO ConsoleCursor;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    ConsoleCursor.bVisible = show;
    ConsoleCursor.dwSize = 1;
    SetConsoleCursorInfo(hConsole, &ConsoleCursor);
}

void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void setcolor(int color, int bgcolor) {
    color &= 0xF;
    bgcolor &= 0xF;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (bgcolor << 4) | color);
}

void randomize(void) {
    int i;
    srand((unsigned)time(NULL));
    for (i = 0; i < 100; i++) {
        rand();
    }
}

int show_all() {
    int i, j;
    for (i = 0; i < 64; i++) {
        for (j = 0; j < 32; j++) {
            if (block_array[i][j] == 1) { // Wall
                setcolor(12, 0);  // red
                printf("■");
            }
            else if (block_array[i][j] == 2) { // Brick
                setcolor(9, 0);   // blue
                printf("■");
            }
            else if (block_array[i][j] == 3) { // Brick type 2
                setcolor(5, 0);   // purple
                printf("■");
            }
            else if (block_array[i][j] == 4) { // Brick type 3
                setcolor(14, 0);  // yellow
                printf("■");
            }
            else if (block_array[i][j] == 5) { // Ball
                setcolor(11, 0);  // light blue
                printf("○");
            }
            else if (block_array[i][j] == 6) { // Bar
                setcolor(2, 0);   // green
                printf("□");
            }
            else {
                printf("  ");
            }
        }
        printf("\n");
    }
    setcolor(15, 0);  // white
    gotoxy(66, 64);
    printf("Score: %d", score_i);
    gotoxy(66, 67);
    printf("Life: %d", life);
    gotoxy(0, 10);
    return 0;
}

int moving_ball(int x, int y) {
    int i, j;
    // Collision with walls
    if (block_array[x][y] == 1) {
        if (x >= 0 && y == 0) {   // Left wall collision
            ay *= -1;
            y = y + ay + ay;
        }
        if (x >= 0 && y == 31) {  // Right wall collision
            ay *= -1;
            y = y + ay + ay;
        }
        if (block_array[x + 1][y] == 2 && x == 0) { // Simultaneous collision with brick and wall
            block_array[x + 1][y] = 0;
            ax *= -1;
            ay *= -1;
            x = x + ax + ax;
            y = y + ay + ay;
            score_i = Score(score_i);
        }
        if (block_array[x + 1][y] == 3 && x == 0) { // Brick type 2 collision
            block_array[x + 1][y] = 2;
            ax *= -1;
            ay *= -1;
            x = x + ax + ax;
            y = y + ay + ay;
            score_i = Score(score_i);
        }
        if (block_array[x + 1][y] == 4 && x == 0) { // Brick type 3 collision
            block_array[x + 1][y] = 3;
            ax *= -1;
            ay *= -1;
            x = x + ax + ax;
            y = y + ay + ay;
            score_i = Score(score_i);
        }
        if (x == 0 && y >= 0) {    // Top wall collision
            ax *= -1;
            x = x + ax + ax;
        }
        if (x == 63 && y >= 0) {   // Bottom wall collision
            life--;
            if (life == -1) {
                game_over();
                return 0;
            }
            else {
                x = 60;
                y = 15;
                ax *= -1;
                ay *= -1;
                for (i = 0; i < 64; i++) {
                    for (j = 0; j < 32; j++) {
                        if (block_array[i][j] == 6)
                            block_array[i][j] = 0;
                    }
                }
                bar_up_down = 62;
                stage_level(select_stage);
                display_bar(bar_left, bar_right);
                gotoxy(0, 64);
                system("PAUSE");
                gotoxy(0, 10);
            }
        }
    }
    // Collision with bricks
    if (block_array[x][y] == 2 || block_array[x][y - ay] == 2 ||
        block_array[x][y] == 3 || block_array[x][y - ay] == 3 ||
        block_array[x][y] == 4 || block_array[x][y - ay] == 4) {
        if (ay == -1) {
            if (block_array[x][y + 1] == 2) {
                block_array[x][y + 1] = 0;
                ax *= -1;
                x = x + ax + ax;
            }
            else if (block_array[x][y + 1] == 3) {
                block_array[x][y + 1] = 2;
                ax *= -1;
                x = x + ax + ax;
            }
            else if (block_array[x][y + 1] == 4) {
                block_array[x][y + 1] = 3;
                ax *= -1;
                x = x + ax + ax;
            }
            else if (block_array[x][y] == 4) {
                block_array[x][y] = 3;
                ax *= -1;
                ay *= -1;
                x = x + ax;
                y = y + ay;
            }
            else if (block_array[x][y] == 3) {
                block_array[x][y] = 2;
                ax *= -1;
                ay *= -1;
                x = x + ax;
                y = y + ay;
            }
            else {
                block_array[x][y] = 0;
                ax *= -1;
                ay *= -1;
                x = x + ax;
                y = y + ay;
            }
        }
        else {
            if (block_array[x][y - 1] == 2) {
                block_array[x][y - 1] = 0;
                ax *= -1;
                x = x + ax + ax;
            }
            else if (block_array[x][y - 1] == 3) {
                block_array[x][y - 1] = 2;
                ax *= -1;
                x = x + ax + ax;
            }
            else if (block_array[x][y - 1] == 4) {
                block_array[x][y - 1] = 3;
                ax *= -1;
                x = x + ax + ax;
            }
            else if (block_array[x][y] == 4) {
                block_array[x][y] = 3;
                ax *= -1;
                ay *= -1;
                x = x + ax;
                y = y + ay;
            }
            else if (block_array[x][y] == 3) {
                block_array[x][y] = 2;
                ax *= -1;
                ay *= -1;
                x = x + ax;
                y = y + ay;
            }
            else {
                block_array[x][y] = 0;
                ax *= -1;
                ay *= -1;
                x = x + ax;
                y = y + ay;
            }
        }
        score_i = Score(score_i);
    }
    if (block_array[x - ax][y] == 2 || block_array[x - ax][y] == 3 || block_array[x - ax][y] == 4) {
        if (ax == -1) {
            if (block_array[x + 1][y] == 2)
                block_array[x + 1][y] = 0;
            if (block_array[x + 1][y] == 3)
                block_array[x + 1][y] = 2;
            if (block_array[x + 1][y] == 4)
                block_array[x + 1][y] = 3;
        }
        else {
            if (block_array[x - 1][y] == 2)
                block_array[x - 1][y] = 0;
            if (block_array[x - 1][y] == 3)
                block_array[x - 1][y] = 2;
            if (block_array[x - 1][y] == 4)
                block_array[x - 1][y] = 3;
        }
        ay *= -1;
        y = y + ay + ay;
        score_i = Score(score_i);
    }
    if (block_array[x][y] == 6 || block_array[x][y - ay] == 6) {
        if (block_array[x][y] == 6 && block_array[x][y - ay] == 0) {
            ax *= -1;
            ay *= -1;
            x = x + ax + ax;
            y = y + ay + ay;
        }
        else {
            ax *= -1;
            x = x + ax + ax;
        }
        score_plus = 0;
    }
    block_array[pre_x][pre_y] = 0;
    block_array[x][y] = 5;
    pre_x = x;
    pre_y = y;
    y = y + ay;
    x = x + ax;
    ab_x = x;
    ab_y = y;
    return 0;
}

int display_bar(int left, int right) {
    int i;
    for (i = left; i <= right; i++)
        block_array[62][i] = 6;
    return 0;
}

int moving_bar(int left, int right, int up_down) {
    int i, j;
    int ch, temp_l, temp_r, temp_up_down;
    for (i = left; i <= right; i++)
        block_array[up_down][i] = 6;
    ch = getch();
    switch (ch) {
    case LEFT:
        if (left == 1) {
            return 0;
        }
        else {
            temp_l = left;
            temp_r = right;
            left--;
            right--;
            block_array[up_down][left] = 6;
            block_array[up_down][right] = 6;
            block_array[up_down][temp_r] = 0;
            bar_left = left;
            bar_right = right;
        }
        break;
    case RIGHT:
        if (right == 30) {
            return 0;
        }
        else {
            temp_l = left;
            temp_r = right;
            left++;
            right++;
            block_array[up_down][left] = 6;
            block_array[up_down][right] = 6;
            block_array[up_down][temp_l] = 0;
            bar_left = left;
            bar_right = right;
        }
        break;
    case UP:
        if (up_down == 10) {
            bar_up_down = 62;
            for (j = left; j <= right; j++)
                block_array[10][j] = 0;
        }
        else {
            temp_up_down = up_down;
            up_down--;
            for (j = left; j <= right; j++) {
                block_array[up_down][j] = 6;
                block_array[temp_up_down][j] = 0;
            }
            bar_up_down = up_down;
        }
        break;
    case DOWN:
        if (up_down == 62)
            bar_up_down = 62;
        else {
            temp_up_down = up_down;
            up_down++;
            for (j = left; j <= right; j++) {
                block_array[up_down][j] = 6;
                block_array[temp_up_down][j] = 0;
            }
            bar_up_down = up_down;
        }
        break;
    }
    return 0;
}

int clear_check(int true_false) {
    int temp_array[2000];
    int i, j, ch;
    int k = 0;
    for (i = 0; i < 64; i++) {
        for (j = 0; j < 32; j++) {
            if (block_array[i][j] == 2) {
                temp_array[k] = 2;
                k++;
            }
        }
    }
    if (temp_array[0] != 2) {
        gotoxy(24, 30);
        printf("===============\n");
        gotoxy(24, 31);
        printf("   C L E A R\n");
        gotoxy(24, 32);
        printf("===============\n");
        k = 0;
        ch = getch();
    }
    return k;
}

int stage_level(int a) {
    int i;
    switch (a) {
    case 1:
        bar_left = 12;
        bar_right = 19;
        Time = 70;
        break;
    case 2:
        bar_left = 12;
        bar_right = 19;
        Time = 70;
        break;
    case 3:
        bar_left = 12;
        bar_right = 19;
        Time = 60;
        break;
    case 4:
        bar_left = 12;
        bar_right = 19;
        Time = 60;
        break;
    case 5:
        bar_left = 13;
        bar_right = 19;
        Time = 50;
        break;
    case 6:
        bar_left = 13;
        bar_right = 19;
        Time = 40;
        break;
    case 7:
        bar_left = 13;
        bar_right = 19;
        Time = 30;
        break;
    case 8:
        bar_left = 13;
        bar_right = 19;
        Time = 20;
        break;
    case 9:
        bar_left = 14;
        bar_right = 18;
        Time = 10;
        break;
    case 10:
        bar_left = 14;
        bar_right = 17;
        Time = 10;
        break;
    }
    return 0;
}

int stage(int a) {
    int i;
    switch (a) {
    case 1:
        bar_left = 12;
        bar_right = 19;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 1");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i++)
            block_array[1][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[2][i] = 2;
        three_block(2);
        Time = 150;
        break;
    case 2:
        bar_left = 12;
        bar_right = 19;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 2");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i++)
            block_array[1][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[2][i] = 2;
        for (i = 5; i < 10; i++)
            block_array[5][i] = 2;
        for (i = 20; i < 25; i++)
            block_array[6][i] = 2;
        three_block(3);
        Time = 140;
        break;
    case 3:
        bar_left = 12;
        bar_right = 19;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 3");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i++)
            block_array[1][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[2][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[4][i] = 2;
        three_block(4);
        Time = 130;
        break;
    case 4:
        bar_left = 12;
        bar_right = 19;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 4");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i++)
            block_array[1][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[2][i] = 2;
        for (i = 1; i < 16; i++)
            block_array[5][i] = 2;
        for (i = 16; i < 31; i++)
            block_array[7][i] = 2;
        three_block(5);
        Time = 120;
        break;
    case 5:
        bar_left = 13;
        bar_right = 19;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 5");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i++)
            block_array[1][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[2][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[3][i] = 2;
        for (i = 2; i < 31; i += 2)
            block_array[4][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[5][i] = 2;
        three_block(6);
        Time = 110;
        break;
    case 6:
        bar_left = 13;
        bar_right = 19;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 6");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i++)
            block_array[1][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[2][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[3][i] = 2;
        for (i = 2; i < 31; i += 2)
            block_array[4][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[5][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[6][i] = 2;
        three_block(7);
        Time = 100;
        break;
    case 7:
        bar_left = 13;
        bar_right = 19;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 7");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i++)
            block_array[1][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[2][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[3][i] = 2;
        for (i = 2; i < 31; i += 2)
            block_array[4][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[5][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[6][i] = 2;
        for (i = 1; i < 8; i++)
            block_array[8][i] = 2;
        for (i = 8; i < 15; i++)
            block_array[9][i] = 2;
        for (i = 15; i < 22; i++)
            block_array[10][i] = 2;
        for (i = 22; i < 31; i++)
            block_array[11][i] = 2;
        three_block(8);
        Time = 90;
        break;
    case 8:
        bar_left = 13;
        bar_right = 19;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 8");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i++)
            block_array[1][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[2][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[3][i] = 2;
        for (i = 2; i < 31; i += 2)
            block_array[4][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[5][i] = 2;
        for (i = 1; i < 31; i++)
            block_array[6][i] = 2;
        for (i = 1; i < 8; i++)
            block_array[8][i] = 2;
        for (i = 8; i < 15; i++)
            block_array[9][i] = 2;
        for (i = 15; i < 22; i++)
            block_array[10][i] = 2;
        for (i = 22; i < 31; i++)
            block_array[11][i] = 2;
        for (i = 1; i < 8; i++)
            block_array[11][i] = 2;
        for (i = 8; i < 15; i++)
            block_array[10][i] = 2;
        for (i = 15; i < 22; i++)
            block_array[9][i] = 2;
        for (i = 22; i < 31; i++)
            block_array[8][i] = 2;
        three_block(9);
        Time = 80;
        break;
    case 9:
        bar_left = 14;
        bar_right = 18;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 9");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i += 2)
            block_array[1][i] = 2;
        for (i = 2; i < 31; i += 2)
            block_array[2][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[3][i] = 2;
        for (i = 2; i < 31; i += 2)
            block_array[4][i] = 2;
        three_block(10);
        Time = 50;
        break;
    case 10:
        bar_left = 14;
        bar_right = 17;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("LAST");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for (i = 1; i < 31; i += 2)
            block_array[1][i] = 2;
        for (i = 2; i < 31; i += 2)
            block_array[2][i] = 2;
        for (i = 1; i < 31; i += 2)
            block_array[3][i] = 2;
        for (i = 2; i < 31; i += 2)
            block_array[4][i] = 2;
        three_block(12);
        Time = 50;
        break;
    }
    return 0;
}

int Score(int score) {
    score = score + score_plus + 10;
    score_plus++;
    return score;
}

int game_over() {
    char ch;
    gotoxy(24, 30);
    printf("===============\n");
    gotoxy(24, 31);
    printf("   Game Over\n");
    gotoxy(24, 32);
    printf("===============\n");
    gotoxy(20, 34);
    printf("Do you wanna re-game?[y/n] : ");
    fflush(stdin);
    scanf("%c", &ch);
    if (ch == 'y') {
        fflush(stdin);
        return_gameover = 1;
    }
    else {
        exit(1);
    }
    return 0;
}

int init() {
    int i;
    system("cls");
    moving_ball(60, 15);
    for (i = bar_left; i <= bar_right; i++)
        block_array[bar_up_down][i] = 0;
    bar_up_down = 62;
    block_array[63][0] = 1;
    block_array[0][0] = 1;
    gotoxy(0, 10);
    return 0;
}

int intro() {
    printf("Select Stage [1-10] : ");
    scanf("%d", &select_stage);
    return 0;
}

int init_variable() {
    int k, l;
    ax = -1;
    ay = -1;
    bar_up_down = 62;
    score_i = 0;
    score_plus = 0;
    return_gameover = 0;
    for (k = 0; k < 64; k++) {
        if (k == 0 || k == 63) {
            for (l = 0; l < 32; l++)
                block_array[k][l] = 1;
        }
        else {
            for (l = 0; l < 32; l++) {
                if (l == 0 || l == 31)
                    block_array[k][l] = 1;
                else
                    block_array[k][l] = 0;
            }
        }
    }
    return 0;
}

int three_block(int n) {
    int i, x, y;
    randomize();
    for (i = 1; i <= n; i++) {
        x = rand() % 62 + 1;
        y = rand() % 30 + 1;
        block_array[x][y] = 4;
    }
    return 0;
}

int main() {
    int i, j;
    char keytemp;
    int restart_game = 1;
    system("chcp 65001");
    setlocale(LC_ALL, "ko_KR.UTF-8");

    while (restart_game) {
        restart_game = 0;
        return_gameover = 0;
        life = 1;
        init_variable();
        intro();

        for (i = select_stage; i <= 10; i++) {
            init();
            stage(i);
            gotoxy(0, 10);
            CursorView(HIDDEN);
            show_all();

            while (1) {
                j = 1;
                block_array[0][0] = 1;

                if (kbhit()) {
                    keytemp = getch();
                    moving_bar(bar_left, bar_right, bar_up_down);
                }

                gotoxy(0, 10);
                show_all();

                j = clear_check(j);

                if (j == 0) {
                    if (i == 10) {
                        gotoxy(24, 30);
                        printf("========\n");
                        gotoxy(24, 31);
                        printf("mission clear\n");
                        gotoxy(24, 32);
                        printf("========\n");
                        break;
                    }
                    else {
                        break;
                    }
                }

                Sleep(Time);

                if (return_gameover == 1) {
                    system("cls");
                    restart_game = 1;
                    break;
                }

                moving_ball(ab_x, ab_y);
            }

            if (restart_game == 1) {
                break;
            }
        }
    }

    return 0;
}