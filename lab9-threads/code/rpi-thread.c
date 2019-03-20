#include "rpi.h"
#include "rpi-thread.h"

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

	t->sp = &(t->stack[1024 * 8 - 1]);
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
		R0_offset = 0/4, 
		R1_offset = 4/4,
	};

	// write this so that it calls code,arg.
	void rpi_init_trampoline(void);

	// do the brain-surgery on the new thread stack here.
	// LR_offset
	// t->sp = &(t->stack[1024 * 8 - 1]);

	t->sp -= 60 / 4;
	t->sp[LR_offset] = (unsigned)rpi_init_trampoline;
	(t->sp)[R0_offset] = (unsigned)arg;
	(t->sp)[R1_offset] = (unsigned)code;
	(t->sp)[CPSR_offset] = (unsigned)rpi_get_cpsr();

	Q_append(&runq, t);
	runq.n++;

	printk("%d\n", Q_size(&runq));
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
	 
	 rpi_thread_t* dequeued_thread = Q_pop(&runq);
	 if (dequeued_thread == 0) {
 		printk("THREAD: done with all threads, switching to scheduler thread\n");
		rpi_cswitch(&cur_thread->sp, &scheduler_thread->sp);
		cur_thread = scheduler_thread; // will never execute
	} else {
		printk("THREAD: rpi_exit switching to a new thread\n");
		rpi_thread_t* cur_thread = mk_thread();
		runq.n--;
		Q_append(&freeq, cur_thread);
		unsigned * cur = &cur_thread->sp;
		cur_thread = dequeued_thread;
		rpi_cswitch(cur, &dequeued_thread->sp);
		// how to free a single struct (old cur_thread)?
	}
}

// yield the current thread.
void rpi_yield(void) {
	// if cannot dequeue a thread from runq
	//	- there are no more runnable threads, return.  
	// otherwise: 
	//	1. enqueue current thread to runq.
	// 	2. context switch to the new thread.
	// unimplemented();
	rpi_thread_t* dequeued_thread = Q_pop(&runq);
	if (dequeued_thread == 0) {
		printk("THREAD: done with all threads, returning from rpi_yield\n");
		// clean_reboot();
	} else {
		// printk("THREAD: rpi_yield switching to a new thread\n");
		runq.n--;
		Q_append(&runq, cur_thread);
		runq.n++;

		unsigned * cur = &cur_thread->sp;
		cur_thread = dequeued_thread;
		rpi_cswitch(cur, &dequeued_thread->sp);
		// rpi_cswitch(&cur_thread->sp, &dequeued_thread->sp);
	}
}

// starts the thread system: nothing runs before.
// 	- <preemptive_p> = 1 implies pre-emptive multi-tasking.
void rpi_thread_start(int preemptive_p) {
	assert(!preemptive_p);

	// if runq is empty, return.
	// otherswise:
	//    1. create a new fake thread 
	//    2. dequeue a thread from the runq
	//    3. context switch to it, saving current state in
 	//	  <scheduler_thread>
	rpi_thread_t* dequeued_thread = Q_pop(&runq);
	
	if (dequeued_thread == 0) {
		printk("THREAD: done with all threads, returning from rpi_thread_start\n");
		clean_reboot();
	} else {
		printk("THREAD: rpi_thread_start starting thread\n");
		scheduler_thread = mk_thread();
		printk("scheduler_thread = %x\n", scheduler_thread);
		// rpi_thread_t* dequeued_thread = Q_pop(&runq);
		runq.n--;
		cur_thread = dequeued_thread;
		// printk("%d\n", Q_size(&runq));
		printk("cur_thread->sp = %x, scheduler_thread->sp = %x\n", cur_thread->sp, scheduler_thread->sp);
		rpi_cswitch(&scheduler_thread->sp, &cur_thread->sp);
	}
}

// pointer to the current thread.  
//	- note: if pre-emptive is enabled this can change underneath you!
rpi_thread_t *rpi_cur_thread(void) {
	return cur_thread;
}

// call this an other routines from assembler to print out different
// registers!
void check_regs(unsigned r0, unsigned r1, unsigned sp, unsigned lr) {
	printk("r0=%x, r1=%x lr=%x sp=%x\n", r0, r1, lr, sp);
	clean_reboot();
}
