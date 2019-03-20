#include "rpi.h"
#include "rpi-thread.h"
#include "timer-interrupt.h"

//typedef rpi_thread_t E;
#define E rpi_thread_t
#include "Q.h"

int* dni_addr = (int*)0x9000000;
uint32_t** cur_thread_reg_array_pointer = (uint32_t**)0x09000004;
static struct Q runq, freeq;
static unsigned nthreads;

static rpi_thread_t *cur_thread;  // current running thread.
static rpi_thread_t *scheduler_thread; // first scheduler thread.

// call kmalloc if no more blocks, otherwise keep a cache of freed threads.
static rpi_thread_t *mk_thread(void) {
	rpi_thread_t *t;

	if(!(t = Q_pop(&freeq)))
 		t = kmalloc(sizeof(rpi_thread_t));

	t->tid = nthreads;
	nthreads++;

	return t;
}

// create a new thread.
rpi_thread_t *rpi_fork(void (*code)(void *arg), void *arg) {
	rpi_thread_t *t = mk_thread();

	enum { 
		// register offsets are in terms of byte offsets!
		PC_offset = 15,
		LR_offset = 14,  
		SP_offset = 13,
		R0_offset = 0, 
		R1_offset = 1, 
	};

	// write this so that it calls code,arg.
	void rpi_init_trampoline(void);

	// do the brain-surgery on the new thread stack here.
	t->sp = &t->stack[sizeof(t->stack)/sizeof(t->stack[0]) -1];

    t->regs[PC_offset] = (uint32_t)rpi_init_trampoline;
	t->regs[LR_offset] = (uint32_t)rpi_init_trampoline + 4;
	t->regs[SP_offset] = (uint32_t)t->sp;
	t->regs[R1_offset] = (uint32_t)code;
	t->regs[R0_offset] = (uint32_t)arg;
	t->cpsr = rpi_get_cpsr();

	Q_append(&runq, t);
	return t;
}

// exit current thread.
void rpi_exit(int exitcode) {
	Q_append(&freeq, cur_thread);
	rpi_thread_t* previous_thread = cur_thread;
	if(!(cur_thread = Q_pop(&runq))){
		cur_thread = scheduler_thread;
	}
	rpi_cswitch(&previous_thread->sp, &cur_thread->sp);
}

// yield the current thread.
void rpi_yield(void) {
	rpi_thread_t* previous_thread = cur_thread;
	cur_thread = Q_pop(&runq);

	if(!cur_thread) {
		cur_thread = previous_thread;
		return;
	}
	
	Q_append(&runq, previous_thread);
	rpi_cswitch(&previous_thread->sp, &cur_thread->sp);
}

uint32_t simpler_int_handler(uint32_t cpsr, uint32_t sp, uint32_t lr_caret, uint32_t future_pc){
	volatile rpi_irq_controller_t *r = RPI_GetIRQController();
	if(r->IRQ_basic_pending & RPI_BASIC_ARM_TIMER_IRQ) {
		rpi_thread_t* previous_thread = cur_thread;
		previous_thread->cpsr = cpsr;

        cur_thread = Q_pop(&runq);
        if(!cur_thread) {
			cur_thread = previous_thread;
			RPI_GetArmTimer()->IRQClear = 1;
			return 0;
		}
        
		Q_append(&runq, previous_thread);
		RPI_GetArmTimer()->IRQClear = 1;
		*cur_thread_reg_array_pointer = cur_thread->regs;

		printk("Previous thread #%d has values PC: %x, LR %x, SP %x\n", previous_thread->tid, previous_thread->regs[15], previous_thread->regs[14], previous_thread->regs[13]);

		printk("cur thread #%d has values PC: %x, LR %x, SP %x\n", cur_thread->tid, cur_thread->regs[15], cur_thread->regs[14], cur_thread->regs[13]);

		return cur_thread->cpsr;
    }
    return 0;
}

void rpi_thread_start(int preemptive_p) {
	scheduler_thread = mk_thread();

	if(preemptive_p) {
		*dni_addr = 0;
	   install_int_handlers();
	   timer_interrupt_init(7000); // about 3 seconds
	   system_enable_interrupts();
	   *cur_thread_reg_array_pointer = (uint32_t*)scheduler_thread->regs;
	}

    cur_thread = scheduler_thread;
	switch_to_first_thread();

	printk("THREAD: done with all threads, returning\n");
	if(preemptive_p) system_disable_interrupts();
}

// pointer to the current thread.  
//	- note: if pre-emptive is enabled this can change underneath you!
rpi_thread_t *rpi_cur_thread(void) {
	return cur_thread;
}

void enable_dni(){
   *dni_addr = 1;
}

void disable_dni(){
	*dni_addr = 0;
}

void clear_arm_timer_interrupt(){
	RPI_GetArmTimer()->IRQClear = 1;
}

// call this an other routines from assembler to print out different
// registers!
void check_regs(unsigned r0, unsigned r1, unsigned r2, unsigned r3) {
	printk("r0=%x, r1=%x r2=%x r3=%x\n", r0, r1, r2, r3);
	return;
}
