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

#endif
