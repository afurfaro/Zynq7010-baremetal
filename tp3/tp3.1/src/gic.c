#include "gic.h"

void gic_init(void)
{
    /* Deshabilitar el GIC durante la configuración */

    GICC_CTLR = 0;  /*Deshabilita CPU Interface: ICCICR[0] = 0*/
    GICD_CTLR = 0;  /*Deshabilita Distrbutor: ICDICR[0] = 0*/

    /* Aceptar todas las prioridades */

    GICC_PMR = 0xFF; /*Establecemos prioridad mínma para enviar interrupciones (0xFF prioridad mínima)*/

    /* Habilitar IRQ 29 (Private Timer) */

    GICD_ISENABLER0 = SET_BIT(GICD_ISENABLER0, IRQ_PRIVATE_TIMER); /*ICDISER0[29] 0 = 1*/

    /*
     * Configura al Distribuidor para enviar la interrupción al CPU0.
     * Para determinar n en ICDIPTRn : n = Interrupt ID / 4, (4 es la cantidad de campos de 
     * cada registro). n = 29/4 = 7
     * Número de byte dentro del registro: Mod (29,4) = 1
     * IRQ29 pertenece al registro ITARGETSR7, y la CPU se escribe en el byte N° 1 
     * byte correspondiente al interrupt ID 29.
     */

    GICD_ITARGETSR7 |= (1 << 8); /*ITARGETSR7[15:8] = 00000001. Bit mask. Es decir: CPU0*/

    /*
     * Prioridad media.
     */

    GICD_IPRIORITYR7 |= (0x80 << 8);

    /* Habilitar CPU Interface */

    GICC_CTLR = 1;

    /* Habilitar Distributor */

    GICD_CTLR = 1;
}