ENTRY(_start)

MEMORY
{
    OCM_LOW  (rx)  : ORIGIN = 0x00000000, LENGTH = 128K
    OCM_HIGH (rwx) : ORIGIN = 0x00020000, LENGTH = 128K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))
        *(.text*)
    } > OCM_LOW

    .rodata :
    {
        *(.rodata*)
    } > OCM_LOW

    _data_load = .;          /* LMA de .data: justo después de .rodata en OCM_LOW */

    .data :
    {
        _data_start = .;
        *(.data*)
        _data_end = .;
    } > OCM_HIGH AT> OCM_LOW

    .bss (NOLOAD) :
    {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } > OCM_HIGH

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        . += 0x4000;
        _stack_top = .;
    } > OCM_HIGH
}