#include "button.h"

void Button_Init(Button *b, uint32_t debounce_ms)
{
    b->debounce_ms     = debounce_ms;
    b->last_accept_ms  = 0;
    b->has_accepted    = false;
    b->press_count     = 0;
    b->seen_count      = 0;
}

bool Button_OnFallingEdge(Button *b, uint32_t now_ms)
{
    /* unsigned subtraction stays correct when the ms tick wraps around */
    if (b->has_accepted &&
        (uint32_t)(now_ms - b->last_accept_ms) < b->debounce_ms)
    {
        return false;
    }

    b->has_accepted   = true;
    b->last_accept_ms = now_ms;
    b->press_count++;
    return true;
}

uint32_t Button_TakeNewPresses(Button *b)
{
    uint32_t now   = b->press_count;
    uint32_t fresh = now - b->seen_count;

    b->seen_count = now;
    return fresh;
}
