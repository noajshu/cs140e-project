#include "rpi.h"
#include "rpi-thread.h"
#include "timer-interrupt.h"

// typedef rpi_thread_t E;
#define E rpi_thread_t
#include "Q.h"

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

	// you can use these for setting the values in the first thread.
	// if your context switching code stores these values at different
	// stack offsets, change them!
	enum { 
		// register offsets are in terms of byte offsets!
		LR_offset = 52/4,  
		CPSR_offset = 56/4,
		R0_offset = 0, 
		R1_offset = 4/4, 
	};

	// write this so that it calls code,arg.
	void rpi_init_trampoline(void);

	// do the brain-surgery on the new thread stack here.
	t->sp = &t->stack[sizeof(t->stack)/sizeof(t->stack[0]) -1];
	t->sp -= 60/4;
	t->sp[LR_offset] = rpi_init_trampoline;
	t->sp[R1_offset] = code;
	t->sp[R0_offset] = arg;
	t->sp[CPSR_offset] = rpi_get_cpsr();

	Q_append(&runq, t);
	return t;
}

// exit current thread.
void rpi_exit(int exitcode) {
	/*
	 * 1. free current thread.
	 *
	 * 2. if there are more threads, dequeue one and context
 	 * switch to it.
	 
	 * 3. otherwise we are done, switch to the scheduler thread 
	 * so we call back into the client code.
	 */
	Q_append(&freeq, cur_thread);
	rpi_thread_t* previous_thread = cur_thread;
	if(!(cur_thread = Q_pop(&runq))){
		cur_thread = scheduler_thread;
	}
	rpi_cswitch(&previous_thread->sp, &cur_thread->sp);
}

// yield the current thread.
void rpi_yield(void) {
	// if cannot dequeue a thread from runq
	//	- there are no more runnable threads, return.  
	// otherwise: 
	//	1. enqueue current thread to runq.
	// 	2. context switch to the new thread.
	rpi_thread_t* previous_thread = cur_thread;
	cur_thread = Q_pop(&runq);

	if(!cur_thread) {
		cur_thread = previous_thread;
		return;
	}
	
	Q_append(&runq, previous_thread);
	rpi_cswitch(&previous_thread->sp, &cur_thread->sp);
}

unsigned int* int_handler(unsigned pc) {
    /*Code here should decide whether to preempt or not and to which 
    thread to preempto to*/
	volatile rpi_irq_controller_t *r = RPI_GetIRQController();
	if(r->IRQ_basic_pending & RPI_BASIC_ARM_TIMER_IRQ) {
		printk("Preemption, switching threads!\n");
		rpi_thread_t* previous_thread = cur_thread;
		cur_thread = Q_pop(&runq);

		if(!cur_thread) {
			cur_thread = previous_thread;
			RPI_GetArmTimer()->IRQClear = 1;
			return 0;
		}
		
		Q_append(&runq, previous_thread);
		/* Clear the ARM Timer interrupt - it's the only interrupt 
		* we have enabled, so we want don't have to work out which 
		* interrupt source caused us to interrupt */
		// printk("switching off irq\n");
		RPI_GetArmTimer()->IRQClear = 1;
		return cur_thread->sp;
	}
}

// starts the thread system: nothing runs before.
// 	- <preemptive_p> = 1 implies pre-emptive multi-tasking.
void rpi_thread_start(int preemptive_p) {
	if(preemptive_p) {
       install_int_handlers();
       timer_interrupt_init(0x4);
       system_enable_interrupts();
	}

	// if runq is empty, return.
	// otherwise:
	//    1. create a new fake thread 
	//    2. dequeue a thread from the runq
	//    3. context switch to it, saving current state in
 	//	  <scheduler_thread>
	scheduler_thread = mk_thread();
    cur_thread = Q_pop(&runq);

	if(!cur_thread) return;
	rpi_cswitch(&scheduler_thread->sp, &cur_thread->sp);

	printk("THREAD: done with all threads, returning\n");
	system_disable_interrupts();
}

// pointer to the current thread.  
//	- note: if pre-emptive is enabled this can change underneath you!
rpi_thread_t *rpi_cur_thread(void) {
	return cur_thread;
}

// call this an other routines from assembler to print out different
// registers!
void check_regs(unsigned r0, unsigned r1, unsigned sp, unsigned lr) {
	printk("r0=%x, r1=%x lr=%x cpsr=%x\n", r0, r1, lr, sp);
	clean_reboot();
}
