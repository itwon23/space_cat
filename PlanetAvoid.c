
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#define WIDTH 80
#define HEIGHT 40
#define MAX_ASTEROIDS 20

// 실제 터미널 크기
int term_width = 80;
int term_height = 30;

// 난이도 설정 변수
int base_spawn = 25;
long base_speed_ns = 300000000;
int max_asteroids = MAX_ASTEROIDS;

// 시드 변수
unsigned int rand_seed;

// 고양이 스프라이트 (우주복 버전)
const char* cat_front[] = { " /\\_/\\ ", "{=O.O=}", ">[   ]<", "[_] [_]" };
const char* cat_left[]  = { " /\\_/\\ ", "{=O.O=}", "<[   ]<", "[_] [_]" };
const char* cat_right[] = { " /\\_/\\ ", "{=O.O=}", ">[   ]>", "[_] [_]" };

const int cat_height = 4;
const int cat_width  = 8;

// 상단 아트
const char* top_art[] = {
"             ⣀⡴⢧⣀   ⣀⣠⠤⠤⠤⠤⣄⣀",
"              ⠘⠏⢀⡴⠊⠁        ⠈⠙⠦⡀",
"              ⣰⠋               ⠘⢶⣶⣒⣶⠦⣤⣀",
"            ⢀⣰⠃                 ⠈⣟⠲⡌⠙⢦⠈⢧",
"         ⣠⢴⡾⢟⣿                   ⣸⡴⢃⡠⠋⣠⠋",
"       ⠞⣱⠋⢰⠁⢿                 ⣀⣠⠤⢖⣋⡥⢖⣫⠔⠋",
"       ⠹⢤⣈⣙⠚⠶⠤⠤⠤⠴⠶⣒⣒⣚⣩⠭⢵⣒⣻⠭⢖⠏⠁",
"          ⠈⠓⠒⠦⠭⠭⠭⣭⠭⠭⠭⠭⠿⠓⠒⠛⠉⠉    ⣠⠏",
"                 ⠈⠓⢤⣀       ⣀⡤⠞⠁   ⣰⣆",
"                    ⠈⠉⠙⠒⠒⠛⠉⠁      ⠉⢳⡞⠉"
};
const int top_art_height = sizeof(top_art)/sizeof(top_art[0]);

// 새로운 배경 아트들
const char* moon_art[] = {
    "   ,-.-.",
    " /.( +.\\",
    " \\ {. */",
    "  `-`-'"
};
const int moon_art_height = 4;

const char* planet_art[] = {
    "      ,",
    "   \\  :  /",
    "`. __/ \\__ .'",
    "_ _\\     /_ _",
    "   /_   _\\",
    " .'  \\ /  `.",
    "   /  :  \\",
    "      '"
};
const int planet_art_height = 8;

const char* star_cluster[] = {
    "                 '",
    "            *          .",
    "                   *       '",
    "              *                *",
    "   *   '*",
    "           *",
    "                *"
};
const int star_cluster_height = 7;

// 별자리 사이드 아트
const char* side_art_left[] = {
"             *     ",
"                *  ",
"   *               ",
"        *          ",
"  *                ",
"             .     ",
"    .              ",
"          *        ",
"                .  ",
" *                 ",
"        .          ",
"   *               ",
"             *     ",
"                   ",
"  .                ",
"        *          "
};

const char* side_art_right[] = {
"   *               ",
"        *          ",
"                 * ",
"   .               ",
"             *     ",
"  *                ",
"        .          ",
"                 . ",
"   *               ",
"             .     ",
"                 * ",
"        *          ",
"   .               ",
"                   ",
"             *     ",
"   .               "
};

const int side_art_height = 16;

// 게임 화면 오프셋
int game_offset_col = 0;

// 터미널 설정
struct termios orig_termios;
void reset_terminal() {
    printf("\033[?25h");
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void init_terminal() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(reset_terminal);
    struct termios new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    printf("\033[?25l");
    
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        term_width = w.ws_col;
        term_height = w.ws_row;
    }
    
    // 게임 화면 중앙 오프셋 계산
    game_offset_col = (term_width - WIDTH) / 2;
    if (game_offset_col < 0) game_offset_col = 0;
}

void sleep_ns(long nanoseconds) {
    struct timespec ts;
    ts.tv_sec = nanoseconds / 1000000000;
    ts.tv_nsec = nanoseconds % 1000000000;
    nanosleep(&ts, NULL);
}

double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

