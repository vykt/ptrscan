#include "player.h"
#include "entity.h"


player::player() : entity(10, 80, 600, 200) {

    this->is_x_pressed = this->is_y_pressed = this->which_player = 0;
}
