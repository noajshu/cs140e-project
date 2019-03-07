/*
 * sonar, hc-sr04
 *
 * the comments on the sparkfun product page might be helpful.
 *
 * Pay attention to the voltages on:
 *	- Vcc
 *	- Vout.
 * 
 * When circuit is "open" it can be noisy --- is it high or low when open?
 * If high need to configure a pin to be a pullup, if low, pulldown to reject
 * noise.
 */
#include "rpi.h"

const int trigger_pin = 12;
const int echo_pin = 16;
const int led_pin = 21;

int distance_in_cm(int t1) {
	return t1/58;
}

void notmain(void) {
    uart_init();

	printk("starting sonar!\n");

	// 	1. init the device (pay attention to time delays here)
    gpio_set_pulldown(echo_pin);
	gpio_set_input(echo_pin);
	gpio_set_output(trigger_pin);
	gpio_set_output(led_pin);
	delay_ms(10);

	int n = 10000;
	int i = 0;
	int choices = 100;
	while(n) {
		unsigned pwm_choices[choices];
	//	2. do a send (again, pay attention to any needed time 
	// 	delays)
		gpio_write(trigger_pin, 1);
		delay_us(10);
		gpio_write(trigger_pin, 0);

	//	3. measure how long it takes and compute round trip
	//	by converting that time to distance using the datasheet
	// 	formula
		while(!gpio_read(echo_pin)){
		}

		int t1 = 0;
		while(gpio_read(echo_pin)) {
			delay_us(1);
			t1++;
		}
		int dst = distance_in_cm(t1);
		if(dst > choices){
			dst = choices;
		} else if(dst < 0) {
			dst = 0;
		}
		pwm_compute(pwm_choices, dst, choices);
		gpio_write(led_pin, pwm_choices[i % choices]);
		printk("Distance %d\n", distance_in_cm(t1));
		n--;
		i++;
	}


	

	


	//
	//
	// 	4. use the code in gpioextra.h and then replace it with your
	//	own (using the broadcom pdf in the docs/ directory).
	// 
	// troubleshooting:
	//	1. readings can be noisy --- you may need to require multiple
	//	high (or low) readings before you decide to trust the 
	// 	signal.
	//
	// 	2. there are conflicting accounts of what value voltage you
	//	need for Vcc.
	//	
	// 	3. the initial 3 page data sheet you'll find sucks; look for
	// 	a longer one. 

	printk("stopping sonar !\n");
	clean_reboot();
}
