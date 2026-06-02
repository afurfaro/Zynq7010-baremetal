.global _start
.extern kernel_main

.section .text
.code 32

_start:

    /* disable IRQ + FIQ */
    cpsid if

    /* set stack */
    ldr sp, =_stack_top

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

    /* jump to C */
    bl kernel_main

hang:
    b hang

    