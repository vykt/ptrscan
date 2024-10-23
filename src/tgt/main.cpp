/*
 *
 *  target program to test out ptrscan validity
 *
 */

#include <iostream>
#include <string>

#include <cstdint>

#include <sys/mman.h>

#include "world.h"


//declare world
world * game_world;


int main() {

    std::string temp;

    game_world = NULL;
    game_world = new world(20, 10);
    if (!game_world)
        throw std::runtime_error("secret error\n");

    bool mapped = false;
    uintptr_t * map_ptr;
    

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

        std::cout << "player health: " << game_world->players[0]->health << std::endl;
        std::cout << "continue (enter): ";
        std::getline(std::cin, temp);

        //after first iteration, map new memory area with an additional ptrchain
        if (!mapped) {

            map_ptr = (uintptr_t *) mmap(nullptr, 0x1000, PROT_READ | PROT_WRITE, 
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

            *map_ptr     = (uintptr_t) &game_world->players[0]->health;
            *(map_ptr+1) = (uintptr_t) &game_world->players[1]->health;

            mapped = true;
        }

    } //end while

    return 0;
}
