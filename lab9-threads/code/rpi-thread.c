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
		CPSR_offset = 60/4,
		R0_offset = 0, 
		R1_offset = 4/4, 
	};

	// write this so that it calls code,arg.
	void rpi_init_trampoline(void);

	// do the brain-surgery on the new thread stack here.
	t->sp = &t->stack[sizeof(t->stack)/sizeof(t->stack[0]) -1];
	t->sp -= 64/4;
	t->sp[LR_offset] = (uint32_t)rpi_init_trampoline;
	t->sp[R1_offset] = (uint32_t)code;
	t->sp[R0_offset] = (uint32_t)arg;
	t->sp[CPSR_offset] = rpi_get_cpsr();

	printk("rpi_fork with thread %x\n", t);
	Q_append(&runq, t);
	//printk("runq @ %x\n", &runq);
	//printk("runq.head = %x, runq.tail = %x\n", runq.head, runq.tail);
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

void int_handler(unsigned int* prev_thread_sp, unsigned int* next_thread_sp) {
    /*Code here should decide whether to preempt or not and to which 
    thread to preempt to*/
	printk("in int_handler\n");
	volatile rpi_irq_controller_t *r = RPI_GetIRQController();
	if(r->IRQ_basic_pending & RPI_BASIC_ARM_TIMER_IRQ) {
		printk("Preemption, switching threads!\n");
		rpi_thread_t* previous_thread = cur_thread;
		//printk("runq @ %x\n", &runq);
		//printk("runq.head = %x, runq.tail = %x\n", runq.head, runq.tail);
		cur_thread = Q_pop(&runq);
		//printk("got cur_thread = %x from runq\n", cur_thread);

		if(!cur_thread) {
			//printk("no thread to switch to, going back\n");
			cur_thread = previous_thread;
			RPI_GetArmTimer()->IRQClear = 1;
			return;
		}
		
		Q_append(&runq, previous_thread);
		/* Clear the ARM Timer interrupt - it's the only interrupt 
		* we have enabled, so we want don't have to work out which 
		* interrupt source caused us to interrupt */
		// printk("switching off irq\n");
		RPI_GetArmTimer()->IRQClear = 1;
		//return if we should preempt or not
		printk(
			"previous_thread # %x, next thread @ %x\n",
			previous_thread,
			cur_thread->sp
		);
		printk(
			"previous_thread sp %x, next thread sp %x\n",
			previous_thread->sp,
			cur_thread->sp
		);
		printk("switching to thread %d\n", cur_thread->tid);
		*prev_thread_sp = (uint32_t)&previous_thread->sp;
		*next_thread_sp = (uint32_t)&cur_thread->sp;
	}
}

// starts the thread system: nothing runs before.
// 	- <preemptive_p> = 1 implies pre-emptive multi-tasking.
void rpi_thread_start(int preemptive_p) {
	// if runq is empty, return.
	// otherwise:
	//    1. create a new fake thread 
	//    2. dequeue a thread from the runq
	//    3. context switch to it, saving current state in
 	//	  <scheduler_thread>
	scheduler_thread = mk_thread();
    cur_thread = Q_pop(&runq);

	if(!cur_thread) return;

	if(preemptive_p) {
	   install_int_handlers();
	   //timer_interrupt_init(0x4); // 4 cycles
	   timer_interrupt_init(7000); // about 3 seconds
	   system_enable_interrupts();
	}

	
	//printk("runq @ %x\n", &runq);
	//printk("runq.head = %x, runq.tail = %x\n", runq.head, runq.tail);
	// while (1) {
	// 	printk("this is the scheduler thread on an infinite loop\n");
	// 	delay_ms(1000);
	// }
	// don't want to use this since it stores state in
	// an incompatible 60 byte format on the stack
	//printk("rpi_thread_start, about to rpi_cswitch\n");
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
void check_regs(unsigned r0, unsigned r1, unsigned r2, unsigned r3) {
	printk("r0=%x, r1=%x r2=%x r3=%x\n", r0, r1, r2, r3);
	return;
	//clean_reboot();
}