void print_center(const char* lines[], int count) {
    printf("\033[2J");
    fflush(stdout);
    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(lines[i]);
        if (len > max_len) max_len = len;
    }
    
    int start_row = (term_height - count) / 2;
    if (start_row < 1) start_row = 1;
    
    int start_col = (term_width - max_len) / 2;
    if (start_col < 1) start_col = 1;
    
    for (int i = 0; i < count; i++) {
        printf("\033[%d;%dH%s", start_row + i, start_col, lines[i]);
    }
    
    fflush(stdout);
}


// 장애물 타입
typedef struct { 
    int x; 
    int y; 
    char symbol;
    int is_big;
} Obstacle;
static Obstacle obstacles[MAX_ASTEROIDS];        
static int obstacle_count = 0;                   

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;  
static volatile int running = 1;                   

static int cat_x = WIDTH / 2;                      
static int cat_y = HEIGHT - cat_height - 1;       
static const char** current_cat = cat_front;       

static int score = 0;                              
static int survival_time = 0;                      
static double start_time;                          
static int high_score = 0;                         
static int target_score = 500;                   
static int last_oxygen_time = 0;                   
static int lives = 2;                              
static const char* HIGHSCORE_FILE = "cat_space_highscore.txt";  

// 리스크 상태
typedef enum { RISK_NONE, RISK_GRAVITY, RISK_DARKNESS, RISK_DARKNESS_WARNING, RISK_METEOR_STORM, RISK_OXYGEN_LEAK } RiskType;
static RiskType current_risk = RISK_NONE;
static double risk_end_time = 0;
static int cat_speed = 2;
static char risk_message[50] = "";
static int darkness_slow_mode = 0;
static int meteor_storm_mode = 0;

void load_high_score() {
    int fd = open(HIGHSCORE_FILE, O_RDONLY);
    if (fd >= 0) {
        char buf[32];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            high_score = atoi(buf);
        }
        close(fd);
    }
}

void save_high_score() {
    int fd = open(HIGHSCORE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%d\n", high_score);
        write(fd, buf, len);
        close(fd);
    }
}

void draw_top_art_once() {
    printf("\033[H");
    int start_col = (term_width - 60) / 2;
    if (start_col < 0) start_col = 0;
    
    for (int i = 0; i < top_art_height; i++) {
        printf("\033[%d;%dH%s", i + 1, start_col, top_art[i]);
    }
}

void draw_background_arts() {
    // 왼쪽 영역에 배경 아트 그리기
    int left_margin = game_offset_col - 22;
    if (left_margin > 5) {
        // 달 아트 (왼쪽 위)
        int moon_y = top_art_height + 2;
        for (int i = 0; i < moon_art_height; i++) {
            printf("\033[%d;%dH%s", moon_y + i, left_margin - 8, moon_art[i]);
        }
        
        // 행성 아트 (왼쪽 아래로 이동)
        int planet_y = HEIGHT - planet_art_height - 2;
        for (int i = 0; i < planet_art_height; i++) {
            printf("\033[%d;%dH%s", planet_y + i, left_margin - 10, planet_art[i]);
        }
    }
    
    // 오른쪽 영역에 배경 아트 그리기
    int right_margin = game_offset_col + WIDTH + 3;
    if (right_margin + 25 < term_width) {
        // 별 클러스터 (오른쪽 아래로 이동)
        int stars_y = HEIGHT - star_cluster_height - 2;
        for (int i = 0; i < star_cluster_height; i++) {
            printf("\033[%d;%dH%s", stars_y + i, right_margin + 5, star_cluster[i]);
        }
    }
}

