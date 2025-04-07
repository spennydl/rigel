#ifndef RIGEL_TIMER_H
#define RIGEL_TIMER_H

#include "rigel.h"

namespace rigel {

struct ZeroCrossTrigger {
    i32 last_observed_sign;
};

inline void update_zero_cross_trigger(ZeroCrossTrigger* trigger, f32 value)
{
    if (trigger->last_observed_sign == 0)
    {
        trigger->last_observed_sign = 1;
    }

    if (value != 0)
    {
        trigger->last_observed_sign = (value < 0) ? -1 : 1;
    }
}

} // namespace rigel


#endif // RIGEL_TIMER_H
