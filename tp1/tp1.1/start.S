.global _start
.extern kernel_main 

.section .text
.code 32

_start:

    /* disable IRQ + FIQ */
    cpsid if

    /* set stack */
    ldr sp, =_stack_top

    /* Punteros a inicio y fin de .bss*/
    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
    /* SIEMPRE el boot pone a cero las variables no inicializadas */
clear_bss:
    cmp r0, r1
    strlo r2, [r0], #4 /*Ejecución condicional. Ver README.md*/
    blo clear_bss


    /* jump to C */
    bl kernel_main

hang:
    b hang

    
