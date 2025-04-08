#include "stm32f0xx.h"   


#include <stdio.h>
#include <Windows.h>
#include <stdlib.h>
#include <time.h>


#define LEFT 75
#define RIGHT 77
#define UP 72
#define DOWN 80

// Direction modifiers for the ball
int ax = -1, ay = -1;     
// Ball's current position
int ab_x, ab_y;            
// Ball's previous position 
int pre_x, pre_y;        
// Position of the bar
int bar_left, bar_right, bar_up_down = 19; 
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
int block_array[21][14];   
enum { HIDDEN, SHOW };

/*
 * CursorView:
 * Hides or displays the console cursor.
 * Parameter 'show': nonzero to display the cursor; zero to hide.
 */
void CursorView(char show) {
    HANDLE hConsole;
    CONSOLE_CURSOR_INFO ConsoleCursor;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    ConsoleCursor.bVisible = show;
    ConsoleCursor.dwSize = 1;
    SetConsoleCursorInfo(hConsole, &ConsoleCursor);
}

/*
 * gotoxy:
 * Moves the console cursor to the specified (x,y) coordinate.
 */
void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

/*
 * setcolor:
 * Sets the foreground and background colors in the console.
 * 'color' is the text color; 'bgcolor' is the background color.
 */
void setcolor(int color, int bgcolor) {
    color &= 0xF;
    bgcolor &= 0xF;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (bgcolor << 4) | color);
}

/*
 * randomize:
 * Seeds the random number generator.
 * Also discards a few initial random numbers to improve randomness.
 */
void randomize(void) {
    int i;
    srand((unsigned)time(NULL));
    // Discard some initial numbers
    for (i = 0; i < (rand() % RAND_MAX); i++)
        (rand() % RAND_MAX);
}

/*
 * show_all:
 * Displays the entire game board (walls, bricks, ball, paddle) on the console.
 */
int show_all() {
    int i, j;
    setcolor(10, 0);
    for(i = 0; i < 21; i++) {
        for(j = 0; j < 14; j++) {
            if(block_array[i][j] == 1) { // Wall
                setcolor(7, 0);
                printf("■");
            }
            else if(block_array[i][j] == 2) { // Brick (type 1)
                setcolor(10, 0);
                printf("■");
                setcolor(7, 0);
            }
            else if(block_array[i][j] == 3) { // Brick (type 2)
                setcolor(13, 0);
                printf("■");
                setcolor(7, 0);
            }
            else if(block_array[i][j] == 4) { // Brick (type 3)
                setcolor(12, 0);
                printf("■");
                setcolor(7, 0);
            }
            else if(block_array[i][j] == 5) { // Ball
                setcolor(5, 0);
                printf("○");
                setcolor(7, 0);
            }
            else if(block_array[i][j] == 6) { // Paddle (bar)
                setcolor(8, 0);
                printf("□");
                setcolor(7, 0);
            }
            else {
                printf("  ");
            }
        }
        printf("\n");
    }
    gotoxy(30, 20);
    printf("Score: %d", score_i);
    gotoxy(30, 23);
    printf("Life: %d", life);
    gotoxy(0, 10);
    return 0;
}

