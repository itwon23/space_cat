#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>

#define MAX_NOTES 300
#define LANES 4
#define GAMETIME 10

float speed = 15.0;
float travel_time = 1.0;

#define JUDGE_LINE_Y 25
#define WORDLINE 20

int lane_x[LANES] = { 5, 15, 25, 35 };

typedef struct {
    float time;
} ChartNote;

ChartNote chart[MAX_NOTES];
int chart_count = 0;

typedef struct {
    float spawn_time;
    float y;
    int lane;
    int active;
} Note;

Note notes[MAX_NOTES];
int note_count = 0;

int score = 0;
char* reward;

float get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void load_chart(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("Chart load failed: %s\n", filename);
        exit(1);
    }

    while (fscanf(fp, "%f", &chart[chart_count].time) == 1) {
        if (chart_count >= MAX_NOTES) break;
        chart_count++;
    }

    fclose(fp);
}

void spawn_note(float current_time) {
    int lane = rand() % LANES;

    notes[note_count].active = 1;
    notes[note_count].lane = lane;
    notes[note_count].spawn_time = current_time;
    notes[note_count].y = 0;

    note_count++;
}

void draw_notes(float current_time) {
    for (int i = 0; i < note_count; i++) {
        if (!notes[i].active) continue;

        notes[i].y = (current_time - notes[i].spawn_time) * speed;

        if (notes[i].y > LINES) {
            notes[i].active = 0;
            continue;
        }

        mvprintw((int)notes[i].y, lane_x[notes[i].lane]-3, "=^._.^=");
    }
}

void draw_lane_separators() {
    int sep[3] = { 10, 20, 30 };

    for (int y = 0; y < LINES; y++) {
        mvprintw(y, sep[0], "|");
        mvprintw(y, sep[1], "|");
        mvprintw(y, sep[2], "|");
    }
}

void judge(int lane) {
    for (int i = 0; i < note_count; i++) {
        if (!notes[i].active) continue;
        if (notes[i].lane != lane) continue;

        float diff = fabs(notes[i].y - JUDGE_LINE_Y);

        //퍼펙트 +2점
        // if (diff < 0.5) {
        //     score += 2;
        //     notes[i].active = 0;
        //     return;
        // }
        //굿 +1점

        if (diff < 1.5) {
            score += 1;
            notes[i].active = 0;
            return;
        }
    }
}

void handle_input() {
    int ch = getch();

    switch (ch) {
        case 'a': judge(0); break;
        case 'w': judge(1); break;
        case 'e': judge(2); break;
        case 'f': judge(3); break;
    }
}

int main() {

    setlocale(LC_ALL, "");

    score = 0;
    note_count = 0;
    chart_count = 0;

    initscr();
    noecho();
    curs_set(FALSE);
    timeout(-1);

    clear();
    // 게임 설명
    mvprintw(LINES/2 - 5, (COLS-20)/2, "==== 리듬 게임 설명 ====");
    mvprintw(LINES/2 - 2, (COLS-40)/2, "A / W / E / F 키로 대응되는 노트를 입력하세요.");
    mvprintw(LINES/2 ,     (COLS-40)/2, "고양이가 판정선에 닿을 때 입력하면 점수가 올라갑니다.");
    mvprintw(LINES/2 +2,     (COLS-40)/2, "점수에 따라 보상의 개수가 달라집니다.");
    mvprintw(LINES/2 + 5, (COLS-20)/2, "아무 키나 눌러 계속...");
    refresh();

    getch();

    while (1) {
        //난이도 선택
        clear();
        mvprintw(LINES/2 - 5, (COLS-20)/2, "==== 난이도 선택 ====");
        mvprintw(LINES/2 - 3, (COLS-26)/2, "난이도 별 보상이 상이합니다.");
        mvprintw(LINES/2 - 1, (COLS-20)/2, "1. Easy (츄르)");
        mvprintw(LINES/2 , (COLS-20)/2, "2. Normal (사료)");
        mvprintw(LINES/2 + 1, (COLS-20)/2, "3. Hard (연어)");
        mvprintw(LINES/2 + 3, (COLS-30)/2, "난이도를 선택하세요 (1/2/3)");

        refresh();

        int ch = getch();
        if (ch == '1') { speed = 10.0; reward = "츄르"; break; }
        if (ch == '2') { speed = 15.0; reward = "사료"; break; }
        if (ch == '3') { speed = 20.0; reward = "연어"; break; }
    }

    timeout(1);

    load_chart("chart.txt");

    system("mpg123 pj2.mp3 >/dev/null 2>&1 </dev/null &");

    float start_time = get_time_sec();
    int chart_index = 0;

    while (1) {
        float current_time = get_time_sec() - start_time;

        if (current_time >= GAMETIME) break;

        clear();

        mvprintw(JUDGE_LINE_Y, 0, "-----------------------------------------");

        draw_lane_separators();

        mvprintw(1, 45, "Time: %.2f / %.2d   Score: %d", current_time, GAMETIME, score);

        mvprintw(JUDGE_LINE_Y+2, lane_x[0], "A");
        mvprintw(JUDGE_LINE_Y+2, lane_x[1], "W");
        mvprintw(JUDGE_LINE_Y+2, lane_x[2], "E");
        mvprintw(JUDGE_LINE_Y+2, lane_x[3], "F");

        if (chart_index < chart_count &&
            current_time >= chart[chart_index].time - travel_time)
        {
            spawn_note(current_time);
            chart_index++;
        }

        draw_notes(current_time);
        handle_input();

        refresh();
        usleep(16000);
    }

    clear();

    // 음악 중지
    system("killall mpg123");

    // 음악 종료 멘트 지우기용
    clear();

    char end1[] = "=== 미니게임 종료 ===";
    char end2[50];
    sprintf(end2, "최종 점수: %d", score);
    char end3[] = "'o' 키를 누르면 종료합니다";

    int cy = LINES / 2;   // 중앙 Y
    int cx1 = (COLS - strlen(end1)) / 2;
    int cx2 = (COLS - strlen(end2)) / 2;
    int cx3 = (COLS - strlen(end3)) / 2;

    mvprintw(cy - 3, cx1, "%s", end1);
    mvprintw(cy-1, cx2, "%s", end2);
    mvprintw(cy + 1, cx3, "[%s]를 총 %d개를 획득하셨습니다.", reward ,score/5);
    mvprintw(cy + 4, cx3, "%s", end3);

    refresh();

    timeout(-1);
    int ch;
    while (1) {
        ch = getch();
        if (ch == 'o') break;
    }

    endwin();
    return 0;
}
