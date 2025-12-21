#ifndef INVENTORY_H
#define INVENTORY_H

typedef struct {
    char name[32];
    int recovery;
} Item;

typedef struct {
    Item food[20];
    int food_count;
    Item oxygen[20];
    int oxygen_count;
} Inventory;


void init_inventory(Inventory* inv);
void add_food(Inventory* inv, const char* name, int recovery);
void add_oxygen(Inventory* inv, int amount);
int use_oxygen(Inventory* inv);
void show_inventory(Inventory* inv);

#endif