int check_collision() {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < obstacle_count; i++) {
        int ax = obstacles[i].x;
        int ay = obstacles[i].y;
        
        int collision = 0;
        if (obstacles[i].is_big) {
            // 달 모양은 4x8 크기
            for (int dy = 0; dy < 4; dy++) {
                for (int dx = 0; dx < 8; dx++) {
                    int check_y = ay + dy;
                    int check_x = ax + dx;
                    if (check_y >= cat_y && check_y < cat_y + cat_height &&
                        check_x >= cat_x && check_x < cat_x + cat_width) {
                        collision = 1;
                        break;
                    }
                }
                if (collision) break;
            }
        } else {
            if (ay >= cat_y && ay < cat_y + cat_height &&
                ax >= cat_x && ax < cat_x + cat_width) {
                collision = 1;
            }
        }
        
        if (collision) {
            if (lives > 1) {
                lives--;
                for (int j = i; j < obstacle_count - 1; j++)
                    obstacles[j] = obstacles[j + 1];
                obstacle_count--;
                pthread_mutex_unlock(&lock);
                return 0;
            }
            pthread_mutex_unlock(&lock);
            return 1;
        }
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

void draw_screen() {
    // 배경 아트 그리기
    draw_background_arts();
    
    // 양옆 별자리 그리기
    for (int i = 0; i < side_art_height && (top_art_height + i) < HEIGHT; i++) {
        if (game_offset_col > 25) {
            printf("\033[%d;%dH%s", top_art_height + i + 1, game_offset_col - 22, side_art_left[i]);
        }
        if (game_offset_col + WIDTH + 3 < term_width) {
            printf("\033[%d;%dH%s", top_art_height + i + 1, game_offset_col + WIDTH + 3, side_art_right[i]);
        }
    }
    
    char screen[HEIGHT][WIDTH];
    
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if ((i + j + survival_time) % 137 == 0) {
                screen[i][j] = '.';
            } else if ((i * 3 + j * 7 + survival_time) % 197 == 0) {
                screen[i][j] = '*';
            } else {
                screen[i][j] = ' ';
            }
        }
    }

    for (int i = top_art_height; i < HEIGHT; i++) {
        screen[i][0] = '|';
        screen[i][WIDTH - 1] = '|';
    }

    pthread_mutex_lock(&lock);
    
    double now = get_time_seconds();
    survival_time = (int)(now - start_time);
    
    if (survival_time / 10 > last_oxygen_time / 10) {
        last_oxygen_time = survival_time;
    }
    
    if (current_risk == RISK_DARKNESS_WARNING && now >= risk_end_time) {
        current_risk = RISK_DARKNESS;
        risk_end_time = now + 2.0;
        darkness_slow_mode = 1;
        strcpy(risk_message, "경고! 우주 암흑!");
    }
    else if (current_risk != RISK_NONE && current_risk != RISK_DARKNESS_WARNING && now >= risk_end_time) {
        current_risk = RISK_NONE;
        cat_speed = 2;
        darkness_slow_mode = 0;
        meteor_storm_mode = 0;
        risk_message[0] = '\0';
    }
    
    if (current_risk != RISK_DARKNESS) {
        for (int i = 0; i < obstacle_count; i++) {
            int x = obstacles[i].x;
            int y = obstacles[i].y;
            
            if (obstacles[i].is_big) {
                // 달 모양 운석 그리기
                const char* big_asteroid[] = {
                    "   ,-.-.",
                    " /.( +.\\",
                    " \\ {. */",
                    "  `-`-'"
                };
                for (int dy = 0; dy < 4; dy++) {
                    for (int dx = 0; dx < 8 && big_asteroid[dy][dx] != '\0'; dx++) {
                        int draw_y = y + dy;
                        int draw_x = x + dx;
                        if (draw_y >= top_art_height && draw_y < HEIGHT && 
                            draw_x > 0 && draw_x < WIDTH - 1) {
                            screen[draw_y][draw_x] = big_asteroid[dy][dx];
                        }
                    }
                }
            } else {
                if (y >= top_art_height && y < HEIGHT && x > 0 && x < WIDTH - 1) {
                    screen[y][x] = obstacles[i].symbol;
                }
            }
        }
    }
    
    score = survival_time;
    
    pthread_mutex_unlock(&lock);

    for (int i = 0; i < cat_height; i++) {
        for (int j = 0; j < cat_width && current_cat[i][j] != '\0'; j++) {
            int gx = cat_x + j;
            int gy = cat_y + i;
            if (gy >= top_art_height && gy < HEIGHT && gx > 0 && gx < WIDTH - 1)
                screen[gy][gx] = current_cat[i][j];
        }
    }

    for (int i = top_art_height; i < HEIGHT; i++) {
        printf("\033[%d;%dH", i + 1, game_offset_col + 1);
        for (int j = 0; j < WIDTH; j++)
            putchar(screen[i][j]);
    }

    int oxygen = survival_time / 10;
    printf("\033[%d;%dH우주 체류: %d초 | 점수: %d/%d | 우주 산소통: %d개 | 목숨: %d", 
           HEIGHT + 1, game_offset_col + 1, survival_time, score, target_score, oxygen, lives);
    
    if (risk_message[0] != '\0') {
        printf(" | %s", risk_message);
    }
    fflush(stdout);
}

