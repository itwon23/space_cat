    #include <stdio.h>
    #include <stdlib.h>
    #include <pthread.h>
    #include <unistd.h>
    #include <string.h>
    #include <time.h>

    #include "Inventory.h"
    #include "rhythm.h"
    typedef struct {
        int hunger;
        int oxygen;
        int mood;
    } CatStatus;

    CatStatus status;
    Inventory inv;
    pthread_mutex_t lock;
    int running = 1;

    int dialogue_triggered_20 = 0;
    int dialogue_triggered_50 = 0;
    int dialogue_triggered_70 = 0;
    int dialogue_triggered_90 = 0;


    volatile int user_choice = -1;
    volatile int item_choice = -1;  
    char last_event[256] = "";
    float repair_progress = 0.0;  

    static pthread_t th_input;
    static int input_thread_alive = 0;


    void print_repair_bar(int filled, int total) {
        printf("[");
        for (int i = 0; i < total; i++) {
            printf("%s", (i < filled) ? "■" : "□");
        }
        printf("]\n");
    }



    void trigger_random_event() {
        int r = rand() % 100;

        if (r < 10) {
            strcpy(last_event, "🌕 운석 조각을 발견했다! 기분 +5!\n"
                            "\"우와! 내가 좋아하는 운석 조각이야. 갖고 있으면 행운이 온대.\"");
            status.mood += 10;
        }
        else if (r < 25) {
            strcpy(last_event, "😾 우주 벌레에게 물렸다! 기분 -7!\n"
                            "\"앗 간지러워! 신경쓰여서 작업을 못하겠네..\"");
            status.mood -= 7;
        }
        else if (r < 35) {
            strcpy(last_event, "🍀 우주 고양이가 선물을 줬다! 생선스낵 +4!\n"
                            "\"이것봐라~ 저쪽에서 찾아왔어. 맛있겠다 그치!\"");
            if (inv.food_count < 20) {
                strcpy(inv.food[inv.food_count].name, "생선스낵");
                inv.food[inv.food_count].recovery = 10;
                inv.food_count++;
            }
        }
        else if (r < 50) {
            strcpy(last_event, "🛠️ 산소 필터 고장! 산소 -2!\n"
                            "\"앗, 내 실수.. 금방 고칠 수 있어\"");
            status.oxygen -= 2;
        }

        else {
            strcpy(last_event, "");
        }
    }

    void* decrease_status(void* arg) {
        while (running) {
            sleep(5);

            pthread_mutex_lock(&lock);
            status.hunger -= rand() % 2 + 1;
            status.oxygen -= rand() % 2  + 1;
            status.mood   -= rand() % 2 + 1;

            trigger_random_event();

            if (status.hunger < 0) status.hunger = 0;
            if (status.oxygen < 0) status.oxygen = 0;
            if (status.mood < 0) status.mood = 0;
            pthread_mutex_unlock(&lock);
        }
        return NULL;
    }


    void* input_thread(void* arg) {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

        while (running) {

            int input;
            if (scanf("%d", &input) == 1) {
                if (user_choice == -1) {
                    user_choice = input;
                } else {
                    item_choice = input;
                }
            } else {
                while(getchar() != '\n');
            }
        }
        return NULL;
    }

    
    void print_status_bar(int value) {
        int bars = value / 2;
        for (int i = 0; i < 50; i++)
            printf("%c", i < bars ? '|' : ' ');
    }
    const char* get_color(int value);


    void show_status() {
        system("clear");

        pthread_mutex_lock(&lock);
        int bar = (int)(repair_progress / 2);  

        printf("=== 우주선 수리 진행도 ===\n");
        printf("[");
        for (int i = 0; i < 50; i++)
            printf("%s", (i < bar) ? "■" : "□");
        printf("] %.1f%%\n\n", repair_progress);

        pthread_mutex_unlock(&lock);

        printf(" ,-.       _,---._ __  / \\\n");
        printf("/  )    .-'       `./ /   \\\n");
        printf("(  (   ,'            `/    /|\n");
        printf(" \\  `-\"             '\\'\\   / |\n");
        printf("  `.              ,  \\ \\ /  |\n");
        printf("   /`.          ,'-`----Y   |\n");
        printf("  (            ;        |   '\n");
        printf("  |  ,-.    ,-'         |  /\n");
        printf("  |  | (   |            | /\n");
        printf("  )  |  \\  `.___________|/\n");
        printf("  `--'   `--'\n\n");

        printf("=== SPACE CAT STATUS ===\n");

        pthread_mutex_lock(&lock);

        printf("허기: %s[", get_color(status.hunger));
        print_status_bar(status.hunger);
        printf("] %3d\033[0m\n", status.hunger);

        printf("산소: %s[", get_color(status.oxygen));
        print_status_bar(status.oxygen);
        printf("] %3d\033[0m\n", status.oxygen);

        printf("기분: %s[", get_color(status.mood));
        print_status_bar(status.mood);
        printf("] %3d\033[0m\n\n", status.mood);

        if (strlen(last_event) > 0) {
            printf("\n[EVENT] %s\n\n", last_event);
        }

        printf("=====================================================\n");
        printf("선택지: 1) 먹이  2) 산소  3) 게임기  0) 종료\n");
        printf("=====================================================\n");
        printf("선택: ");
        printf("\n");

        fflush(stdout);

        
    pthread_mutex_unlock(&lock);

    printf("\n");

    }


    const char* get_color(int value) {
        if (value >= 80) return "\033[92m";  // 초록
        if (value >= 50) return "\033[93m";  // 주황
        if (value >= 30) return "\033[91m";  // 빨강
        return "\033[95m"; 
    }



    void sort_food() {
        for (int i = 0; i < inv.food_count - 1; i++) {
            for (int j = i + 1; j < inv.food_count; j++) {
                if (inv.food[j].recovery > inv.food[i].recovery) {
                    Item tmp = inv.food[i];
                    inv.food[i] = inv.food[j];
                    inv.food[j] = tmp;
                }
            }
        }
    }

    void sort_oxygen() {
        for (int i = 0; i < inv.oxygen_count - 1; i++) {
            for (int j = i + 1; j < inv.oxygen_count; j++) {
                if (inv.oxygen[j].recovery > inv.oxygen[i].recovery) {
                    Item tmp = inv.oxygen[i];
                    inv.oxygen[i] = inv.oxygen[j];
                    inv.oxygen[j] = tmp;
                }
            }
        }
    }

    void use_food() {
        if (inv.food_count == 0) {
            printf("\n사용할 음식이 없습니다!\n");
            sleep(1);
            return;
        }

        sort_food();

        printf("\n=== FOOD INVENTORY ===\n");
            printf("0) 뒤로가기\n");

        for (int i = 0; i < inv.food_count; i++)
            printf("%d) %s (+%d)\n", i+1, inv.food[i].name, inv.food[i].recovery);

        printf("사용할 음식 번호: ");
        fflush(stdout);

        while (item_choice == -1 && running) {
            usleep(100 * 1000);
        }

        int n = item_choice;
        item_choice = -1;

        if (n == 0) {
            printf("뒤로 갑니다...\n");
            sleep(1);
            return;
        }

        if (n < 1 || n > inv.food_count) {
            printf("잘못된 선택!\n");
            sleep(1);
            return;
        }

        pthread_mutex_lock(&lock);
        status.hunger += inv.food[n-1].recovery;
        if (status.hunger > 100) status.hunger = 100;
        pthread_mutex_unlock(&lock);

        printf("%s 사용!\n", inv.food[n-1].name);

        for (int i = n-1; i < inv.food_count - 1; i++) {
            inv.food[i] = inv.food[i+1];
        }
        inv.food_count--;

        sleep(1);
    }

    void use_oxygen() {
        if (inv.oxygen_count == 0) {
            printf("\n사용할 산소가 없습니다!\n");
            sleep(1);
            return;
        }

        sort_oxygen();

        printf("\n=== OXYGEN INVENTORY ===\n");
                printf("0) 뒤로가기\n");

        for (int i = 0; i < inv.oxygen_count; i++)
            printf("%d) %s (+%d)\n", i+1, inv.oxygen[i].name, inv.oxygen[i].recovery);

        printf("사용할 산소 번호: ");
        fflush(stdout);

        while (item_choice == -1 && running) {
            usleep(100 * 1000);
        }

        int n = item_choice;
        item_choice = -1;

        if (n == 0) {
            printf("뒤로 갑니다...\n");
            sleep(1);
            return;
        }


        if (n < 1 || n > inv.oxygen_count) {
            printf("잘못된 선택!\n");
            sleep(1);
            return;
        }

        pthread_mutex_lock(&lock);
        status.oxygen += inv.oxygen[n-1].recovery;
        if (status.oxygen > 100) status.oxygen = 100;
        pthread_mutex_unlock(&lock);

        printf("%s 사용!\n", inv.oxygen[n-1].name);

        for (int i = n-1; i < inv.oxygen_count - 1; i++) {
            inv.oxygen[i] = inv.oxygen[i+1];
        }
        inv.oxygen_count--;

        sleep(1);
    }

    void play_with_cat() {
        int choice = -1;
        item_choice = -1;

        printf("\n=== 놀아주기 ===\n");
        printf("0) 뒤로가기\n");
        printf("1) 🎵 리듬 게임 🎵\n");
        printf("2) 🪐 행성 피하기 게임🪐\n");
        printf("\n");
        printf("선택: ");
        fflush(stdout);

        while (item_choice == -1 && running) {
            usleep(100 * 1000);
        }

        choice = item_choice;
        item_choice = -1;

        switch(choice) {
            case 0:
                printf("뒤로 갑니다!\n");
                sleep(1);
                return;

            case 1: {
                if (input_thread_alive) {
                    pthread_cancel(th_input);
                    pthread_join(th_input, NULL);
                    input_thread_alive = 0;
                }

                RhythmGameResult r = rhythm_game();

                pthread_mutex_lock(&lock);

                for (int i = 0; i < r.reward_count; i++) {
                    if (inv.food_count >=20) break;
                    inv.food[inv.food_count] = r.reward;
                    inv.food_count++;
                }
                pthread_mutex_unlock(&lock);

                pthread_create(&th_input, NULL, input_thread, NULL);
                input_thread_alive = 1;

                pthread_mutex_lock(&lock);
                status.mood += 20;
                if (status.mood > 100) status.mood = 100; 
                pthread_mutex_unlock(&lock);

                printf("\n[🎵 리듬 게임 🎵 완료] %s x %d 획득!\n", r.reward.name, r.reward_count);
                sleep(1);
                return;
            }



	    case 2:
		    printf("행성 피하기 게임 시작!\n");
    		sleep(1);
    
    		system("./planet_avoid");
            
            pthread_mutex_lock(&lock);
            status.mood += 20;
            if (status.mood > 100) status.mood = 100;  
            pthread_mutex_unlock(&lock);

	        system("stty -echo -icanon");    
    		printf("게임 종료! 메인으로 돌아갑니다.\n");
    		sleep(2);
    		break;
            default:
                printf("잘못된 선택!\n");
                sleep(1);
        }
    }



    void* repair_thread(void* arg) {
        int total_time = 480; 
        
        for (int i = 0; i < total_time && running; i++) {

            sleep(1);

            pthread_mutex_lock(&lock);
            repair_progress = (i + 1) * (100.0 / total_time);
            pthread_mutex_unlock(&lock);
        }

        if (running) {
            running = 0;
            system("clear");

            printf("   _____ ____  _   _  _____ _____         _______ _    _ _            _______ _____ ____  _   _ \n");
            printf("  / ____/ __ \\| \\ | |/ ____|  __ \\     /\\|__   __| |  | | |        /\\|__   __|_   _/ __ \\| \\ | |\n");
            printf(" | |   | |  | |  \\| | |  __| |__) |   /  \\  | |  | |  | | |       /  \\  | |    | || |  | |  \\| |\n");
            printf(" | |   | |  | | . ` | | |_ |  _  /   / /\\ \\ | |  | |  | | |      / /\\ \\ | |    | || |  | | . ` |\n");
            printf(" | |___| |__| | |\\  | |__| | | \\ \\  / ____ \\| |  | |__| | |____ / ____ \\| |   _| || |__| | |\\  |\n");
            printf("  \\_____\\____/|_| \\_|\\_____|_|  \\_\\/_/    \\_\\_|   \\____/|______/_/    \\_\\_|  |_____\\____/|_| \\_|\n");
            printf("\n");
            printf("🎉 우주선 수리 완료! 탈출 성공!!!!\n");
            printf("\n");

            printf ("\"고마워! 네가 옆에서 도와준 덕분이야. 이제 같이 여행을 떠나보자~\"\n");


            printf("\n\n");
        }

        return NULL;
    }

    void run_space_cat_game() {

        
        srand(time(NULL));

        status.hunger = 70;
        status.oxygen = 70;
        status.mood   = 70;
        running = 1;

        pthread_mutex_init(&lock, NULL);

        inv.food_count = 3;
        strcpy(inv.food[0].name, "캔푸드"); inv.food[0].recovery = 20;
        strcpy(inv.food[1].name, "우주츄르"); inv.food[1].recovery = 30;
        strcpy(inv.food[2].name, "생선스낵"); inv.food[2].recovery = 10;

        inv.oxygen_count = 2;
        strcpy(inv.oxygen[0].name, "미니 산소통"); inv.oxygen[0].recovery = 15;
        strcpy(inv.oxygen[1].name, "우주 산소통"); inv.oxygen[1].recovery = 30;

        pthread_t th_status, th_repair;

        pthread_create(&th_status, NULL, decrease_status, NULL);

        pthread_create(&th_input, NULL, input_thread, NULL);
        input_thread_alive = 1;

        pthread_create(&th_repair, NULL, repair_thread, NULL);

        while (running) {
        show_status();

        pthread_mutex_lock(&lock);
        float p = repair_progress;
        pthread_mutex_unlock(&lock);

        if (status.hunger <= 25 && status.oxygen <= 25 && status.mood <= 25) {
            system("clear");
            printf("  _______      ___      .___  ___.  _______      ______   ____    ____  _______ .______      \n");
            printf(" /  _____|    /   \\     |   \\/   | |   ____|    /  __  \\  \\   \\  /   / |   ____||   _  \\     \n");
            printf("|  |  __     /  ^  \\    |  \\  /  | |  |__      |  |  |  |  \\   \\/   /  |  |__   |  |_)  |    \n");
            printf("|  | |_ |   /  /_\\  \\   |  |\\/|  | |   __|     |  |  |  |   \\      /   |   __|  |      /     \n");
            printf("|  |__| |  /  _____  \\  |  |  |  | |  |____    |  `--'  |    \\    /    |  |____ |  |\\  \\----.\n");
            printf(" \\______| /__/     \\__\\ |__|  |__| |_______|    \\______/      \\__/     |_______|| _| `._____|\n");
            printf("\n");

            printf("고양이가 버티지 못했습니다... 😿\n ");
            running = 0;
            break;
        }

        if (user_choice != -1) {
            switch(user_choice) {
                case 0: running = 0; break;
                case 1: use_food(); break;
                case 2: use_oxygen(); break;
                case 3: play_with_cat(); break;
                default: printf("잘못된 입력!\n"); sleep(1);
            }
            user_choice = -1;
        }

        usleep(200 * 1000);
    }

        pthread_join(th_status, NULL);
        pthread_join(th_repair, NULL);
        pthread_cancel(th_input);
        pthread_mutex_destroy(&lock);
    }
