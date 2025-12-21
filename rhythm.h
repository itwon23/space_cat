#ifndef RHYTHM_H
#define RHYTHM_H

#include "Inventory.h"

typedef struct {
    Item reward;
    int reward_count;    // score / 7
} RhythmGameResult;

RhythmGameResult rhythm_game(void);

#endif
