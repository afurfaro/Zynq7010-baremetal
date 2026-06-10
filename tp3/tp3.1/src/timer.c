#include "timer.h"

void timer_init(uint32_t load)
{
    /* Valor inicial del contador */

    PTIMER_LOAD = load;

    /*
     * Control Register
     *
     * bit0 = ENABLE
     * bit1 = AUTO_RELOAD
     * bit2 = IRQ_ENABLE
     * bits[15:8] = PRESCALER
     */

    PTIMER_CONTROL = (1 << 0) |
                     (1 << 1) |
                     (1 << 2);

    /* Limpiar una posible interrupción pendiente */

    PTIMER_ISR = 1;
}