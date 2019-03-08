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
  //-- irq mode
  sub lr, lr, #4 // in IRQ mode, r14_irq(lr_irq) points to PC+#4 in user mode
  //-- save context
  push {r0-r12, lr} // save user mode registers

  //spsr is like cpsr but in interrupt mode
  mrs r0, spsr // spsr -> r0
  cps #0x13
  //-- supervisor mode
  mov r1, sp
  mov r2, lr

  cps #0x12
  //-- irq mode
  //-- we will use these in _IRQ_interrupt_context_switch
  push {r0-r2} // save spsr, user mode sp, lr

  // call IRQ_hander(user-mode-sp)
  mov r0, r2
  bl  interrupt_vector
  cmp r0, #0
  bne _IRQ_interrupt_context_switch

  pop {r0-r2} // restore spsr, user mode sp, lr
  msr spsr, r0 // r0 -> spsr
  cps #0x13
  //-- supervisor mode
  mov sp, r1 // restore sp
  mov lr, r2 // restore lr

  cps #0x12
  //-- irq mode
  pop  {r0-r12,lr}
  movs pc, lr



_IRQ_interrupt_context_switch:
  //-- irq mode
  // r0 is next threads SP

  pop {r1-r3} // restore spsr, user mode sp, lr

  // save registers in user mode stack
  // r1: user mode cpsr
  // r2: user mode sp
  // r3: user mode lr
  sub r2, r2, #4
  str r1, [r2] // spsr

//TODO: confused about how we know that we should load r4 from there to store
  ldr r4, [sp, #4*13]
  sub r2, r2, #4
  str r4, [r2] // user mode pc (r14_irq)

  sub r2, r2, #4
  str r3, [r2] // user mode lr

  ldr r4, [sp, #4*12]
  sub r2, r2, #4
  str r4, [r2] // user mode r12

  ldr r4, [sp, #4*11]
  sub r2, r2, #4
  str r4, [r2] // user mode r11

  ldr r4, [sp, #4*10]
  sub r2, r2, #4
  str r4, [r2] // user mode r10

  ldr r4, [sp, #4*9]
  sub r2, r2, #4
  str r4, [r2] // user mode r9

  ldr r4, [sp, #4*8]
  sub r2, r2, #4
  str r4, [r2] // user mode r8

  ldr r4, [sp, #4*7]
  sub r2, r2, #4
  str r4, [r2] // user mode r7

  ldr r4, [sp, #4*6]
  sub r2, r2, #4
  str r4, [r2] // user mode r6

  ldr r4, [sp, #4*5]
  sub r2, r2, #4
  str r4, [r2] // user mode r5

  ldr r4, [sp, #4*4]
  sub r2, r2, #4
  str r4, [r2] // user mode r4

  ldr r4, [sp, #4*3]
  sub r2, r2, #4
  str r4, [r2] // user mode r3

  ldr r4, [sp, #4*2]
  sub r2, r2, #4
  str r4, [r2] // user mode r2

  ldr r4, [sp, #4*1]
  sub r2, r2, #4
  str r4, [r2] // user mode r1

  ldr r4, [sp]
  sub r2, r2, #4
  str r4, [r2] // user mode r0

  mov r4, sp // r4 <- sp_irq

  msr spsr, r1 // r1 -> spsr
  cps #0x13          //@ svc mode
  //-- svc mode
  // change user mode stack to next threads stack
  mov sp, r0
  // push into sp_irq (r4)
  ldr r2, [sp, #4*14] // user mode pc
  sub r4, r4, #4
  str r2, [r4]
  ldr r2, [sp, #4*15] // spsr
  sub r4, r4, #4
  str r2, [r4]
  // restore registers
  pop {r0-r12,lr}
  add sp, sp, #4*2 // pop pc, spsr

  cps #0x12          //@ irq mode
  //-- irq mode
  sub sp, sp, #4*2
  pop {lr}
  // enable irq and restore spsr
  bic lr, lr, #0x80
  msr spsr, lr // lr -> spsr
  // restore lr (user mode pc)
  pop {lr}
  // discard pushed registers
  add sp, sp, #4*14
  // movs pc, * ... movs pc restores status register
  movs pc, lr




  @ mov sp, #0x8000 @ what dwelch uses: bad if int handler uses deep stack
  @mov sp, #0x9000000  @ i believe we have 512mb - 16mb, so this should be safe
  @sub   lr, lr, #4

  @push  {r0-r12,lr}         @ XXX: pushing too many registers: only need caller
  @ vpush {s0-s15}	    @ uncomment if want to save caller-saved fp regs

  @mov   r0, lr              @ Pass old pc
  @bl    interrupt_vector    @ C function

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
  bl    reset_vector
undefined_instruction_asm:
  sub   lr, lr, #4
  bl    undefined_instruction_vector
software_interrupt_asm:
  sub   lr, lr, #4
  bl    software_interrupt_vector
prefetch_abort_asm:
  sub   lr, lr, #4
  bl    prefetch_abort_vector
data_abort_asm:
  sub   lr, lr, #4
  bl    data_abort_vector
