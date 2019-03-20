/* 
 * interrupts-c.c: some interrupt support code.  Default handlers, 
 * interrupt installation.
 */
#include "rpi.h"
#include "rpi-interrupts.h"

/*
 * Copy in interrupt vector table and FIQ handler _table and _table_end
 * are symbols defined in the interrupt assembly file, at the beginning
 * and end of the table and its embedded constants.
 */
void install_int_handlers(void) {
        printk("install_int_handlers\n");
        unsigned *dst = (void*)RPI_VECTOR_START,
                 *src = &_interrupt_table,
                 n = &_interrupt_table_end - src;
        for(int i = 0; i < n; i++)
                dst[i] = src[i];
}

void interrupt_vector(){
	// defined by the client.
    void simpler_int_handler();
    simpler_int_handler();
}

#define UNHANDLED(msg,r) \
	panic("ERROR: unhandled exception <%s> at PC=%x\n", msg,r)
void fast_interrupt_vector(unsigned pc) {
	UNHANDLED("fast", pc);
}
void software_interrupt_vector(unsigned pc) {
	UNHANDLED("soft interrupt", pc);
}
void reset_vector(unsigned pc) {
	UNHANDLED("reset vector", pc);
}
void undefined_instruction_vector(unsigned pc) {
	UNHANDLED("undefined instruction", pc);
}
void prefetch_abort_vector(unsigned pc) {
	UNHANDLED("prefetch abort", pc);
}
void data_abort_vector(unsigned pc) {
	UNHANDLED("data abort", pc);
}

