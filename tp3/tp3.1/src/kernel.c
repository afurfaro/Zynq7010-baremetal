#include "kernel.h"
#include "gic.h"
#include "timer.h"

void kernel_main(void)
{
    /* Inicialización de periféricos */

    gic_init();

    timer_init(1000000);

    /* Habilitar IRQ en el procesador */

    asm volatile ("cpsie i");

    /* El kernel queda esperando interrupciones */

    while (1)
    {
        asm volatile ("nop");
    }
}