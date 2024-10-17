#include "world.h"
#include "player.h"
#include "enemy.h"


world::world(int trees, int rocks) {

    this->num_of_trees = trees;
    this->num_of_rocks = rocks;

    for (int i = 0; i < 8; ++i) {
        enemies[i] = new enemy(3);
    }

    for (int i = 0; i < 2; ++i) {
        players[i] = new player();
    }
}
