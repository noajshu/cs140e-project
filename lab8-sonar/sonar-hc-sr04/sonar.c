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
#include "pwm.h"
#include "my-spi.h"


const int trigger_pin = 12;
const int echo_pin = 16;
const int led_pin = 21;

int distance_in_cm(int t1) {
	return t1/58;
}


enum rfid_cmds {
	CMD_GEN_RANDOM_ID = 0b0010,
	CMD_IDLE = 0,
	CMD_TRANSCEIVE = 0b1100
};
enum rfid_regs {
	REG_CMD = 0x01,
	REG_FIFO_DATA = 0x09,
	REG_FIFO_LEVEL = 0x0A,
	REG_TXCTL = 0x14,
	REG_COLL = 0x0e,
	REG_STATUS2 = 0x08,
};
enum rfid_regs_access_modes {
	REG_WRITE = 0 << 7,
	REG_READ = 1 << 7
};

// k = expected number of send bytes
// n = expected number of received bytes
void rfid_transaction(char* inbuf, char* outbuf, unsigned k, unsigned n) {
	spi0->CS = 0xB0;
	// CLEAR RX FIFO (this will discard any data inside)
	while(((1 << 17) & spi0->CS)) {
		char junk = spi0->FIFO;
	}
	while(!((1<<18) & spi0->CS)) {} // TXD full
	unsigned i = 0, j = 0;
	// printk("about to go in ij loop with k=%d, n=%d\n", k, n);
	while ((i < k) || (j < n)) {
		// printk("i=%d, j=%d\n", i, j);
		while((i < k) && ((1 << 18) & spi0->CS)) {
			// TXD can accept data
			spi0->FIFO = inbuf[i] &0xFF;
			i++;
		}
		while((j < n) && ((1 << 17) & spi0->CS)) {
			// RXD has data
			outbuf[j] = spi0->FIFO;
			j++;
		}
	}
	while (!((1 << 16) & spi0->CS)) {} // DONE
	spi0->CS = 0x0;
	return;
}

void rfid_write_reg(char reg, char val) {
	char msg[2];
	msg[0] = REG_WRITE | (reg << 1);
	msg[1] = val;
	return rfid_transaction(msg, (void*)0, 2, 0);
}

char rfid_read_reg(char reg) {
	char msg = REG_READ | (reg << 1);
	char out[1];
	rfid_transaction(&msg, out, 1, 1);
	return out[0];
}


void rfid_init() {
	// check if we need are in "power down" mode
	gpio_set_input(25);
	if(!gpio_read(25)) {
		gpio_set_output(25);
		gpio_write(25, 1);
		delay_ms(50);
	}
	rfid_write_reg(REG_TXCTL, (rfid_read_reg(REG_TXCTL) & ~0b11) | 0b11);
}


void rfid_loopback(char* fifo_data) {

	spi0->CS = 0xB0;
	
	// set command to gen random id
	while (!((1 << 18) & spi0->CS)) {} // TXD
	spi0->FIFO = ((REG_WRITE | (REG_CMD << 1)) &0xFF);
	spi0->FIFO = (CMD_GEN_RANDOM_ID &0xFF);
	while (!((1 << 16) & spi0->CS)) {} // DONE
	while (!((1 << 17) & spi0->CS)) {} // RXD
	while (((1 << 17) & spi0->CS)) {
		printk("spi0->FIFO = %x\n", spi0->FIFO);
	}
	
	delay_ms(10);

	// write to the RFID fifo just for fun
	while (!((1 << 18) & spi0->CS)) {} // TXD
	spi0->FIFO = ((REG_WRITE | (REG_FIFO_DATA << 1)) &0xFF);
	char* msg = "hello from mifare device";
	for(int i=0; i < 24; i++) {
		spi0->FIFO = (msg[i] &0xFF);
	}


	// read from the RFID fifo
	while (!((1 << 18) & spi0->CS)) {} // TXD
	spi0->FIFO = ((REG_READ | (REG_FIFO_LEVEL << 1)) &0xFF);
	while (!((1 << 17) & spi0->CS)) {} // RXD
	unsigned fifo_level = spi0->FIFO;
	for(unsigned i=0; i<fifo_level; i++) {
		spi0->FIFO = ((REG_READ | (REG_FIFO_DATA << 1)) &0xFF);
	}
	while (!((1 << 16) & spi0->CS)) {} // DONE
	while (!((1 << 17) & spi0->CS)) {} // RXD
	int j=0;
	while (((1 << 17) & spi0->CS) && j < 100) {
		fifo_data[j] = spi0->FIFO;
		printk("fifo_data[j] = %x\n", fifo_data[j]);
		j++;
	}
	fifo_data[j] = 0;
	spi0->CS = 0x0;

	return;
}


