/*
	Example of how to use interrupts to interleave two tasks: setting
	a LED smoothly to different percentage of brightness and periodically
	reading sonar distance.
	
	Without interrupts have to decide how to PWM led while you are 
	doing sonar.  Can easily get jagged results (flickering) if you
	don't do it often enough.  Manually putting in code to interleave
	is a pain.

	Current method to fix:
		1. non-interrupt code periodically measures distance
		and computes the off/on decisions in a vector 

		2. interrupt handler cycles through vector doing off/on based
		on it's values.

	You could do the other way: sonar measurements in the int handler,
	pwm in the main code.  Q: tradeoffs?
*/
#include "rpi.h"
#include "timer-interrupt.h"
#include "pwm.h"

// use whatever pins you want.
const unsigned trigger = 12,
	      echo = 16,
	      led = 21;

// useful timeout.
int timeout_read(int pin, int v, unsigned timeout) {
	unsigned start = timer_get_time();
	while(1) {
		if(gpio_read(pin) != v)
			return 1;
		if(timer_get_time() - start > timeout)
			return 0;
	}
}

int distance_in_cm(int t1) {
	return t1/58;
}


#define CHOICES 100
unsigned pwm_choices[CHOICES];
int current_round = 0;

void int_handler(unsigned pc) {
	/*
	   Can have GPU interrupts even though we just enabled timer: 
	   check for timer, ignore if not.

	   p 114:  bits set in reg 8,9
	   These bits indicates if there are bits set in the pending
	   1/2 registers. The pending 1/2 registers hold ALL interrupts
	   0..63 from the GPU side. Some of these 64 interrupts are
	   also connected to the basic pending register. Any bit set
	   in pending register 1/2 which is NOT connected to the basic
	   pending register causes bit 8 or 9 to set. Status bits 8 and
	   9 should be seen as "There are some interrupts pending which
	   you don't know about. They are in pending register 1 /2."
	 */
	volatile rpi_irq_controller_t *r = RPI_GetIRQController();
	if(r->IRQ_basic_pending & RPI_BASIC_ARM_TIMER_IRQ) {


		// do very little in the interrupt handler: just flip
	 	// led off or on.
		gpio_write(led, pwm_choices[current_round % CHOICES]);
		current_round++;

        	/* 
		 * Clear the ARM Timer interrupt - it's the only interrupt 
		 * we have enabled, so we want don't have to work out which 
		 * interrupt source caused us to interrupt 
		 */
        	RPI_GetArmTimer()->IRQClear = 1;
	}
}

void notmain() {
    uart_init();
    install_int_handlers();
    enable_cache();
	timer_interrupt_init(0x4);

    gpio_set_pulldown(echo);
    gpio_set_output(led);
    gpio_set_output(trigger);
    gpio_set_input(echo);


	// do last!
  	system_enable_interrupts();  	// Q: comment out: what happens?

	delay_ms(10);

	while(1) {
	//	2. do a send (again, pay attention to any needed time 
	// 	delays)
		gpio_write(trigger, 1);
		delay_us(10);
		gpio_write(trigger, 0);

	//	3. measure how long it takes and compute round trip
	//	by converting that time to distance using the datasheet
	// 	formula
		while(!gpio_read(echo)){
		}

		int t1 = 0;
		while(gpio_read(echo)) {
			delay_us(1);
			t1++;
		}

		int dist = distance_in_cm(t1);
		if(dist > CHOICES) {
			dist = CHOICES;
		} else if(dist < 0) {
			dist = 0;
		}

		pwm_compute(pwm_choices, dist, CHOICES);
		printk("Distance %d\n", distance_in_cm(t1));

		delay_ms(100);
	}

	system_disable_interrupts();
}
