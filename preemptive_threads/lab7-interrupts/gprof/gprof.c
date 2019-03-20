/*
 * use interrupts to implement a simple statistical profiler.
 *	- interrupt code is a replication of ../timer-int/timer.c
 *	- you'll need to implement kmalloc so you can allocate 
 *	  a histogram table from the heap.
 *	- implement functions so that given a pc value, you can increment
 *	  its associated count
 */
#include "rpi.h"
#include "timer-interrupt.h"
extern char __heap_start__;
extern char __bss_start__;


/**********************************************************************
 * trivial kmalloc:  
 *	- do first.  check that its output makes sense!
 */

// useful for rounding up.   e.g., roundup(n,8) to roundup <n> to 8 byte
// alignment.
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))
void * heapPointer;
/*
 * return a memory block of at least size <nbytes>
 *	- use to allocate gprof code histogram table.
 *	- note: there is no free, so is trivial.
 * 	- should be just a few lines of code.
 */
void *kmalloc(unsigned nbytes) {
	int * result = heapPointer;
    heapPointer += nbytes;
    for(int i = 0; i < nbytes; i++) {
    	result[i] = 0;
    }
    return result;
}

/*
 * one-time called before kmalloc to setup heap.
 * 	- should be just a few lines of code.
 */
void kmalloc_init(void) {
	heapPointer = (int *) roundup((int)&__heap_start__, 8);
	assert((int)heapPointer % 8 == 0);
}

/***************************************************************************
 * gprof implementation:
 *	- allocate a table with one entry for each instruction.
 *	- gprof_init(void) - call before starting.
 *	- gprof_inc(pc) will increment pc's associated entry.
 *	- gprof_dump will print out all samples.
 */
int * gprof_table;
int num_instr;

// allocate table.
//    few lines of code
static unsigned gprof_init(void) {
	num_instr = (int)(&__bss_start__ - 0x8000)/4;
	gprof_table = (int *)kmalloc(num_instr);
	return 0;
}

// increment histogram associated w/ pc.
//    few lines of code
static void gprof_inc(unsigned pc) {
	if(pc < 0x8000 || pc > (int)&__bss_start__) {
		panic("Invalid program counter");
	}
	gprof_table[(pc - 0x8000)/4] += 1;
}

// print out all samples whose count > min_val
//
// make sure sampling does not pick this code up!
static void gprof_dump(unsigned min_val) {
	system_disable_interrupts();
	for(int i = 0; i < num_instr; i++) {
		if(gprof_table[i] > min_val)
			printk("pc: %x, val: %d\n", 0x8000 + i*4, gprof_table[i]);
	}
	system_enable_interrupts();
}


/***********************************************************************
 * timer interrupt code from before, now calls gprof update.
 */
// Q: if you make not volatile?
static volatile unsigned cnt;
static volatile unsigned period;

// client has to define this.
void int_handler(unsigned pc) {
	unsigned pending = RPI_GetIRQController()->IRQ_basic_pending;

	// if this isn't true, could be a GPU interrupt: just return.
	if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
		return;

        /* 
	 * Clear the ARM Timer interrupt - it's the only interrupt we have
         * enabled, so we want don't have to work out which interrupt source
         * caused us to interrupt 
	 *
	 * Q: if we delete?
	 */
        RPI_GetArmTimer()->IRQClear = 1;
	cnt++;

        gprof_inc(pc);

	static unsigned last_clk = 0;
	unsigned clk = timer_get_time();
	period = last_clk ? clk - last_clk : 0;
	last_clk = clk;
	
	// Q: if we put a print statement?
}

// trivial program to test gprof implementation.
// 	- look at output: do you see weird patterns?
void notmain() {
	uart_init();
	
	printk("about to install handlers\n");
        install_int_handlers();

	printk("setting up timer interrupts\n");
	// Q: if you change 0x100?
	timer_interrupt_init(0x10);

	// could combine some of these.
	kmalloc_init();
	gprof_init();

	// Q: if you don't do?
	printk("gonna enable ints globally!\n");
  	system_enable_interrupts();
	printk("enabled!\n");

	// enable_cache(); 	// Q: what happens if you enable cache?
        unsigned iter = 0;
        while(cnt<200) {
                printk("iter=%d: cnt = %d, period = %dusec, %x\n",
                                iter,cnt, period,period);
                iter++;
                if(iter % 10 == 0)
                        gprof_dump(2);
        }
	clean_reboot();
}
