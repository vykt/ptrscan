#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"


class enemy : public entity {

    private:
        short difficulty;
        int current_tactics;

    public:
        enemy(int difficulty);

};

#endif
