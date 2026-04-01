/*
 * Linker script for STM32F411CE with TinyUF2 compatibility (64K RAM split)
 */

/* Entry Point */
ENTRY(reset_handler)

/* Specify the memory areas */
MEMORY
{
    rom (rx)        : ORIGIN = 0x08010000, LENGTH = 512K - 64K
    
    /* * ram_lower ends at 0x2000FFFB. 
     * The 4 bytes at 0x2000FFFC are the "hole" for the bootloader magic.
     */
    ram_lower (rwx) : ORIGIN = 0x20000000, LENGTH = 64K - 4
    
    /* ram_upper starts at 0x20010000, leaving 4 bytes unused between them */
    ram_upper (rwx) : ORIGIN = 0x20010000, LENGTH = 64K
}

/* Enforce emission of the vector table. */
EXTERN (vector_table)

/* Define sections. */
SECTIONS
{
    .text : {
        KEEP(*(.vectors))
        *(.text*)
        . = ALIGN(4);
        *(.rodata*)
        . = ALIGN(4);
    } >rom

    .preinit_array : {
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
    } >rom

    .init_array : {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
    } >rom

    .fini_array : {
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        __fini_array_end = .;
    } >rom

    .ARM.extab : {
        *(.ARM.extab*)
    } >rom

    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >rom

    . = ALIGN(4);
    _etext = .;

    /* All variable data sections now use ram_lower */
    .noinit (NOLOAD) : {
        *(.noinit*)
    } >ram_lower

    .data : {
        . = ALIGN(4);
        _data = .;
        *(.data*)
        *(.ramtext*)
        . = ALIGN(4);
        _edata = .;
    } >ram_lower AT >rom
    _data_loadaddr = LOADADDR(.data);

    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >ram_lower

    /DISCARD/ : { *(.eh_frame) }

    . = ALIGN(4);
    end = .;
}

/* * The stack starts at the very end of ram_upper (0x20020000).
 * This ensures the stack has its own 64K and won't collide with 
 * the magic token area at 0x2000FFFC.
 */
_estack = ORIGIN(ram_upper) + LENGTH(ram_upper);
PROVIDE(_stack = _estack);