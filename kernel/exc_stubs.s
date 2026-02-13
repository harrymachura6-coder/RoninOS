.extern isr_exception_handler
.global isr0_stub

.macro EXC n
  .global isr\n\()_stub
isr\n\()_stub:
  cli
  pushl $0          # err_code fake (für Exceptions ohne err_code)
  pushl $\n         # int_no
  call isr_exception_handler
  add $8, %esp
  hlt
  jmp .
.endm

# 0..31
EXC 0
EXC 1
EXC 2
EXC 3
EXC 4
EXC 5
EXC 6
EXC 7
EXC 8
EXC 9
EXC 10
EXC 11
EXC 12
EXC 13
EXC 14
EXC 15
EXC 16
EXC 17
EXC 18
EXC 19
EXC 20
EXC 21
EXC 22
EXC 23
EXC 24
EXC 25
EXC 26
EXC 27
EXC 28
EXC 29
EXC 30
EXC 31
