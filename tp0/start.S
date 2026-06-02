.global _start
.extern kernel_main

.section .text
.code 32

_start:

    /* disable IRQ + FIQ */
    cpsid if

    /* set stack */
    ldr sp, =_stack_top

    /* jump to C */
    bl kernel_main

hang:
    b hang

    