void* asteroid_thread(void* arg) {
    (void)arg;
    unsigned int local_seed = rand_seed + 1;
    
    char obstacle_types[] = {'*', 'O', '#', '@', 'X'};
    double last_big_spawn = 0;
    
    while (running) {
        double elapsed = get_time_seconds() - start_time;
        int spawn_percent = base_spawn + (int)elapsed / 3;
        if (spawn_percent > 60) spawn_percent = 60;
        
        long sleep_duration = base_speed_ns - (long)(elapsed * 5000000);
        if (sleep_duration < 90000000) sleep_duration = 90000000;
        
        pthread_mutex_lock(&lock);
        if (darkness_slow_mode) {
            sleep_duration = sleep_duration * 2;
        }
        if (meteor_storm_mode) {
            sleep_duration = sleep_duration / 2;
        }
        pthread_mutex_unlock(&lock);

        pthread_mutex_lock(&lock);
        double now = get_time_seconds();
        
        if (obstacle_count < max_asteroids - 1 && (now - last_big_spawn) > 15.0) {
            if (rand_r(&local_seed) % 100 < 30) {
                obstacles[obstacle_count].x = rand_r(&local_seed) % (WIDTH - 10) + 2;
                obstacles[obstacle_count].y = top_art_height;
                obstacles[obstacle_count].is_big = 1;
                obstacles[obstacle_count].symbol = 'M';
                obstacle_count++;
                last_big_spawn = now;
            }
        }
        
        if (obstacle_count < max_asteroids) {
            if (rand_r(&local_seed) % 100 < spawn_percent) {
                obstacles[obstacle_count].x = rand_r(&local_seed) % (WIDTH - 2) + 1;
                obstacles[obstacle_count].y = top_art_height;
                obstacles[obstacle_count].is_big = 0;
                obstacles[obstacle_count].symbol = obstacle_types[rand_r(&local_seed) % 5];
                obstacle_count++;
            }
        }
        for (int i = 0; i < obstacle_count; i++)
            obstacles[i].y++;
        for (int i = 0; i < obstacle_count; i++) {
            if (obstacles[i].y >= HEIGHT) {
                for (int j = i; j < obstacle_count - 1; j++)
                    obstacles[j] = obstacles[j + 1];
                obstacle_count--;
                i--;
            }
        }
        pthread_mutex_unlock(&lock);
        sleep_ns(sleep_duration);
    }
    return NULL;
}

void* risk_thread(void* arg) {
    (void)arg;
    unsigned int local_seed = rand_seed + 2;
    
    while (running) {
        int wait_time = 10 + (rand_r(&local_seed) % 11);
        sleep_ns(wait_time * 1000000000L);
        
        if (!running) break;
        
        pthread_mutex_lock(&lock);
        if (current_risk == RISK_NONE) {
            double now = get_time_seconds();
            int risk_type = rand_r(&local_seed) % 4;
            
            if (risk_type == 0) {
                current_risk = RISK_GRAVITY;
                cat_speed = 1;
                risk_end_time = now + 5.0;
                strcpy(risk_message, "중력 증가!");
            } else if (risk_type == 1) {
                current_risk = RISK_DARKNESS_WARNING;
                risk_end_time = now + 1.0;
                strcpy(risk_message, "암흑 경고!");
            } else if (risk_type == 2) {
                current_risk = RISK_METEOR_STORM;
                meteor_storm_mode = 1;
                risk_end_time = now + 4.0;
                strcpy(risk_message, "운석 폭풍!");
            } else {
                current_risk = RISK_OXYGEN_LEAK;
                cat_speed = 1;
                risk_end_time = now + 5.0;
                strcpy(risk_message, "산소 누출!");
            }
        }
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

static void* input_thread(void* arg) {
    (void)arg;
    fd_set readfds;
    struct timeval tv;
    
    while (running) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        
        int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0) {
            int c = getchar();
            if (c == 'a' || c == 'A') {
                pthread_mutex_lock(&lock);
                cat_x -= cat_speed;
                if (cat_x < 1) cat_x = 1;
                current_cat = cat_left;
                pthread_mutex_unlock(&lock);
            } else if (c == 'd' || c == 'D') {
                pthread_mutex_lock(&lock);
                cat_x += cat_speed;
                if (cat_x > WIDTH - cat_width - 1)
                    cat_x = WIDTH - cat_width - 1;
                current_cat = cat_right;
                pthread_mutex_unlock(&lock);
            } else if (c == 'q' || c == 'Q') {
                running = 0;
            } else {
                pthread_mutex_lock(&lock);
                current_cat = cat_front;
                pthread_mutex_unlock(&lock);
            }
        }
    }
    return NULL;
}

