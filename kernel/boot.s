.section .multiboot2, "a"
.balign 8

.set MB2_MAGIC, 0xE85250D6
.set MB2_ARCH,  0
.set MB2_LEN,   (mb2_end - mb2_start)
.set MB2_CHK,   -(MB2_MAGIC + MB2_ARCH + MB2_LEN)

mb2_start:
  .long MB2_MAGIC
  .long MB2_ARCH
  .long MB2_LEN
  .long MB2_CHK

  # Framebuffer request tag (type 5): prefer 1024x768x32 under UEFI/GOP.
  .short 5
  .short 0
  .long 20
  .long 1024
  .long 768
  .long 32

  .balign 8
  .short 0
  .short 0
  .long 8
mb2_end:

.section .bss
.align 16
stack_bottom:
  .skip 16384          # 16 KiB Stack
stack_top:

.section .text
.global _start
.extern kmain

_start:
  cli
  mov %eax, %esi
  mov %ebx, %edi
  mov $stack_top, %esp
  push %edi
  push %esi
  call kmain

.hang:
  hlt
  jmp .hang
