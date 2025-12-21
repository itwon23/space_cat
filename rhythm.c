#include "rhythm.h"
#include <locale.h>
#include <ncursesw/ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHART_PATH   "chart.txt"
#define GAMETIME 30
#define MAX_NOTES 300
#define LANES 4

static float speed;
static float travel_time = 1.0;

#define JUDGE_LINE_Y 25

static int lane_x[LANES] = { 5, 15, 25, 35 };

typedef struct {
    float time;
} ChartNote;

typedef struct {
    float spawn_time;
    float y;
    int lane;
    int active;
} Note;

// static Item make_item(const char* name, int recovery) {
//     Item it;
//     memset(&it, 0, sizeof(it));
//     strncpy(it.name, name, sizeof(it.name) - 1);
//     it.recovery = recovery;
//     return it;
// }

static float get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (float)ts.tv_sec + (float)ts.tv_nsec * 1e-9f;
}

static int load_chart(const char* filename, ChartNote* chart, int* chart_count) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return 0;

    *chart_count = 0;
    while (fscanf(fp, "%f", &chart[*chart_count].time) == 1) {
        (*chart_count)++;
        if (*chart_count >= MAX_NOTES) break;
    }

    fclose(fp);
    return 1;
}

static void spawn_note(Note* notes, int* note_count, float current_time) {
    if (*note_count >= MAX_NOTES) return;

    int lane = rand() % LANES;

    notes[*note_count].active = 1;
    notes[*note_count].lane = lane;
    notes[*note_count].spawn_time = current_time;
    notes[*note_count].y = 0;

    (*note_count)++;
}

static void draw_notes(Note* notes, int note_count, float current_time) {
    for (int i = 0; i < note_count; i++) {
        if (!notes[i].active) continue;

        notes[i].y = (current_time - notes[i].spawn_time) * speed;

        if ((int)notes[i].y >= LINES) {
            notes[i].active = 0;
            continue;
        }

        mvprintw((int)notes[i].y, lane_x[notes[i].lane] - 3, "=^._.^=");
    }
}

static void draw_lane_separators(void) {
    int sep[3] = { 10, 20, 30 };
    for (int y = 0; y < LINES; y++) {
        mvprintw(y, sep[0], "|");
        mvprintw(y, sep[1], "|");
        mvprintw(y, sep[2], "|");
    }
}

static void judge(Note* notes, int note_count, int lane, int* score) {
    for (int i = 0; i < note_count; i++) {
        if (!notes[i].active) continue;
        if (notes[i].lane != lane) continue;

        float diff = fabsf(notes[i].y - (float)JUDGE_LINE_Y);

        if (diff < 1.5) {
            (*score) += 1;
            notes[i].active = 0;
            return;
        }
    }
}

static void handle_input(Note* notes, int note_count, int* score) {
    int ch = getch();
    switch (ch) {
        case 'a': judge(notes, note_count, 0, score); break;
        case 'w': judge(notes, note_count, 1, score); break;
        case 'e': judge(notes, note_count, 2, score); break;
        case 'f': judge(notes, note_count, 3, score); break;
        default: break;
    }
}

static void play_random_bgm(void) {
    const char *tracks[] = {
        "music/song1.mp3",
        "music/song2.mp3"
    };
    const int n = (int)(sizeof(tracks) / sizeof(tracks[0]));

    int idx = rand() % n;

    char cmd[512];

    snprintf(cmd, sizeof(cmd),
             "mpg123 -q --loop -1 '%s' >/dev/null 2>&1 &",
             tracks[idx]);

    system(cmd);
}