int run_game() {
    obstacle_count = 0;
    cat_x = WIDTH / 2 - cat_width / 2;
    cat_y = HEIGHT - cat_height - 1;
    current_cat = cat_front;
    score = 0;
    survival_time = 0;
    last_oxygen_time = 0;
    start_time = get_time_seconds();
    running = 1;
    current_risk = RISK_NONE;
    cat_speed = 2;
    darkness_slow_mode = 0;
    meteor_storm_mode = 0;
    risk_message[0] = '\0';

    printf("\033[2J");
    draw_top_art_once();
    pthread_t tid_asteroid, tid_input, tid_risk;
    pthread_create(&tid_asteroid, NULL, asteroid_thread, NULL);
    pthread_create(&tid_input, NULL, input_thread, NULL);
    pthread_create(&tid_risk, NULL, risk_thread, NULL);

    int collided = 0;
    int success = 0;
    
    while (running) {
        draw_screen();
        
        if (score >= target_score) {
            success = 1;
            running = 0;
            break;
        }
        
        if (check_collision()) {
            collided = 1;
            running = 0;
            break;
        }
        sleep_ns(100000000);
    }

    pthread_join(tid_input, NULL);
    pthread_join(tid_asteroid, NULL);
    pthread_join(tid_risk, NULL);
    
    running = 0;
    usleep(100000);
    printf("\033[2J\033[H");  // 즉시 화면 클리어
    fflush(stdout);
    return success ? 2 : (collided ? 1 : 0);
}

int planet_avoid_game() {
    init_terminal();
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    rand_seed = ts.tv_nsec;
    
    load_high_score();

    const char* rules[] = {
        "+==========================================+",
        "|      우주 고양이 생존 게임               |",
        "+==========================================+",
        "|                                          |",
        "|  [조작법]                                |",
        "|   'A' 키 : 왼쪽으로 이동                 |",
        "|   'D' 키 : 오른쪽으로 이동               |",
        "|   'Q' 키 : 게임 종료                     |",
        "|                                          |",
        "|  [목표]                                  |",
        "|   우주 장애물을 피하세요!                |",
        "|   *=소행성 O=파편 #=위성 @=혜성 X=운석   |",
        "|   가끔 큰 달 운석이 나타납니다!          |",
        "|                                          |",
        "+==========================================+",
        "|        난이도: [1]하 [2]중 [3]상         |",
        "|       1[3]번 누르고 엔터 게임시작        |",
        "|       2[3]번 누르고 엔터 게임시작        |",
        "|       3[3]번 누르고 엔터 게임시작        |",
        "+==========================================+"
    };
print_center(rules, 20);




int tmp;
while ((tmp = getchar()) != '\n' && tmp != EOF);



system("stty raw -echo");
int c = getchar();
system("stty cooked echo");

// 남은 입력 제거
const char* message;
if (c == '1') {
    base_spawn = 20; base_speed_ns = 350000000; max_asteroids = 15;
    target_score = 50;
    lives = 3;
    message = "|  [ 하 ] 목표: 50점 (목숨 3개)            |";
} else if (c == '2') {
    base_spawn = 25; base_speed_ns = 300000000; max_asteroids = 20;
    target_score = 80;
    lives = 2;
    message = "|  [ 중 ] 목표: 80점 (목숨 2개)            |";
} else {
    base_spawn = 35; base_speed_ns = 250000000; max_asteroids = 25;
    target_score = 100;
    lives = 1;
    message = "|  [ 상 ] 목표: 100점 (목숨 1개)           |";
}



    const char* msg_block[] = { 
        "+==========================================+",
        "|                                          |",
        message,
        "|                                          |",
        "+==========================================+"
    };
    print_center(msg_block, 5);
    sleep_ns(2000000000);

    int result = run_game();
    printf("\033[2J\033[H");

    int oxygen = survival_time / 10;

    char line1[100], line2[100], line3[100];

	if (result == 2) {
    		strcpy(line1, "|  목표 달성! 성공!                       |");
	} else if (result == 1) {
    		strcpy(line1, "|  충돌!                                   |");
	} else {
	    strcpy(line1, "|  종료                                    |");
	}

	snprintf(line2, sizeof(line2), "|  최종 점수  %3d점                        |", score);
	snprintf(line3, sizeof(line3), "|  우주 산소통  %3d개                      |", oxygen);	 

const char* end_lines[] = {
    "+==========================================+",
    "|                                          |",
    "|              게임 종료                   |",
    "|                                          |",
    "+==========================================+",
    line1,
    line2,
    line3,
    "+==========================================+",
    "|                                          |",
    "|      'O' 키 눌러 엔터 2번                |",
    "|                                          |",
    "+==========================================+"
};
print_center(end_lines, 13);
if (score > high_score) {
    high_score = score;
    save_high_score();
}
fflush(stdout);

// raw 모드
system("stty raw -echo");
int key = getchar();
system("stty cooked echo");

printf("\033[2J\033[H");
return 0;
}
