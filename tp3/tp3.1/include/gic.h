#ifndef GIC_H
#define GIC_H

#include <stdint.h>
#include "kernel.h"

/*------------------------------------------------*/
/* Generic Interrupt Controller                   */
/*------------------------------------------------*/

/* Distributor */

#define GICD_BASE              0xF8F01000

#define GICD_CTLR              REG32(GICD_BASE + 0x000) /*ICDDCR` */
#define GICD_ISENABLER0        REG32(GICD_BASE + 0x100) /*ICDISER0*/
#define GICD_IPRIORITYR7       REG32(GICD_BASE + 0x41C) /*ICDIPR7`*/
#define GICD_ITARGETSR7        REG32(GICD_BASE + 0x81C) /*ICDIPTR7*/

/* CPU Interface */

#define GICC_BASE              0xF8F00100

#define GICC_CTLR              REG32(GICC_BASE + 0x000) /*ICCICR*/
#define GICC_PMR               REG32(GICC_BASE + 0x004) /*ICCPMR*/
#define GICC_IAR               REG32(GICC_BASE + 0x00C) /*ICCIAR*/
#define GICC_EOIR              REG32(GICC_BASE + 0x010) /*ICCEOIR*/

/*------------------------------------------------*/
/* IDs de interrupción                            */
/*------------------------------------------------*/

#define IRQ_PRIVATE_TIMER      29

/*------------------------------------------------*/
/* Funciones                                      */
/*------------------------------------------------*/

void gic_init(void);

#endif