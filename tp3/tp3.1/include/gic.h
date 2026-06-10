#ifndef GIC_H
#define GIC_H

#include <stdint.h>
#include "kernel.h"

/*------------------------------------------------*/
/* Generic Interrupt Controller                   */
/*------------------------------------------------*/

/* Distributor */

#define GICD_BASE              0xF8F01000

#define GICD_CTLR              REG32(GICD_BASE + 0x000)
#define GICD_ISENABLER0        REG32(GICD_BASE + 0x100)
#define GICD_IPRIORITYR7       REG32(GICD_BASE + 0x41C)
#define GICD_ITARGETSR7        REG32(GICD_BASE + 0x81C)

/* CPU Interface */

#define GICC_BASE              0xF8F00100

#define GICC_CTLR              REG32(GICC_BASE + 0x000)
#define GICC_PMR               REG32(GICC_BASE + 0x004)
#define GICC_IAR               REG32(GICC_BASE + 0x00C)
#define GICC_EOIR              REG32(GICC_BASE + 0x010)

/*------------------------------------------------*/
/* IDs de interrupción                            */
/*------------------------------------------------*/

#define IRQ_PRIVATE_TIMER      29

/*------------------------------------------------*/
/* Funciones                                      */
/*------------------------------------------------*/

void gic_init(void);

#endif