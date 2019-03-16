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
_interrupt_asm:               .word interrupt_asm
_interrupt_table_end:


.globl reggie1234
reggie1234:
  mov r0, #1
  mov r1, #2
  mov r2, #3
  mov r3, #4
  b reggie1234


@ only handler that should run since we only enable general interrupts
interrupt_asm:
  @ mov sp, #0x8000 @ what dwelch uses: bad if int handler uses deep stack
  mov r2, sp

  mov sp, #0x9000000  @ i believe we have 512mb - 16mb, so this should be safe
  sub   lr, lr, #4
  push {r0-r12, lr}

  @ldr r0, =0x100000
  @cmp r0, #0
  @bne do_not_preempt

  sub sp, sp, #4
  mov r0, sp 
  sub sp, sp, #4
  mov r1, sp
  
  push {r0-r12, lr}
  bl  interrupt_vector 
  pop {r0-r12, lr}

  @r0 has addr for sp of next thread
  @r1 has addr of sp of prev thread
  pop {r0}
  pop {r1}
  ldr r0, [r0]
  sub r0, r0, #64
  ldr r1, [r1]

  @pop the reg values of the prev thread one by one 
  @and store them in the prev thread stack
  pop {r3} //r0
  str r3, [r1]
  pop {r3} //r1
  str r3, [r1, #4]
  pop {r3} //r4
  str r3, [r1, #8]
  pop {r3} //r3
  str r3, [r1, #12]
  pop {r3} //r4
  str r3, [r1, #16]
  pop {r3} //r5
  str r3, [r1, #20]
  pop {r3} //r6
  str r3, [r1, #24]
  pop {r3} //r7
  str r3, [r1, #28]
  pop {r3} //r8
  str r3, [r1, #32]
  pop {r3} //r9
  str r3, [r1, #36]
  pop {r3} //r10
  str r3, [r1, #40]
  pop {r3} //r11
  str r3, [r1, #44]
  pop {r3} //r12
  str r3, [r1, #48]
  pop {r3} //pc
  str r3, [r1, #52]

  add r3, r1, #56
  stm r3, {lr}^

  mrs r3, spsr   //cpsr of prev thread
  str r3, [r1, #60]

  @restore next reg values
  mov sp, r0

  @update the spsr
  ldr r0, [sp, #60]
  msr spsr, r0

  add r0, sp, #56
  ldm r0, {lr}^ @lr for future function

  pop {r0-r12, lr}
  add sp, sp, #8
  movs pc, lr   @ moves the link register into the pc and implicitly
                @ loads the PC with the result, then copies the 
                @ SPSR to the CPSR.


do_not_preempt:
   pop {r0-r12, lr}
   mov r0, lr
   bl check_regs
   movs pc, lr

  @update the spsr
  @ldr r0, [sp, #60]
  @msr spsr, r0

  @add r0, sp, #52
  @stm r0, {lr}^

  @ldr lr, [sp, #56]

  @pop {r0-r12}

  @add sp, sp, #12


  @__________________









  @push  {r0-r12,lr}         @ XXX: pushing too many registers: only need caller
  @ vpush {s0-s15}	    @ uncomment if want to save caller-saved fp regs

  @stm r0, {lr}
  @stm r1, {pc}^
  @bl check_regs

  @mrs r0, spsr
  @mov r1, sp
  @mov r2, lr
  @push {r0-r2}

  @mov   r0, lr              @ Pass old pc
  @bl    interrupt_vector    @ C function
  @cmp r0, #0
  @ do context switch here
  @bne _interrupt_context_switch


  @ vpop {s0-s15}           @ pop caller saved fp regs
  @pop   {r0-r12,lr} 	    @ pop integer registers

  @ return from interrupt handler: will re-enable general ints.
  @movs    pc, lr        @ moves the link register into the pc and implicitly
                        @ loads the PC with the result, then copies the 
                        @ SPSR to the CPSR.


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
