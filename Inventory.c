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
    for (int i = 0; i < amount && inv->oxygen_count < 20; i++) {
        strcpy(inv->oxygen[inv->oxygen_count].name, "산소통");
        inv->oxygen[inv->oxygen_count].recovery = 0;
        inv->oxygen_count++;
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
    printf("[산소통] %d개\n", inv->oxygen_count);
    printf("===================\n");
}
