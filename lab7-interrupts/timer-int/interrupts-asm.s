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

@ only handler that should run since we only enable general interrupts
interrupt_asm:
  @ mov sp, #0x8000 @ what dwelch uses: bad if int handler uses deep stack
  mov sp, #0x9000000  @ i believe we have 512mb - 16mb, so this should be safe
  sub   lr, lr, #4
  push {r0-r12, lr}
  

  sub sp, sp, #4
  mov r0, sp 
  sub sp, sp, #4
  mov r1, sp
  @sub sp, sp, #4
  @mov r2, sp
  bl  interrupt_vector 

  @r0 has addr for sp of next thread
  @r1 has addr of sp of prev thread
  pop {r0-r1}
  
  @r2 holds the sp of the prev thread
  ldr r4, [r1]
  sub r4, r4, #64

  @pop the reg values of the prev thread one by one 
  @and store them in the prev thread stack
  pop {r3} //r0
  str r3, [r4]
  pop {r3} //r1
  str r3, [r4, #4]
  pop {r3} //r4
  str r3, [r4, #8]
  pop {r3} //r3
  str r3, [r4, #12]
  pop {r3} //r4
  str r3, [r4, #16]
  pop {r3} //r5
  str r3, [r4, #20]
  pop {r3} //r6
  str r3, [r4, #24]
  pop {r3} //r7
  str r3, [r4, #28]
  pop {r3} //r8
  str r3, [r4, #32]
  pop {r3} //r9
  str r3, [r4, #36]
  pop {r3} //r10
  str r3, [r4, #40]
  pop {r3} //r11
  str r3, [r4, #44]
  pop {r3} //r12
  str r3, [r4, #48]
  pop {r3} //pc
  str r3, [r4, #52]

  ldm r3, {lr}^ //lr of prev thread
  ldr r3, [r3]
  str r3, [r4, #56]

  mrs r3, spsr   //cpsr of prev thread
  str r3, [r4, #60]

  @update the prev thread sp in the struct
  @add r4, r2, #64
  str r4, [r1]

  @load the value of the new thread sp 
  ldr sp, [r0]
  @sub sp, sp, #64

  @restore next reg values
  add sp, sp, #64
  @ update the value of the new thread sp in the struct
  str sp, [r0]
  sub sp, sp, #64

  @update the spsr
  ldr r0, [sp, #60]
  msr spsr, r0

  @update the lr^
  @cps 0b10011          //@ supervisor mode
  @ cps 0b11111          //@ system mode
  @ need to go to system mode to access shadow lr
  @ldr lr, [sp, #56]
  @stm r0, {lr}^
  @cps 0b10010          //@ irq mode
  @ ldr pc, _reset_asm
  @ldr r0, [sp, #56]
  @stm r0, {lr}^

  @ldr r0, [sp, #52]
  @bl check_regs
  @mov lr, r0

  @cmp r2, #1
  @bne end
  @pop {r0-r12, lr}
  @mov r0, lr
  @bl check_regs
  @ldr pc, _reset_asm


  @ldr lr, [sp, #56]
  @pop {r0-r12}
  @pop {r0}
  @bl check_regs
  @add sp, sp, #8
  @movs pc, r12

  @idea: ldr pc (address of )
  @ldr lr, [sp, #56]
  @pop {r0-r12}
  @add sp, sp, #12
  @ldr r12, [sp, #-12]
  @movs pc, r12

  pop {r0-r12, lr}
  add sp, sp, #8
  movs pc, lr   @ moves the link register into the pc and implicitly
                @ loads the PC with the result, then copies the 
                @ SPSR to the CPSR.









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
