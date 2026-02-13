ENTRY(_start)

SECTIONS
{
  . = 1M;
  _kernel_start = .;

  .multiboot2 ALIGN(8) :
  {
    KEEP(*(.multiboot2))
  }

  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(COMMON) *(.bss*) }

  _kernel_end = .;
}
