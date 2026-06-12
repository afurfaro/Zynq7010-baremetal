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
#define BIT(n)                (1U << (n))                   /*Construye una máscara*/
#define SET_BIT(value, bit)   ((value) |  (1U << (bit)))    /*Setea bit dentro de reg*/
#define CLR_BIT(value, bit)   ((value) & ~(1U << (bit)))    /*Limpia bit dentro de reg*/

/* Variables globales del kernel (si las hubiera) */

/* Funciones generales (si las hubiera) */


#endif