//리듬 게임 함수
RhythmGameResult rhythm_game(void) {
    RhythmGameResult out = {{0}, 0};
    out.reward = make_item("캔푸드", 20);  // 기본값

    setlocale(LC_ALL, "");

    int score = 0;
    int note_count = 0;
    int chart_count = 0;

    ChartNote chart[MAX_NOTES];
    Note notes[MAX_NOTES];

    memset(chart, 0, sizeof(chart));
    memset(notes, 0, sizeof(notes));

    /* ncurses 시작 */
    initscr();
    noecho();
    curs_set(FALSE);
    timeout(-1);

    // 게임 설명
    clear();
    mvprintw(LINES/2 - 5, (COLS-20)/2, "==== 리듬 게임 설명 ====");
    mvprintw(LINES/2 - 2, (COLS-40)/2, "A / W / E / F 키로 대응되는 노트를 입력하세요.");
    mvprintw(LINES/2,     (COLS-40)/2, "고양이가 판정선에 닿을 때 입력하면 점수가 올라갑니다.");
    mvprintw(LINES/2 + 2, (COLS-40)/2, "점수에 따라 보상의 개수가 달라집니다.");
    mvprintw(LINES/2 + 5, (COLS-20)/2, "아무 키나 눌러 계속...");
    refresh();
    getch();

    // 난이도 선택
    while (1) {
        clear();
        mvprintw(LINES/2 - 5, (COLS-20)/2, "==== 난이도 선택 ====");
        mvprintw(LINES/2 - 3, (COLS-26)/2, "난이도 별 보상이 상이합니다.");
        mvprintw(LINES/2 - 1, (COLS-20)/2, "1. Easy [생선스낵 (+10)]");
        mvprintw(LINES/2 ,    (COLS-20)/2, "2. Normal [캔푸드 (+20)]");
        mvprintw(LINES/2 + 1, (COLS-20)/2, "3. Hard [우주츄르 (+30)]");
        mvprintw(LINES/2 + 3, (COLS-30)/2, "난이도를 선택하세요 (1/2/3)");
        refresh();

        int ch = getch();
        if (ch == '1') { speed = 15.0; out.reward = make_item("생선스낵", 10); break; }
        if (ch == '2') { speed = 23.0; out.reward = make_item("캔푸드", 20); break; }
        if (ch == '3') { speed = 30.0; out.reward = make_item("우주츄르", 30); break; }


    }

    timeout(1);

    //차트 로드 실패 시
    if (!load_chart(CHART_PATH, chart, &chart_count)) {
        clear();
        mvprintw(LINES/2, (COLS-40)/2, "Chart load failed: %s", CHART_PATH ? CHART_PATH : "(null)");
        mvprintw(LINES/2 + 2, (COLS-30)/2, "아무 키나 누르면 종료합니다.");
        refresh();
        timeout(-1);
        getch();
        endwin();
        return out;
    }

    srand((unsigned)time(NULL));

    play_random_bgm();

    float start_time = get_time_sec();
    int chart_index = 0;

    while (1) {
        float current_time = get_time_sec() - start_time;
        if (current_time >= (float)GAMETIME) break;

        clear();

        mvprintw(JUDGE_LINE_Y, 0, "-----------------------------------------");
        draw_lane_separators();

        mvprintw(1, 45, "Time: %.2f / %.2d   Score: %d", current_time, GAMETIME, score);

        mvprintw(JUDGE_LINE_Y + 2, lane_x[0], "A");
        mvprintw(JUDGE_LINE_Y + 2, lane_x[1], "W");
        mvprintw(JUDGE_LINE_Y + 2, lane_x[2], "E");
        mvprintw(JUDGE_LINE_Y + 2, lane_x[3], "F");

        if (chart_index < chart_count &&
            current_time >= chart[chart_index].time - travel_time)
        {
            spawn_note(notes, &note_count, current_time);
            chart_index++;
        }

        draw_notes(notes, note_count, current_time);
        handle_input(notes, note_count, &score);

        refresh();
        usleep(16000);
    }

    system("killall mpg123 >/dev/null 2>&1");

    clear();

    out.reward_count = score / 7;

    char end1[] = "=== 미니게임 종료 ===";
    char end2[64];
    snprintf(end2, sizeof(end2), "최종 점수: %d", score);
    char end3[] = "'o' 키를 누르면 종료합니다";

    int cy = LINES / 2;
    int cx1 = (COLS - (int)strlen(end1)) / 2;
    int cx2 = (COLS - (int)strlen(end2)) / 2;
    int cx3 = (COLS - (int)strlen(end3)) / 2;

    mvprintw(cy - 3, cx1, "%s", end1);
    mvprintw(cy - 1, cx2, "%s", end2);
    mvprintw(cy + 1, cx3, "[%s]를 총 %d개를 획득하셨습니다.", out.reward.name, out.reward_count);

    mvprintw(cy + 4, cx3, "%s", end3);

    refresh();

    timeout(-1);
    while (1) {
        int ch = getch();
        if (ch == 'o' || ch == 'O') break;
    }

    endwin();
    return out;
}