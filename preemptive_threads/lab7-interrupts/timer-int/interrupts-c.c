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

//void interrupt_vector(unsigned int* addr_of_prev_thread_sp, unsigned int* addr_of_next_thread_sp, unsigned int* prev_thread_sp) {
uint32_t interrupt_vector(uint32_t cpsr){
	// defined by the client.
    uint32_t simpler_int_handler(uint32_t cpsr);
    return simpler_int_handler(cpsr);
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

