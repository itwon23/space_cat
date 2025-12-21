#ifndef PLANET_AVOID_H
#define PLANET_AVOID_H

#include "Inventory.h"

typedef struct {
    Item reward;
    int reward_count;    // score / 10
} PlanetAvoidResult;

PlanetAvoidResult planet_avoid_game();

#endif
