#include "mapping.h"

const char *mapping_buttonaxis_labels[] =
{
    "None",
    "A",
    "B",
    "X",
    "Y",
    "D-Pad Left",
    "D-Pad Right",
    "D-Pad Down",
    "D-Pad Up",
    "Start",
    "Z",
    "R Button",
    "L Button",
    "R Analog",
    "L Analog",
    "Left",
    "Right",
    "Down",
    "Up",
    "C-Left",
    "C-Right",
    "C-Down",
    "C-Up",
};


void process_inputs_analog(gc_inputs *i)
{
    i->ax = clamp(deadzone(i->ax, cfg.dz) * cfg.range / 100, -128, 127);
    i->ay = clamp(deadzone(i->ay, cfg.dz) * cfg.range / 100, -128, 127);

    i->cx = clamp(deadzone(i->cx, cfg.dz) * cfg.range / 100, -128, 127);
    i->cy = clamp(deadzone(i->cy, cfg.dz) * cfg.range / 100, -128, 127);
}

void process_inputs_digital(gc_inputs *i)
{
    if (cfg.scale_diagonals) {
        struct Vec2 cstick = circle_to_square(i->cx, i->cy);
        i->cx = cstick.x;
        i->cy = cstick.y;

        struct Vec2 stick = circle_to_square(i->ax, i->ay);
        i->ax = stick.x;
        i->ay = stick.y;
    }
}

const char *mapping_get_label(enum MappingButtonAxis ba)
{
    if (ba >= BA_NONE && ba < BA_MAX) {
        return mapping_buttonaxis_labels[ba];
    } else {
        return mapping_buttonaxis_labels[BA_NONE];
    }
}

int get_buttonaxis_state(enum MappingButtonAxis ba, gc_inputs *i, gc_inputs *id, int is_analog)
{
    if (!is_analog) {
        i = id;
    }

    int state;

    switch (ba)
    {
    case BA_A:
        state = i->a;
        break;
    case BA_B:
        state = i->b;
        break;
    case BA_X:
        state = i->x;
        break;
    case BA_Y:
        state = i->y;
        break;
    case BA_DLEFT:
        state = i->dleft;
        break;
    case BA_DRIGHT:
        state = i->dright;
        break;
    case BA_DDOWN:
        state = i->ddown;
        break;
    case BA_DUP:
        state = i->dup;
        break;
    case BA_START:
        state = i->start;
        break;
    case BA_Z:
        state = i->z;
        break;
    case BA_R_BUTTON:
        state = i->r;
        break;
    case BA_L_BUTTON:
        state = i->l;
        break;
    case BA_R_ANALOG:
        state = i->rt;
        break;
    case BA_L_ANALOG:
        state = i->lt;
        break;
    case BA_LEFT:
        state = smax(-i->ax, 0);
        break;
    case BA_RIGHT:
        state = smax(i->ax, 0);
        break;
    case BA_DOWN:
        state = smax(-i->ay, 0);
        break;
    case BA_UP:
        state = smax(i->ay, 0);
        break;
    case BA_CLEFT:
        state = smax(-i->cx, 0);
        break;
    case BA_CRIGHT:
        state = smax(i->cx, 0);
        break;
    case BA_CDOWN:
        state = smax(-i->cy, 0);
        break;
    case BA_CUP:
        state = smax(i->cy, 0);
        break;
    default:
        return 0;
        break;
    }

    if (is_analog) {
        if (ba < BA_R_ANALOG) {
            return state ? cfg.range : 0;
        }
        if (ba == BA_R_ANALOG || ba == BA_L_ANALOG) {
            return state / 2;
        }
    } else {
        if (ba >= BA_LEFT && ba <= BA_CUP) {
            return state > cfg.stick_a2d_thres;
        }
        if (ba == BA_R_ANALOG || ba == BA_L_ANALOG) {
            return (state - cfg.trig_thres) > 0;
        }
    }

    return state;
}

int get_mapping_state(gc_inputs *i, gc_inputs *id, struct Mapping m, int is_analog)
{
    int p = get_buttonaxis_state(m.pri, i, id, is_analog);
    int s = get_buttonaxis_state(m.sec, i, id, is_analog);

    if (is_analog) {
        return smax(p, s);
    }

    return p | s;
}
