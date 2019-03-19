 # Final Project Proposal
 ### *Nadin El-Yabroudi and Noah Shutty*

 ## Introduction
 Our project consisted of two parts: setting up an RFID card reader that interfaced with an OLED screen and creating preemptive threads. The purpose of the RFID card reader was to support two-factor authentication that would grant users permissions to run certain programs. The preemptive threads were used 

 ## Preemptive Threads
 To create preemptive threads we started with the baseline non-preemptive threads that we did in lab 9. We modified the thread struct to have an array called "regs" where the registers are saved when the thread is preempted. This was easier than writing the registers at the end of the thread's stack because we always knew where to write the registers as opposed to having to calculate it each time we were preempted. We kept a global variable called "cur_thread_reg_array_pointer" which held a known memory address, 0x09000004. At this memory address we stored the address of the register arry for the currently running thread. 

 These are the steps that happened each time we were preempted:
 1. The interrupt table branched to `simpler_interrupt_asm` function (in `lab7-interrupts/timer-int/interrupt-asm.s`)
 2. We subtract 4 from the lr to get the pc where the thread was preempted.
 3. We load the value at address `0x09000004` (the `cur_thread_reg_array_pointer`) into sp, this is the address of the previous thread register array.
 4. In the register array we store values r0-r12, sp^, and lr^ where the ^represents the value of the register in system mode. We also store the lr in the interrupt mode which corresponds to the thread's pc.
 5. We load the spsr onto r0 and call the interrupt_vector function. We move sp to `0x09000000` so that `interrupt_vector` has a regular stack that it can push things onto. If we didn't do this, `interrupt_vector` could override the register array.
 6. `Interrupt_vector` then calls `simpler_int_handler` in `lab9-threads/code/rpi-thread.c`. This function sets the passed in cpsr as the cpsr for the previously running thread. Then it dequeues the next thread, updates `cur_thread_reg_array_pointer` to point to this new thread's register array, and returns the cpsr of this new thread. It also disables interrupts so that we are not preempted infinitely into the interrupt handler.
 7. When we return to `simpler_interrupt_asm` function we save the return spsr.
 8. Then we load the address of the next thread's register array onto sp.
 9. Finally we load the values from the register array onto r0-r12, sp^, lr^, and pc^. This causes a jump to the next thread's pc.

### DNI Bit
 We also added support for a do not interrupt bit. This allows a thread to call `enable_dni()` and `disable_dni()` defined in `rpi-thread.c` between a critical section of the code so that the thread cannot be preempted at this time. To implement this, we created a global dni_addr in rpi-thread.c which points to memory address `0x9000000`, and at this address there is either a one or a zero determining if the DNI bit is on or off. When a thread is preempted, we check the DNI bit at address `0x9000000`, and if it is set we jump to the lr - 4 without chaning any of the registers. This way we recover the state that the thread was in when it was preempted. 

 Another way to do this could have been to disable interrupts when `enable_dni()` was set. However, we decided not to do it this way because if a thread never disabled the dni bit then the thread could starve all other threads. By keeping interrupts enabled the OS could kill a thread that has been running with the dni bit enabled for a long time.
