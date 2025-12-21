#include <stdio.h>
#include "Intro.h"
#include "SpaceCatGame.h"
#include "Inventory.h"              
Inventory global_inventory;        

int main() {
    init_inventory(&global_inventory);  
    show_title();
    show_prologue();
    run_space_cat_game();
    return 0;
}
