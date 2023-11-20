#ifndef WORLD_H
#define WORLD_H

#include "player.h"
#include "enemy.h"


class world {

    public:
        int num_of_trees, num_of_rocks;
        enemy * enemies[8];
        player * players[2];

        world(int trees, int rocks);
};


#endif
