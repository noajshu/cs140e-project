@empty stub routines.  use these, or make your own.

.globl test_csave
test_csave:
	str r0, [r0, #-64]
	str r1, [r0, #-60]
	str r2, [r0, #-56]
	str r3, [r0, #-52]
	str r4, [r0, #-48]
	str r5, [r0, #-44]
	str r6, [r0, #-40]
	str r7, [r0, #-36]
	str r8, [r0, #-32]
	str r9, [r0, #-28]
	str r10, [r0, #-24]
	str r11, [r0, #-20]
	str r12, [r0, #-16]
	str r13, [r0, #-12]
	str r14, [r0, #-8]
	mrs r1, cpsr
	str r1, [r0, #-4]
	add r0, r0, #-64
	bx lr


.globl test_csave_stmfd
test_csave_stmfd:
	bx lr

.globl rpi_get_cpsr
rpi_get_cpsr:
	mrs r0, CPSR
	bx lr

.globl rpi_cswitch
rpi_cswitch:
	add sp, #-60

	@ STORE
	str r0, [sp]
	str sp, [r0]
	add sp, #4
	stm sp, {r1-r12, r14}
	@ str r1, [sp, #4]
	@ str r2, [sp, #8]
	@ str r3, [sp, #12]
	@ str r4, [sp, #16]
	@ str r5, [sp, #20]
	@ str r6, [sp, #24]
	@ str r7, [sp, #28]
	@ str r8, [sp, #32]
	@ str r9, [sp, #36]
	@ str r10, [sp, #40]
	@ str r11, [sp, #44]
	@ str r12, [sp, #48]
	@ str r14, [sp, #52]
	sub sp, #4

	mrs r4, cpsr
	str r4, [sp, #56]

	@ LOAD
	ldr sp, [r1]
	ldr r4, [sp, #56]
	msr cpsr, r4
	@ ldr r0, [sp]
	@ ldr r1, [sp, #4]
	@ ldr r2, [sp, #8]
	@ ldr r3, [sp, #12]
	@ ldr r4, [sp, #16]
	@ ldr r5, [sp, #20]
	@ ldr r6, [sp, #24]
	@ ldr r7, [sp, #28]
	@ ldr r8, [sp, #32]
	@ ldr r9, [sp, #36]
	@ ldr r10, [sp, #40]
	@ ldr r11, [sp, #44]
	@ ldr r12, [sp, #48]
	@ ldr r14, [sp, #52]
	ldmia sp, {r0-r12,r14}

	add sp, #60

	bx lr



@ [Make sure you can answer: why do we need to do this?]
@
@ use this to setup each thread for the first time.
@ setup the stack so that when cswitch runs it will:
@	- load address of <rpi_init_trampoline> into LR
@	- <code> into r1, 
@	- <arg> into r0
@ 
.globl rpi_init_trampoline
rpi_init_trampoline:
	@ mov r2, sp
	@ mov r3, lr
	@ b check_regs

	blx r1
b rpi_exit


