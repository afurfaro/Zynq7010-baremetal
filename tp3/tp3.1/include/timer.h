#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "kernel.h"

/*------------------------------------------------*/
/* Cortex-A9 Private Timer                        */
/*------------------------------------------------*/

#define PTIMER_BASE            0xF8F00600

#define PTIMER_LOAD            REG32(PTIMER_BASE + 0x00)
#define PTIMER_COUNTER         REG32(PTIMER_BASE + 0x04)
#define PTIMER_CONTROL         REG32(PTIMER_BASE + 0x08)
#define PTIMER_ISR             REG32(PTIMER_BASE + 0x0C)

/*------------------------------------------------*/
/* Funciones                                      */
/*------------------------------------------------*/

void timer_init(uint32_t load);

#endif