int moving_ball(int x, int y) {
    int i, j;
    
    // Collision with walls
    if(block_array[x][y] == 1) {
        if(x >= 0 && y == 0) {   // Left wall collision
            ay *= -1;
            y = y + ay + ay;
        }
        if(x >= 0 && y == 13) {  // Right wall collision
            ay *= -1;
            y = y + ay + ay;
        }
        if(block_array[x+1][y] == 2 && x == 0) { // Simultaneous collision with brick and wall
            block_array[x+1][y] = 0;
            ax *= -1;
            ay *= -1;
            x = x + ax + ax;
            y = y + ay + ay;
            score_i = Score(score_i);
        }
        if(block_array[x+1][y] == 3 && x == 0) { // Brick type 2 collision
            block_array[x+1][y] = 2;
            ax *= -1;
            ay *= -1;
            x = x + ax + ax;
            y = y + ay + ay;
            score_i = Score(score_i);
        }
        if(block_array[x+1][y] == 4 && x == 0) { // Brick type 3 collision
            block_array[x+1][y] = 3;
            ax *= -1;
            ay *= -1;
            x = x + ax + ax;
            y = y + ay + ay;
            score_i = Score(score_i);
        }
        if(x == 0 && y >= 0) {    // Top wall collision
            ax *= -1;
            x = x + ax + ax;
        }
        if(x == 20 && y >= 0) {   // Bottom wall collision
            life--;  // Decrement life
            if(life == -1) { // No lives remaining
                game_over();
                return 0;
            }
            else {
                x = 17;
                y = 5;
                ax *= -1;
                ay *= -1;
                for(i = 0; i < 21; i++) {
                    for(j = 0; j < 14; j++) {
                        if(block_array[i][j] == 6)
                            block_array[i][j] = 0;
                    }
                }
                bar_up_down = 19;
                stage_level(select_stage);
                display_bar(bar_left, bar_right);
                gotoxy(0, 20);
                system("PAUSE");
                gotoxy(0, 10);
            }
        }
    }
    
    // Collision with bricks
    if(block_array[x][y] == 2 || block_array[x][y - ay] == 2 ||
       block_array[x][y] == 3 || block_array[x][y - ay] == 3 ||
       block_array[x][y] == 4 || block_array[x][y - ay] == 4) {
       
        // When the ball is moving from right to left while rising
        if(ay == -1) {
            if(block_array[x][y + 1] == 2) {
                block_array[x][y + 1] = 0;
                ax *= -1;
                x = x + ax + ax;
            }
            else if(block_array[x][y + 1] == 3) {
                block_array[x][y + 1] = 2;
                ax *= -1;
                x = x + ax + ax;
            }
            else if(block_array[x][y + 1] == 4) {
                block_array[x][y + 1] = 3;
                ax *= -1;
                x = x + ax + ax;
            }
            else if(block_array[x][y] == 4) {
                block_array[x][y] = 3;
                ax *= -1;
                ay *= -1;
                x = x + ax;
                y = y + ay;
            }
            else if(block_array[x][y] == 3) {
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
        // When the ball is moving from left to right while rising
        else {
            if(block_array[x][y - 1] == 2) {
                block_array[x][y - 1] = 0;
                ax *= -1;
                x = x + ax + ax;
            }
            else if(block_array[x][y - 1] == 3) {
                block_array[x][y - 1] = 2;
                ax *= -1;
                x = x + ax + ax;
            }
            else if(block_array[x][y - 1] == 4) {
                block_array[x][y - 1] = 3;
                ax *= -1;
                x = x + ax + ax;
            }
            else if(block_array[x][y] == 4) {
                block_array[x][y] = 3;
                ax *= -1;
                ay *= -1;
                x = x + ax;
                y = y + ay;
            }
            else if(block_array[x][y] == 3) {
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
    
    // Collision with brick sides
    if(block_array[x - ax][y] == 2 || block_array[x - ax][y] == 3 || block_array[x - ax][y] == 4) {
        if(ax == -1) {
            if(block_array[x + 1][y] == 2)
                block_array[x + 1][y] = 0;
            if(block_array[x + 1][y] == 3)
                block_array[x + 1][y] = 2;
            if(block_array[x + 1][y] == 4)
                block_array[x + 1][y] = 3;
        }
        else {
            if(block_array[x - 1][y] == 2)
                block_array[x - 1][y] = 0;
            if(block_array[x - 1][y] == 3)
                block_array[x - 1][y] = 2;
            if(block_array[x - 1][y] == 4)
                block_array[x - 1][y] = 3;
        }
        ay *= -1;
        y = y + ay + ay;
        score_i = Score(score_i);
    }
    
    // Collision with the paddle
    if(block_array[x][y] == 6 || block_array[x][y - ay] == 6) {
        if(block_array[x][y] == 6 && block_array[x][y - ay] == 0) { // Hit paddle edge
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
    
    // Clear the ball's previous position and update its new position
    // Erase previous trail
    block_array[pre_x][pre_y] = 0;  
     // Display ball at new location
    block_array[x][y] = 5;         
    pre_x = x;
    pre_y = y;

    // Update ball position for the next movement
    y = y + ay;
    x = x + ax;

    ab_x = x;
    ab_y = y;
    return 0;
}

/*
 * display_bar:
 * Displays the paddle (bar) on the game board between the given left and right positions.
 */
int display_bar(int left, int right) {
    int i;
    for(i = left; i <= right; i++)
        block_array[19][i] = 6;
    return 0;
}

/*
 * moving_bar:
 * Moves the paddle based on key input.
 * Adjusts the left, right, and vertical positions based on arrow keys.
 */
int moving_bar(int left, int right, int up_down) {
    int i, j;
    // Variables to store previous positions
    int ch, temp_l, temp_r, temp_up_down; 
    // Display the paddle at its current position
    for(i = left; i <= right; i++)         
        block_array[up_down][i] = 6;
    // Wait for key input
    ch = getch();  
    switch(ch) {
    /// Move left
    case LEFT:   
        // Prevent moving if at left edge   
        if(left == 1) {    
            return 0;
        }
        else {        
            temp_l = left;  
            temp_r = right;   
            left--;      
            right--;
            // Show paddle in new position
            block_array[up_down][left] = 6;  
            block_array[up_down][right] = 6;
            block_array[up_down][temp_r] = 0;
            bar_left = left;   
            bar_right = right;
        }
        break;
    case RIGHT:     
        if(right == 12) {
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
        if(up_down == 10) {
            // Reset to bottom if moving too high
            bar_up_down = 19;  
            for(j = left; j <= right; j++)  
                block_array[10][j] = 0;
        }
        else {
            temp_up_down = up_down;
            up_down--;
            for(j = left; j <= right; j++) {
                block_array[up_down][j] = 6;
                block_array[temp_up_down][j] = 0;
            }
            bar_up_down = up_down;
        }
        break;
    case DOWN:      
        if(up_down == 19)
            bar_up_down = 19;
        else {
            temp_up_down = up_down;
            up_down++;
            for(j = left; j <= right; j++) {
                block_array[up_down][j] = 6;
                block_array[temp_up_down][j] = 0;
            }
            bar_up_down = up_down;
        }
        break;
    }
    return 0;
}

/*
  Checks whether any bricks (value 2) remain on the game board.
  Returns the count of remaining bricks; if none remain, the game is cleared.
 */
int clear_check(int true_false) {
    // Array to store remaining bricks
    int temp_array[200];    
    int i, j, ch;
    int k = 0;             
    for(i = 0; i < 21; i++) {
        for(j = 0; j < 14; j++) {
            if(block_array[i][j] == 2) {
                temp_array[k] = 2;
                k++;
            }
        }
    }
    // If no brick (value 2) is found, game clear
    if(temp_array[0] != 2) {  
        gotoxy(6, 15);
        printf("===============\n");
        gotoxy(6, 16);
        printf("   C L E A R\n");
        gotoxy(6, 17);
        printf("===============\n");
        k = 0;
        ch = getch();
    }
    return k;
}

/*
   Sets the game speed and paddle positions based on the stage number.
 */
int stage_level(int a) {
    int i;
    switch(a) {
    case 1:  // Stage 1
        bar_left = 4;
        bar_right = 7;
        Time = 150;
        break;
    case 2:  // Stage 2
        bar_left = 4;
        bar_right = 7;
        Time = 140;
        break;
    case 3:  // Stage 3
        bar_left = 4;
        bar_right = 7;
        Time = 130;
        break;
    case 4:  // Stage 4
        bar_left = 4;
        bar_right = 7;
        Time = 120;
        break;
    case 5:  // Stage 5
        bar_left = 5;
        bar_right = 7;
        Time = 110;
        break;
    case 6:  // Stage 6
        bar_left = 5;
        bar_right = 7;
        Time = 100;
        break;
    case 7:  // Stage 7
        bar_left = 5;
        bar_right = 7;
        Time = 90;
        break;
    case 8:  // Stage 8
        bar_left = 5;
        bar_right = 7;
        Time = 80;
        break;
    case 9:  // Stage 9
        bar_left = 6;
        bar_right = 7;
        Time = 50;
        break;
    case 10:  // Stage 10
        bar_left = 6;
        bar_right = 6;
        Time = 50;
        break;
    }
    return 0;
}

/*
   Sets up the game board and paddle for a given stage.
   This function also sets brick locations and the game speed.
 */
int stage(int a) {
    int i;
    switch(a) {
    case 1:  // Stage 1
        bar_left = 4;
        bar_right = 7;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 1");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i++)
            block_array[1][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[2][i] = 2;
        three_block(1);
        Time = 150;
        break;
    case 2:  // Stage 2
        bar_left = 4;
        bar_right = 7;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 2");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i++)
            block_array[1][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[2][i] = 2;
        for(i = 3; i < 6; i++)
            block_array[5][i] = 2;
        for(i = 8; i < 11; i++)
            block_array[6][i] = 2;
        three_block(1);
        Time = 140;
        break;
    case 3:  // Stage 3
        bar_left = 4;
        bar_right = 7;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 3");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i++)
            block_array[1][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[2][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[4][i] = 2;
        three_block(2);
        Time = 130;
        break;
    case 4:  // Stage 4
        bar_left = 4;
        bar_right = 7;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 4");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i++)
            block_array[1][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[2][i] = 2;
        for(i = 1; i < 7; i++)
            block_array[5][i] = 2;
        for(i = 7; i < 13; i++)
            block_array[7][i] = 2;
        three_block(2);
        Time = 120;
        break;
    case 5:  // Stage 5
        bar_left = 5;
        bar_right = 7;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 5");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i++)
            block_array[1][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[2][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[3][i] = 2;
        for(i = 2; i < 13; i += 2)
            block_array[4][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[5][i] = 2;
        three_block(3);
        Time = 110;
        break;
    case 6:  // Stage 6
        bar_left = 5;
        bar_right = 7;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 6");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i++)
            block_array[1][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[2][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[3][i] = 2;
        for(i = 2; i < 13; i += 2)
            block_array[4][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[5][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[6][i] = 2;
        three_block(3);
        Time = 100;
        break;
    case 7:  // Stage 7
        bar_left = 5;
        bar_right = 7;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 7");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i++)
            block_array[1][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[2][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[3][i] = 2;
        for(i = 2; i < 13; i += 2)
            block_array[4][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[5][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[6][i] = 2;
        for(i = 1; i < 4; i++)
            block_array[8][i] = 2;
        for(i = 4; i < 7; i++)
            block_array[9][i] = 2;
        for(i = 7; i < 10; i++)
            block_array[10][i] = 2;
        for(i = 10; i < 13; i++)
            block_array[11][i] = 2;
        three_block(4);
        Time = 90;
        break;
    case 8:  // Stage 8
        bar_left = 5;
        bar_right = 7;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 8");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i++)
            block_array[1][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[2][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[3][i] = 2;
        for(i = 2; i < 13; i += 2)
            block_array[4][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[5][i] = 2;
        for(i = 1; i < 13; i++)
            block_array[6][i] = 2;
        for(i = 1; i < 4; i++)
            block_array[8][i] = 2;
        for(i = 4; i < 7; i++)
            block_array[9][i] = 2;
        for(i = 7; i < 10; i++)
            block_array[10][i] = 2;
        for(i = 10; i < 13; i++)
            block_array[11][i] = 2;
        for(i = 1; i < 4; i++)
            block_array[11][i] = 2;
        for(i = 4; i < 7; i++)
            block_array[10][i] = 2;
        for(i = 7; i < 10; i++)
            block_array[9][i] = 2;
        for(i = 10; i < 13; i++)
            block_array[8][i] = 2;
        three_block(4);
        Time = 80;
        break;
    case 9:  // Stage 9
        bar_left = 6;
        bar_right = 7;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("STAGE 9");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i += 2)
            block_array[1][i] = 2;
        for(i = 2; i < 13; i += 2)
            block_array[2][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[3][i] = 2;
        for(i = 2; i < 13; i += 2)
            block_array[4][i] = 2;
        three_block(5);
        Time = 50;
        break;
    case 10:  // Stage 10 (Last)
        bar_left = 6;
        bar_right = 6;
        gotoxy(0, 9);
        setcolor(0, 15);
        printf("LAST");
        gotoxy(0, 10);
        display_bar(bar_left, bar_right);
        for(i = 1; i < 13; i += 2)
            block_array[1][i] = 2;
        for(i = 2; i < 13; i += 2)
            block_array[2][i] = 2;
        for(i = 1; i < 13; i += 2)
            block_array[3][i] = 2;
        for(i = 2; i < 13; i += 2)
            block_array[4][i] = 2;
        three_block(5);
        Time = 50;
        break;
    }
    return 0;
}

/*
 * Score:
 * Calculates and updates the score by adding the base value plus any 
 * additional score from consecutive brick hits.
 */
int Score(int score) {
    score = score + score_plus + 10;
    score_plus++;
    return score;
}

/*
    game_over:
    Displays a "Game Over" message and asks the player if they want to restart.
    Returns control based on the user's choice.
 */
int game_over() {
    char ch;
    gotoxy(6, 15);
    printf("===============\n");
    gotoxy(6, 16);
    printf("   Game Over\n");
    gotoxy(6, 17);
    printf("===============\n");
    gotoxy(4, 19);
    printf("Do you wanna re-game?[y/n] : ");
    fflush(stdin);
    scanf("%c", &ch);   
    // Restart game if 'y' is pressed
    if(ch == 'y') { 
        fflush(stdin);
        return_gameover = 1;
    }
    else {  
        exit(1);
    }
    return 0;
}

/*
 * background:
 * Displays background elements like the "BRICK" text and arrow key instructions.
 */
int background() {
    gotoxy(30, 10);
    setcolor(14, 0);
    printf("□□□");
    gotoxy(30, 11);
    printf("□   □");
    gotoxy(30, 12);
    printf("□□□");
    gotoxy(30, 13);
    printf("□   □");
    gotoxy(30, 14);
    printf("□□□");

    gotoxy(38, 10);
    setcolor(9, 0);
    printf("□□□");
    gotoxy(38, 11);
    printf("□   □");
    gotoxy(38, 12);
    printf("□□□");
    gotoxy(38, 13);
    printf("□   □");
    gotoxy(38, 14);
    printf("□    □");

    gotoxy(47, 10);
    setcolor(12, 0);
    printf("□");
    gotoxy(47, 11);
    printf("□");
    gotoxy(47, 12);
    printf("□");
    gotoxy(47, 13);
    printf("□");
    gotoxy(47, 14);
    printf("□");

    gotoxy(50, 10);
    setcolor(11, 0);
    printf(" □□");
    gotoxy(50, 11);
    printf("□  □");
    gotoxy(50, 12);
    printf("□");
    gotoxy(50, 13);
    printf("□  □");
    gotoxy(50, 14);
    printf(" □□");

    gotoxy(57, 10);
    setcolor(13, 0);
    printf("□   □");
    gotoxy(57, 11);
    printf("□ □");
    gotoxy(57, 12);
    printf("□□");
    gotoxy(57, 13);
    printf("□ □");
    gotoxy(57, 14);
    printf("□   □");

    gotoxy(40, 29);
    setcolor(14, 0);
    printf("△");
    gotoxy(30, 30);
    printf("Key Control: ◁ ▽ ▷");

    return 0;
}

/*
 * init:
 * Initializes the game state when restarting, resets ball and paddle positions,
 * and clears previous paddle locations.
 */
int init() {
    int i;
    system("cls");        // Clear the console screen
    moving_ball(18, 5);   // Set ball's starting position
    for(i = bar_left; i <= bar_right; i++) // Clear previous paddle location
        block_array[bar_up_down][i] = 0;
    bar_up_down = 19;     // Reset paddle to the bottom
    block_array[19][0] = 1;  // Ensure wall appears by setting leftmost element to wall (1)
    block_array[0][0] = 1;
    gotoxy(0, 10);
    return 0;
}

/*
 * intro:
 * Displays the game's introductory screen and asks the player to select a stage.
 */
int intro() {
    gotoxy(5, 10);
    setcolor(14, 0);
    printf("□□□");
    gotoxy(5, 11);
    printf("□   □");
    gotoxy(5, 12);
    printf("□□□");
    gotoxy(5, 13);
    printf("□   □");
    gotoxy(5, 14);
    printf("□□□");

    gotoxy(13, 10);
    setcolor(9, 0);
    printf("■■■");
    gotoxy(13, 11);
    printf("■   ■");
    gotoxy(13, 12);
    printf("■■■");
    gotoxy(13, 13);
    printf("■   ■");
    gotoxy(13, 14);
    printf("■    ■");

    gotoxy(22, 10);
    setcolor(12, 0);
    printf("□");
    gotoxy(22, 11);
    printf("□");
    gotoxy(22, 12);
    printf("□");
    gotoxy(22, 13);
    printf("□");
    gotoxy(22, 14);
    printf("□");

    gotoxy(25, 10);
    setcolor(11, 0);
    printf(" ■■");
    gotoxy(25, 11);
    printf("■  ■");
    gotoxy(25, 12);
    printf("■");
    gotoxy(25, 13);
    printf("■  ■");
    gotoxy(25, 14);
    printf(" ■■");

    gotoxy(33, 10);
    setcolor(13, 0);
    printf("□   □");
    gotoxy(33, 11);
    printf("□ □");
    gotoxy(33, 12);
    printf("□□");
    gotoxy(33, 13);
    printf("□ □");
    gotoxy(33, 14);
    printf("□   □");

    gotoxy(10, 16);
    setcolor(7, 0);
    printf("Select Stage [1-10] : ");
    scanf("%d", &select_stage);
    return 0;
}

int init_variable() {
    int k, l;
    ax = -1;
    ay = -1;
    // ab_x, ab_y (ball position) remain uninitialized until set
    // pre_x, pre_y for previous ball position
    bar_up_down = 19;
    score_i = 0;
    score_plus = 0;
    return_gameover = 0;
     // Initialize game board array
    for(k = 0; k < 21; k++) { 
        if(k == 0 || k == 20) {
            for(l = 0; l < 14; l++)
                // Set walls on first and last rows
                block_array[k][l] = 1;  
        }
        else {
            for(l = 0; l < 14; l++) {
                if(l == 0 || l == 13)
                // Set walls on first and last columns
                    block_array[k][l] = 1;  
                else
                // Empty space
                    block_array[k][l] = 0; 
            }
        }
    }
    return 0;
}


int three_block(int n) {
    int i, x, y;
    randomize();
    for(i = 1; i <= n; i++) {
        // Random row within 15 rows
        x = rand() % 15 + 1;  
        // Random column within 13 columns
        y = rand() % 12 + 1;  
        block_array[x][y] = 4;
    }
    return 0;
}
int main() {
    int i, j;
    char keytemp;
    int restart_game = 1;

    while (restart_game) {
        restart_game = 0; 
        // 초기화      
        return_gameover = 0;    
         // 목숨 초기화
        life = 1;              
        // 변수 초기화
        init_variable();        
        // 인트로 화면
        intro();               

        for (i = select_stage; i <= 10; i++) {
            init();
            stage(i);
            background();
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
                        gotoxy(6, 15);
                        printf("===============\n");
                        gotoxy(6, 16);
                        printf("A L L   C L E A R\n");
                        gotoxy(6, 17);
                        printf("===============\n");
                        break;
                    } else {
                        break;
                    }
                }

                Sleep(Time);

                if (return_gameover == 1) {
                    system("cls");
                    // 다시 시작하도록 플래그 설정
                    restart_game = 1;  
                    // 현재 while 루프 탈출
                    break;            
                }

                moving_ball(ab_x, ab_y);
            }

            // 만약 게임오버로 빠져나온 거면 for문도 탈출
            if (restart_game == 1) {
                break;
            }
        }
    }

    return 0;
}
