#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "kernel.h"

/*------------------------------------------------*/
/* Cortex-A9 Private Timer                        */
/* Macros con las direcciones de los registros    */
/*------------------------------------------------*/

#define PTIMER_BASE            0xF8F00600   /*Dirección Base del Private Timer*/

#define PTIMER_LOAD            REG32(PTIMER_BASE + 0x00) /* Private Timer Load Register */
#define PTIMER_COUNTER         REG32(PTIMER_BASE + 0x04) /* Private Timer Count Register */
#define PTIMER_CONTROL         REG32(PTIMER_BASE + 0x08) /* Private Timer Control Register */
#define PTIMER_ISR             REG32(PTIMER_BASE + 0x0C) /* Private Timer Interrupt Status Register */

/*------------------------------------------------*/
/* Funciones                                      */
/*------------------------------------------------*/

void timer_init(uint32_t load);

#endif