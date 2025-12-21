#include "Inventory.h"
#include <string.h>
#include <stdio.h>

void init_inventory(Inventory* inv) {
    inv->food_count = 0;
    inv->oxygen_count = 0;
}

void add_food(Inventory* inv, const char* name, int recovery) {
    if (inv->food_count < 20) {
        strncpy(inv->food[inv->food_count].name, name, 31);
        inv->food[inv->food_count].name[31] = '\0';
        inv->food[inv->food_count].recovery = recovery;
        inv->food_count++;
    }
}

void add_oxygen(Inventory* inv, int amount) {
    if (inv->oxygen_count > 0) {
        // 기존 산소통이 있으면 count 증가
        inv->oxygen[0].count += amount;
        printf("[DEBUG] 기존 산소통에 %d개 추가, 총 %d개\n", amount, inv->oxygen[0].count);
        return;
    }

    // 새 산소통 아이템으로 추가
    if (inv->oxygen_count < 20) {
        strcpy(inv->oxygen[inv->oxygen_count].name, "산소통");
        inv->oxygen[inv->oxygen_count].recovery = 1;
        inv->oxygen[inv->oxygen_count].count = amount;
        inv->oxygen_count++;
        printf("[DEBUG] 새 산소통 추가, 총 %d개\n", inv->oxygen[0].count);
    }
}


int use_oxygen(Inventory* inv) {
    if (inv->oxygen_count > 0) {
        inv->oxygen_count--;
        return 1;
    }
    return 0;
}

void show_inventory(Inventory* inv) {
    



    printf("\n===== 인벤토리 =====\n");
    printf("[음식] %d개\n", inv->food_count);
    for (int i = 0; i < inv->food_count; i++) {
        printf("  - %s (회복: %d)\n", inv->food[i].name, inv->food[i].recovery);
    }
    printf("[산소통] %d종\n", inv->oxygen_count);
for (int i = 0; i < inv->oxygen_count; i++) {
    printf("  %d) %s (+%d) x%d\n", i + 1, inv->oxygen[i].name, inv->oxygen[i].recovery, inv->oxygen[i].count);
}

    printf("===================\n");
}
