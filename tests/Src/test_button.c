#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "button.h"

static void check(const char *name, long actual, long expected)
{
    if (actual != expected)
    {
        printf("[FAIL] %s: expected=%ld actual=%ld\n", name, expected, actual);
        exit(1);
    }
    printf("[PASS] %s\n", name);
}

int main(void)
{
    Button b;
    printf("Button host tests\n");

    Button_Init(&b, 50);
    check("first edge accepted", Button_OnFallingEdge(&b, 1000), 1);
    check("bounce 10 ms later rejected", Button_OnFallingEdge(&b, 1010), 0);
    check("bounce 49 ms later rejected", Button_OnFallingEdge(&b, 1049), 0);
    check("edge after 50 ms accepted", Button_OnFallingEdge(&b, 1050), 1);
    check("take returns 2 presses", Button_TakeNewPresses(&b), 2);
    check("take again returns 0", Button_TakeNewPresses(&b), 0);

    Button_Init(&b, 50);
    check("edge at t=0 accepted (first edge)", Button_OnFallingEdge(&b, 0), 1);

    Button_Init(&b, 50);
    Button_OnFallingEdge(&b, 0xFFFFFFF0u);
    check("tick wraparound: +10 ms rejected", Button_OnFallingEdge(&b, 0xFFFFFFFAu), 0);
    check("tick wraparound: +60 ms accepted", Button_OnFallingEdge(&b, 0x0000002Cu), 1);

    printf("[PASS] ALL TESTS\n");
    return 0;
}
