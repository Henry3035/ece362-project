/*
 * Brick Breaker 게임 (C, Linux 콘솔)
 * STM32 HAL 및 하드웨어 의존 코드 제거
 * 컴파일: gcc -o brick_breaker brick_breaker.c
 * 실행: ./brick_breaker
 * 종료: 'q' 키 입력 또는 ESC 누름
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <string.h>

/* 터미널 raw 모드 설정: 비정규 모드, 키 입력 즉시 처리 */
static struct termios orig_termios;
void reset_terminal_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}
void set_conio_terminal_mode(void) {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));
    cfmakeraw(&new_termios);            /* ICANON, ECHO 등 비활성화 */
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    atexit(reset_terminal_mode);
}

/* 비차단 입력 검사 */
int kbhit(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) > 0;
}
/* 한 문자 읽기 */
int getch(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) < 0) return -1;
    return c;
}

/* 게임 영역 크기와 객체 설정 */
#define WIDTH       30       /* 전체 너비 (경계 포함) */
#define HEIGHT      20       /* 전체 높이 (경계 포함) */
#define PADDLE_LEN   7       /* 패들 길이 */
#define BRICK_ROWS   5
#define BRICK_COLS  ((WIDTH-2)/2)  /* 내부 폭/2 */

int bricks[BRICK_ROWS][BRICK_COLS];
int paddle_x;      /* 패들의 중앙 x좌표 */
int ball_x, ball_y;
int ball_dx, ball_dy;
int score = 0;

/* 게임 초기화 */
void init_game() {
    for(int r = 0; r < BRICK_ROWS; r++){
        for(int c = 0; c < BRICK_COLS; c++){
            bricks[r][c] = 1;
        }
    }
    paddle_x = WIDTH / 2;
    ball_x = WIDTH / 2;
    ball_y = HEIGHT - 3;
    ball_dx = 1;
    ball_dy = -1;
    score = 0;
}

/* 게임 화면 그리기 (ANSI 이스케이프 사용) */
void draw_game() {
    /* 화면 전체 지우고 커서 (0,0)으로 이동 */
    printf("\x1b[2J\x1b[H");
    /* 상단 경계 */
    for(int x = 0; x < WIDTH; x++) printf("-");
    printf("\n");
    /* 내부 영역(벽, 블럭, 공, 패들) */
    for(int y = 1; y < HEIGHT-1; y++) {
        printf("|");  /* 왼쪽 경계 */
        for(int x = 1; x < WIDTH-1; x++) {
            int drawn = 0;
            /* 블럭 그리기: 행 당 가로 폭 2칸 사용 */
            if(y-1 < BRICK_ROWS) {
                int col = (x-1) / 2;
                if((x-1) % 2 == 0 && col < BRICK_COLS && bricks[y-1][col]) {
                    printf("=");
                    drawn = 1;
                }
            }
            /* 공 그리기 */
            if(!drawn && x == ball_x && y == ball_y) {
                printf("O");
                drawn = 1;
            }
            /* 패들 그리기 (고정된 y 위치) */
            int half = PADDLE_LEN/2;
            if(!drawn && y == HEIGHT-2 &&
               x >= paddle_x-half && x <= paddle_x+half) {
                printf("=");
                drawn = 1;
            }
            /* 빈 칸 */
            if(!drawn) printf(" ");
        }
        printf("|");
        printf("\n");
    }
    /* 하단 경계 */
    for(int x = 0; x < WIDTH; x++) printf("-");
    printf("\n");
    /* 점수 출력 */
    printf("Score: %d\n", score);
}

/* 입력 처리: 화살표 키로 패들 이동, 'q' 또는 ESC로 종료 */
void process_input() {
    if(!kbhit()) return;
    int c = getch();
    if(c == 27) { /* ESC 또는 ESC 시퀀스 */
        if(kbhit() && getch() == '[') {
            int arrow = getch();
            if(arrow == 'C') { /* 오른쪽 */
                if(paddle_x + PADDLE_LEN/2 < WIDTH-2)
                    paddle_x++;
            } else if(arrow == 'D') { /* 왼쪽 */
                if(paddle_x - PADDLE_LEN/2 > 1)
                    paddle_x--;
            }
        } else {
            exit(0); /* ESC 단독: 종료 */
        }
    } else if(c == 'q' || c == 'Q') {
        exit(0); /* q/Q: 종료 */
    }
}

/* 공 위치 업데이트 및 충돌 처리 */
void update_ball() {
    int next_x = ball_x + ball_dx;
    int next_y = ball_y + ball_dy;
    /* 좌우 벽 충돌 */
    if(next_x <= 1 || next_x >= WIDTH-2) {
        ball_dx = -ball_dx;
        next_x = ball_x + ball_dx;
    }
    /* 위쪽 벽 충돌 */
    if(next_y <= 1) {
        ball_dy = -ball_dy;
        next_y = ball_y + ball_dy;
    }
    /* 패들 충돌 */
    if(next_y == HEIGHT-2) {
        if(next_x >= paddle_x - PADDLE_LEN/2 &&
           next_x <= paddle_x + PADDLE_LEN/2) {
            ball_dy = -ball_dy;
            next_y = ball_y + ball_dy;
        } else if(next_y >= HEIGHT-1) {
            init_game();  /* 패들을 놓치면 게임 초기화 */
            return;
        }
    }
    /* 블럭 충돌 */
    if(next_y-1 < BRICK_ROWS && next_y > 0) {
        int col = (next_x-1) / 2;
        if((next_x-1) % 2 == 0 && col < BRICK_COLS && bricks[next_y-1][col]) {
            bricks[next_y-1][col] = 0;
            score += 1;
            ball_dy = -ball_dy;
            next_y = ball_y + ball_dy;
        }
    }
    /* 위치 갱신 */
    ball_x = next_x;
    ball_y = next_y;
}

/* 메인 루프 */
int main(void) {
    set_conio_terminal_mode();
    init_game();
    while(1) {
        process_input();
        update_ball();
        draw_game();
        usleep(50000);  /* 약 20 FPS (50ms) */
    }
    return 0;
}
