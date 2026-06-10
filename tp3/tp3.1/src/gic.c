#include "gic.h"

void gic_init(void)
{
    /* Deshabilitar el GIC durante la configuración */

    GICC_CTLR = 0;
    GICD_CTLR = 0;

    /* Aceptar todas las prioridades */

    GICC_PMR = 0xFF;

    /* Habilitar IRQ 29 (Private Timer) */

    GICD_ISENABLER0 = SET_BIT(GICD_ISENABLER0, IRQ_PRIVATE_TIMER);

    /*
     * Enviar la interrupción al CPU0.
     *
     * IRQ29 pertenece al registro ITARGETSR7,
     * byte correspondiente al interrupt ID 29.
     */

    GICD_ITARGETSR7 |= (1 << 8);

    /*
     * Prioridad media.
     */

    GICD_IPRIORITYR7 |= (0x80 << 8);

    /* Habilitar CPU Interface */

    GICC_CTLR = 1;

    /* Habilitar Distributor */

    GICD_CTLR = 1;
}