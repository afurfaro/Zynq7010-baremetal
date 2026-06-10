#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

/*------------------------------------------------*/
/* Acceso a registros mapeados en memoria         */
/*------------------------------------------------*/

#define REG32(addr) (*(volatile uint32_t *)(addr))

/*------------------------------------------------*/
/* Manipulación de bits                           */
/*------------------------------------------------*/

#define SET_BIT(value, bit)   ((value) |  (1U << (bit)))
#define CLR_BIT(value, bit)   ((value) & ~(1U << (bit)))

/* Variables globales del kernel (si las hubiera) */

/* Funciones generales (si las hubiera) */


#endif