/*
 * interrupt-asm.s
 *
 * Code for interrupt handling.  Refer to armisa.pdf in docs/ for what
 * the opcodes mean.
 */

/*
 * Enable/disable interrupts.
 *
 * CPSR = current program status register
 *        upper bits are different carry flags.
 *        lower 8:
 *           7 6 5 4 3 2 1 0
 *          +-+-+-+---------+
 *          |I|F|T|   Mode  |
 *          +-+-+-+---------+
 *
 *  I : disables IRQ when = 1.
 *  F : disables FIQ when = 1.
 *  T : = 0 indicates ARM execution, = 1 is thumb execution.
 *      Mode = current mode.
 */
 @ set 7th bit in CPSR to 0 to enable IRQ
.globl system_enable_interrupts
system_enable_interrupts:
    mrs r0,cpsr                 @ move process status register (PSR) to r0
    bic r0,r0,#(1<<7)		@ clear 7th bit.
    msr cpsr_c,r0		@ move r0 back to PSR
    bx lr		        @ return.

.globl system_disable_interrupts
system_disable_interrupts:
    mrs r0,cpsr		       
    orr r0,r0,#(1<<7)	       @ set 7th bit: or in 0b100 0000
    msr cpsr_c,r0
    bx lr

.globl _interrupt_table
.globl _interrupt_table_end
_interrupt_table:
  ldr pc, _reset_asm
  ldr pc, _undefined_instruction_asm
  ldr pc, _software_interrupt_asm
  ldr pc, _prefetch_abort_asm
  ldr pc, _data_abort_asm
  ldr pc, _reset_asm
  ldr pc, _interrupt_asm
fast_interrupt_asm:
  sub   lr, lr, #4 @First instr of FIQ handler
  push  {lr}
  push  {r0-r12}
  mov   r0, lr              @ Pass old pc
  bl    fast_interrupt_vector    @ C function
  pop   {r0-r12}
  ldm   sp!, {pc}^

_reset_asm:                   .word reset_asm
_undefined_instruction_asm:   .word undefined_instruction_asm
_software_interrupt_asm:      .word software_interrupt_asm
_prefetch_abort_asm:          .word prefetch_abort_asm
_data_abort_asm:              .word data_abort_asm
_interrupt_asm:               .word simpler_interrupt_asm
_interrupt_table_end:


.globl simpler_interrupt_asm
simpler_interrupt_asm:
  sub lr, lr, #4
  
  @ Check the DNI bit written to this address
  mov sp, #0x9000000
  ldr sp, [sp]

  @ if DNI bit is set, branch to do_not_preempt function
  cmp sp, #1
  beq do_not_preempt

  @ moves sp to the location where address of thread's reg array is
  mov sp, #0x9000000
  add sp, sp, #4
  @ load the address of the reg array to sp
  ldr sp, [sp]

  @ store the register in the reg array
  stmia sp!, {r0-r12}
  stm sp, {sp}^
  add sp, sp, #4
  stm sp, {lr}^
  add sp, sp, #4
  @add sp, sp, #60
  @ store the pc to return to at the 15th index of the reg array
  stm sp!, {lr}
  

  @ move sp here so that interrupt_vector can add things to the stack that grows down from this address
  mov sp, #0x9000000
  @ pass the cpsr of the previous thread to the function to save in the thread struct
  //mrs r0, spsr

  bl interrupt_vector

  @ this is the cpsr of the next thread, so set the spsr
  //msr spsr, r0

  @ the interrupt_vector has put the address of the new reg array into this address.
  mov sp, #0x9000000
  add sp, sp, #4
  @ load the address of the new reg array
  ldr sp, [sp]

  sub sp, sp, #4
  ldr r0, [sp]
  msr spsr, r0
  add sp, sp, #4

  @ load the values of the registers from the reg array
  ldmia sp!, {r0-r12}
  ldm sp, {r13}^
  add sp, sp, #4
  ldm sp, {r14}^
  add sp, sp, #4
  @add sp, sp, #60
  ldm sp, {r15}^


@ if we don't want to preempt, just turn off interrupts so we don't jump back to the 
@ interrupt handler. Then jump to the lr leaving all the registers as they were before 
@ the preemption.
do_not_preempt:
  push {r0-r12, lr}
  bl clear_arm_timer_interrupt
  pop {r0-r12, lr}

  movs pc, lr


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ we don't generate any of these, will just panic and halt.
@
reset_asm:
  sub   lr, lr, #4
  mov   r0, lr
  bl    reset_vector
undefined_instruction_asm:
  sub   lr, lr, #4
  mov   r0, lr
  bl    undefined_instruction_vector
software_interrupt_asm:
  sub   lr, lr, #4
  mov   r0, lr
  bl    software_interrupt_vector
prefetch_abort_asm:
  sub   lr, lr, #4
  mov   r0, lr
  bl    prefetch_abort_vector
data_abort_asm:
  sub   lr, lr, #4
  mov   r0, lr
  bl    data_abort_vector
