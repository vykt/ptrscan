#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"


class player : public entity {

    private:
        bool is_x_pressed, is_y_pressed, which_player;

    public:
        player();

};

#endif
