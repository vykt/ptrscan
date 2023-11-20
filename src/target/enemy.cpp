#include "enemy.h"
#include "entity.h"


enemy::enemy(int difficulty) : entity(15, 30, 300, 100) {

    this->difficulty = difficulty;
    this->current_tactics = 0;
}
