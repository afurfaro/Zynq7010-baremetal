.include "include/armv7.inc"

.global _start
.extern kernel_main

.section .text
.code 32

_start:

    /* disable IRQ + FIQ */
    cpsid if

/*----------------------*/
    /* set stacks */
    /* stack para Modo IRQ */
    cps #IRQ_MODE
    ldr sp, =_stack_irq_top

    /* stack para Modo FIQ */
    cps #FIQ_MODE
    ldr sp, =_stack_fiq_top

    /* stack para modo Abort */
    cps #ABT_MODE
    ldr sp, =_stack_abt_top

    /* stack para modo Undefined */
    cps #UND_MODE
    ldr sp, =_stack_und_top

    /* stack para modo SVC */
    cps #SVC_MODE
    ldr sp, =_stack_svc_top

    /* stack para modo System */
    cps #SYS_MODE
    ldr sp, =_stack_sys_top

/*----------------------------------*/
/* copiar .data desde LMA a VMA     */
/*----------------------------------*/
    ldr r0, =_data_load    /* origen  (LMA en OCM_LOW)  */
    ldr r1, =_data_start   /* destino (VMA en OCM_HIGH) */
    ldr r2, =_data_end

copy_data:
    cmp r1, r2
    ldrlo r3, [r0], #4
    strlo r3, [r1], #4
    blo copy_data

/*----------------------*/
    /* Punteros a inicio y fin de .bss*/
    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
    /* SIEMPRE el boot pone a cero las variables no inicializadas */
clear_bss:
    cmp r0, r1
    strlo r2, [r0], #4 /*Ejecución condicional. Ver README.md*/
    blo clear_bss
/*----------------------*/

    /* jump to C */
    bl kernel_main

hang:
    b hang

    