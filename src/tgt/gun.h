#ifndef GUN_H
#define GUN_H


class gun {

    public:
        //render stuff
        int * render_obj_ptr;
        //type stuff
        short type;
        int ammo, ammo_max;
        //misc
        int can_reload;

        gun(short type, int ammo_max);
};

#endif
