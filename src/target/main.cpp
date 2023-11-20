/*
 *
 *  target program to test out ptrscan validity
 *
 */

#include <iostream>
#include <string>

#include "world.h"


//declare world
world * game_world;


int main() {

    std::string temp;

    game_world = NULL;
    game_world = new world(20, 10);
    if (!game_world)
        throw std::runtime_error("secret error\n");

    while (1) {

        std::cout << "game is running..." << std::endl;

        for (int i = 0; i < 2; ++i) {
            game_world->players[i]->health -= 1;
            game_world->players[i]->weapon->ammo -= 1;
        }

        for (int i = 0; i < 8; ++i) {
            game_world->enemies[i]->health -= 2;
            game_world->enemies[i]->weapon->ammo -= 1;
        }

        std::cout << "continue (enter): ";
        std::getline(std::cin, temp);

    } //end while

    return 0;
}