char* rfid_loopback_simple() {
	char* message = "  hello simple from mifare device!!";
	message[0] = REG_WRITE | (REG_FIFO_DATA << 1);
	char buf[100];
	// put message in fifo
	printk("putting message in fifo\n");
	rfid_transaction(message, (void*)0, strlen(message), 0);
	// retrive message from fifo
	char readcmd[100];
	for(unsigned i=0; i<100; i++)
		readcmd[i] = REG_READ | (REG_FIFO_DATA << 1);
	rfid_transaction(readcmd, buf, strlen(message), strlen(message));

	return message;
}

// 
// 0b00100110
// 0010
// void write_
// unsigned CMD_REQ = 
// int cards_nearby() {
// 
// }


void notmain(void) {
    uart_init();

	printk("enabling spi0\n");
	spi_init();
	printk("enabled spi0\ninitializing rfid");
	rfid_init();
	// // printk("putting soft reset cmd to spi0\n");
	// // spi_putc((unsigned long)0b1111);
	// // printk("putting gen rand ID cmd to spi0\n");
	// // spi_putc((unsigned long)0b0010);
	// // printk("putting transmit cmd to spi0\n");
	// // spi_putc((unsigned long)0b0100);
	// // spi_putc((unsigned long)((0x01 << 1) || 0b1));
	// // printk("getting char from spi0\n");
	// // printk("got '%c'\n", spi_getc());
	// 
	// delay_ms(100);
	// spi0->CS = 0xB0;
	// // gpio_write(8, 0);
	// // should be version
	// for(unsigned i=1; i<55; i++) {
	// 	while (!((1 << 18) & spi0->CS)) {} // TXD
	// 	spi0->FIFO = ((1 << 7 | (i << 1)) &0xFF);
	// }
	// printk("waiting for DONE\n");
	// while (!((1 << 16) & spi0->CS)) {} // DONE
	// // printk("waiting for RXD\n");
	// while (!((1 << 17) & spi0->CS)) {} // RXD
	// while (((1 << 17) & spi0->CS)) {
	// 	printk("spi0->FIFO = %x\n", spi0->FIFO);
	// }
	// spi0->CS = 0x0;
	// delay_ms(300);
	// 
	// char fifo_data[100];
	// rfid_loopback(fifo_data);
	// printk("rfid_loopback = %s\n", fifo_data);
	// printk("rfid_loopback_simple = %s\n", rfid_loopback_simple());


	printk(
		"rfid_read_reg(REG_CMD) & 0b1111 = %b\n",
		rfid_read_reg(REG_CMD) & 0b1111
	);

	printk("setting up transcieve mode\n");
	rfid_write_reg(REG_CMD, (rfid_read_reg(REG_CMD) & ~0b1111) | CMD_TRANSCEIVE);
	rfid_write_reg(REG_COLL, rfid_read_reg(REG_COLL) & ~0x80);
	for(int i=0; i<200; i++) {
		printk(
			"rfid_read_reg(REG_COLL) & 0b11111 = %b\n",
			rfid_read_reg(REG_COLL) & 0b11111
		);
		printk(
			"rfid_read_reg(REG_STATUS2) & 0b111 = %b\n",
			rfid_read_reg(REG_STATUS2) & 0b111
		);
		printk(
			"rfid_read_reg(REG_CMD) & 0b1111 = %b\n",
			rfid_read_reg(REG_CMD) & 0b1111
		);
		delay_ms(100);
	}

#if 0
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
	
#endif
	clean_reboot();
}
