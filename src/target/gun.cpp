#include "gun.h"

gun::gun(short type, int ammo_max) {

    this->type = type;
    this->ammo = this->ammo_max = ammo_max;
    this->can_reload = 0;
    this->render_obj_ptr = (int *) 0x1234;
}
