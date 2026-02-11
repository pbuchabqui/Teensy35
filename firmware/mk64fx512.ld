/**
 * @file mk64fx512.ld
 * @brief Linker script for Teensy 3.5 (MK64FX512VMD12)
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This linker script defines the memory layout for the Freescale/NXP
 * Kinetis MK64FX512 microcontroller used on the Teensy 3.5 board.
 *
 * Memory Map:
 * - Flash: 0x00000000 - 0x0007FFFF (512 KB)
 * - RAM:   0x1FFF0000 - 0x2002FFFF (256 KB)
 * - Stack: Top of RAM (grows downward)
 * - Heap:  After BSS section (grows upward)
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

/* Entry point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);    /* End of RAM */
_sstack = _estack - 0x4000;             /* 16KB stack */

/* Minimum heap size */
_Min_Heap_Size = 0x2000;  /* 8KB */
_Min_Stack_Size = 0x4000; /* 16KB */

/* Memory regions definition */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x00000000, LENGTH = 512K
    RAM (rwx)       : ORIGIN = 0x1FFF0000, LENGTH = 256K
}

/* Sections */
SECTIONS
{
    /* Interrupt vector table */
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))     /* Startup code */
        . = ALIGN(4);
    } >FLASH

    /* Program code and read-only data */
    .text :
    {
        . = ALIGN(4);
        *(.text)              /* Normal code */
        *(.text*)             /* All text sections */
        *(.glue_7)            /* ARM/Thumb interworking */
        *(.glue_7t)
        *(.eh_frame)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        _etext = .;           /* End of code section */
    } >FLASH

    /* Read-only data (constants) */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)            /* Read-only data */
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* C++ exception handling (not used in embedded) */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH

    /* Pre-init array */
    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } >FLASH

    /* Init array (C++ constructors) */
    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } >FLASH

    /* Fini array (C++ destructors) */
    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } >FLASH

    /* Used by startup to initialize data */
    _sidata = LOADADDR(.data);

    /* Initialized data section (loaded into RAM at startup) */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;           /* Create a global symbol at data start */
        *(.data)              /* .data sections */
        *(.data*)             /* .data* sections */
        *(.RAMtext)           /* Functions to run from RAM */

        . = ALIGN(4);
        _edata = .;           /* Define a global symbol at data end */
    } >RAM AT>FLASH

    /* Uninitialized data section (zero-initialized at startup) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;            /* Define a global symbol at bss start */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;            /* Define a global symbol at bss end */
        __bss_end__ = _ebss;
    } >RAM

    /* Heap section (grows upward from end of BSS) */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        PROVIDE ( __end__ = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /* Remove debugging information from standard libraries */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    /* ARM Cortex-M specific sections */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}
