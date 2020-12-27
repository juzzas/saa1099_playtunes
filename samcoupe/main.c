#include <stddef.h>
#include <z80.h>

#include "SAATunes.h"

static struct SAATunesContext L_context;

int main(void)
{
    __asm__ ("di");

    SAATunesInit(&L_context);

    SAATunesStrum(&L_context);

    while(1)
    {
        z80_delay_ms(1);
        SAATunesTick(&L_context);
    }


    __asm__ ("infinite: jr infinite");
    return 0;
}
