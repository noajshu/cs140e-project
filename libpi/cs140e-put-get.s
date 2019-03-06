@ dwelch routines for reading/writing memory.  we shoud be able to merge
@ the put32/PUT32's.  (we have different nows b/c of different prototypes)
.globl put32
put32:
    str r1,[r0]
    bx lr

.globl PUT32
PUT32:
    str r1,[r0]
    bx lr

.globl PUT16
PUT16:
    strh r1,[r0]
    bx lr

.globl PUT8
PUT8:
    strb r1,[r0]
    bx lr

.globl get32
get32:
    ldr r0,[r0]
    bx lr

.globl GET32
GET32:
    ldr r0,[r0]
    bx lr

.globl GETPC
GETPC:
    mov r0,lr
    bx lr

.globl dummy
dummy:
    bx lr

.globl BRANCHTO
BRANCHTO:
    bx r0
