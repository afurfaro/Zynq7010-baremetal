.include "include/armv7.inc"

.global _start
.extern kernel_main

/*----------------------------------*/
/* Vector table                     */
/*----------------------------------*/
.section .vectors, "ax"

_vectors:
    b _start          /* Reset  salta a _start que es el inicio de la sección .text*/
    b undef_handler   /* Undefined */
    b svc_handler     /* SVC */
    b pabort_handler  /* Prefetch abort */
    b dabort_handler  /* Data abort */
    b .               /* Reserved */   
    b irq_handler     /* IRQ */
    b fiq_handler     /* FIQ */

/*----------------------------------*/
/* Boot code                        */
/*----------------------------------*/   
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

    /* stack para modo Supervisor */
    cps #SVC_MODE
    ldr sp, =_stack_svc_top

    /* stack para modo System */
    cps #SYS_MODE
    ldr sp, =_stack_sys_top

/*----------------------*/
    /* copiar .data */
    ldr r0, =_data_load
    ldr r1, =_data_start
    ldr r2, =_data_end

copy_data:
    ldr r3, [r0], #4
    str r3, [r1], #4
    cmp r1, r2
    blo copy_data
/*----------------------*/

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

/*----------------------------------*/
/* Exception handlers               */
/* Por ahora ejecutan el mismo loop */
/* infinito, pero ahora dentro de   */
/* cada Handler.                    */
/*----------------------------------*/

undef_handler:
    b .

svc_handler:
    mrs r0, cpsr
    mrs r1, spsr
    b .

pabort_handler:
    b .

dabort_handler:
    b .

irq_handler:
    b .

fiq_handler:
    b .
    