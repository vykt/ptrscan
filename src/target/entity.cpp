#include "entity.h"
#include "gun.h"


entity::entity(short gun_type, int ammo_max, int health_max, int armor_max) {

    this->health = this->health_max = health_max;
    this->armor = this->armor_max = armor_max;

    x_pos = y_pos = z_pos = 0;
    x_rot = y_rot = z_rot = 90.0f;

    this->weapon = new gun(gun_type, ammo_max);
    
    this->render_obj_ptr = (int *) 0x8796;

}
