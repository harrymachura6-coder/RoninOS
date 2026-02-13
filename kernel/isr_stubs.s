.global irq0_stub
.global irq1_stub
.extern irq0_handler_c
.extern irq1_handler_c

irq0_stub:
    pusha
    call irq0_handler_c
    popa
    iret

irq1_stub:
    pusha
    call irq1_handler_c
    popa
    iret
