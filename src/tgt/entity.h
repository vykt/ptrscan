#ifndef ENTITY_H
#define ENTITY_H

#include "gun.h"


class entity {

    public:
        //render stuff
        int * render_obj_ptr;
        //stat stuff 
        int health, armor;
        int health_max, armor_max;
        //pos stuff
        float x_pos, y_pos, z_pos;
        float x_rot, y_rot, z_rot;
        //gun stuff
        gun * weapon;

        entity(short gun_type, int ammo_max, int health_max, int armor_max);
};


#endif
