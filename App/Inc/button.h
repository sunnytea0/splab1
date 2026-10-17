#ifndef BUTTON_H
#define BUTTON_H

/*
 * Button: debounce + press counter, no HAL dependency (host-testable).
 *
 * ISR context  : Button_OnFallingEdge()
 * Main context : Button_TakeNewPresses()
 *
 * Lock-free by design: press_count is written only by the ISR,
 * seen_count only by the main loop, so no critical section is needed.
 */

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t debounce_ms;
    uint32_t last_accept_ms;
    bool     has_accepted;
    volatile uint32_t press_count;   /* written by ISR only  */
    uint32_t seen_count;             /* written by main only */
} Button;

void     Button_Init(Button *b, uint32_t debounce_ms);

/* Call from the EXTI falling-edge callback. Returns true if the edge counted. */
bool     Button_OnFallingEdge(Button *b, uint32_t now_ms);

/* Call from the main loop. Returns number of new accepted presses. */
uint32_t Button_TakeNewPresses(Button *b);

#endif
