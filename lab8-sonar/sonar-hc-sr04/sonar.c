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
#include "spi02.h"
#include "rpi-thread.h"
#include "MFRC522.h"


const int trigger_pin = 12;
const int echo_pin = 16;
const int led_pin = 21;

int distance_in_cm(int t1) {
	return t1/58;
}

enum picc_cmds {
	PICC_CMD_REQA = 0x26,
	PICC_CMD_WUPA = 0x52
};

enum rfid_cmds {
	CMD_GEN_RANDOM_ID = 0b0010,
	CMD_IDLE = 0,
	CMD_TRANSCEIVE = 0b1100
};

enum rfid_regs {
	REG_CMD = 0x01,
	REG_COMIEN = 0x02,
	REG_DIVIEN = 0x03,
	REG_COM_IRQ = 0x04,
	REG_DIV_IRQ = 0x05,
	REG_ERROR = 0x06,
	REG_STATUS1 = 0x07,
	REG_STATUS2 = 0x08,
	REG_FIFO_DATA = 0x09,
	REG_FIFO_LEVEL = 0x0A,
	REG_CTL = 0x0c,
	REG_MODE = 0x11,
	REG_TX_MODE = 0x12,
	REG_RX_MODE = 0x13,
	REG_TXCTL = 0x14,
	REG_TX_ASK = 0x15,
	REG_BIT_FRAMING = 0x0d,
	REG_COLL = 0x0e,
	REG_TMODE = 0x2a,
	REG_TPRESCALER = 0x2b,
	REG_T_COUNTER_VAL_HI = 0x2e,
	REG_T_COUNTER_VAL_LO = 0x2f,
	REG_T_RELOAD_HI = 0x2c,
	REG_T_RELOAD_LO = 0x2d,
	REG_VERSION = 0x37
};

enum rfid_regs_access_modes {
	REG_WRITE = 0 << 7,
	REG_READ = 1 << 7
};

// k = expected number of send bytes
// n = expected number of received bytes
void rfid_transaction(char* inbuf, char* outbuf, unsigned k, unsigned n) {
	chip_select(1);
	transfer_active(1);
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
	char msg[2];
	msg[0] = REG_READ | (reg << 1);
	msg[1] = 0;
	char out[2];
	rfid_transaction(msg, out, 2, 2);
	return out[1];
}

#define RFID_RESET_PIN 5
void rfid_init() {
	// check if we need are in "power down" mode
	gpio_set_input(RFID_RESET_PIN);
	if(!gpio_read(RFID_RESET_PIN)) {
		gpio_set_output(RFID_RESET_PIN);
		gpio_write(RFID_RESET_PIN, 0);
		delay_ms(50);
		gpio_write(RFID_RESET_PIN, 1);
		delay_ms(50);
	}
	// DISABLE RCVOFF
	rfid_write_reg(REG_CMD, (rfid_read_reg(REG_CMD) & ~0b101111) | CMD_IDLE);
	// while(rfid_read_reg(REG_CMD & (1 << 4))) {
	// 	// wait for exit from soft power down mode
	// 	printk("waiting for exit of soft power-down mode\n");
	// }
}

void rfid_loopback_simple(char*buf) {
	char* message = "  hello simple from mifare device!!";
	message[0] = REG_WRITE | (REG_FIFO_DATA << 1);
	// put message in fifo
	printk("putting message in fifo\n");
	rfid_transaction(message, (void*)0, strlen(message), 0);
	// retrive message from fifo
	char readcmd[100];
	for(unsigned i=0; i<100; i++)
		readcmd[i] = REG_READ | (REG_FIFO_DATA << 1);
	rfid_transaction(readcmd, buf, strlen(message), strlen(message));
	buf[strlen(message)] = 0;
	return;
}

void rfid_loopback_regs(char*buf) {
	char* message = "hello simple from mifare device!!";
	// message[0] = REG_WRITE | (REG_FIFO_DATA << 1);
	// // put message in fifo
	// printk("putting message in fifo\n");
	// rfid_transaction(message, (void*)0, strlen(message), 0);
	// // retrive message from fifo
	// char readcmd[100];
	// for(unsigned i=0; i<100; i++)
	// 	readcmd[i] = REG_READ | (REG_FIFO_DATA << 1);
	// rfid_transaction(readcmd, buf, strlen(message), strlen(message));
	unsigned i=0;
	while (i<strlen(message)) {
		rfid_write_reg(REG_FIFO_DATA, message[i]);
		i++;
		// printk("rfid_read_reg(REG_FIFO_LEVEL) = %b\n", rfid_read_reg(REG_FIFO_LEVEL));
	}
	buf[i]=0;
	// printk("rfid_read_reg(REG_FIFO_LEVEL) = %b\n", rfid_read_reg(REG_FIFO_LEVEL));
	i=0;
	unsigned level = rfid_read_reg(REG_FIFO_LEVEL) & 0b01111111;
	while(i < level) {
		// printk("reading from FIFO\n");
		buf[i] = rfid_read_reg(REG_FIFO_DATA);
		i++;
	}
	buf[i]=0;
	return;
}


void tramp(void* args) {
	while(1){}
}

void detect_rfid_card(void* args) {
	while(1) {
		// do register manipulation of MIFARE
		// to check for a card
		// maybe do critical section protection (DNI)
		printk("I am checking the RFID card rigt now.\n");
		delay_ms(1000);
	}
}

void update_display(void* args) {
	while(1) {
		printk("I am updating the OLED display with information.\n");
		delay_ms(1000);
	}
}
void oled_reset(void) {
	#define OLED_RST_PIN 6
	gpio_set_output(OLED_RST_PIN);
	gpio_write(OLED_RST_PIN, 1);
	delay_ms(50);
	gpio_write(OLED_RST_PIN, 0);
	delay_ms(50);
	gpio_write(OLED_RST_PIN, 1);
	delay_ms(50);
}

void notmain(void) {
    uart_init();

	printk("enabling spi0\n");
	spi_init();
	printk("initializing rfid\n");
	rfid_init();
	printk("initializing oled\n");
	oled_reset();
	oled_init();
    show_text(0,"rfid OS");
    // show_text(1,"pls scan");
	
	
	// rpi_fork(tramp, 0);
	// rpi_fork(detect_rfid_card, 0);
	// rpi_fork(update_display, 0);
	// rpi_thread_start(1);
	// 
	
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
	
	char buf1[100];
	rfid_loopback_simple(buf1);
	printk("rfid_loopback_simple = %s\n", buf1);
	char buf2[100];
	rfid_loopback_regs(buf2);
	printk("rfid_loopback_regs = %s\n", buf2);
	// clean_reboot();

	printk(
		"rfid_read_reg(REG_CMD) & 0b1111 = %b\n",
		rfid_read_reg(REG_CMD) & 0b1111
	);

	rfid_write_reg(REG_TMODE, 0x80);			// TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
    rfid_write_reg(REG_TPRESCALER, 0xA9);	// TPreScaler = TModeReg[3..0]:TPrescalerReg, ie 0x0A9 = 169 => f_timer=40kHz, ie a timer period of 25�s.
    rfid_write_reg(REG_T_RELOAD_HI, 0x04);		// Reload timer with 0x3E8 = 1000, ie 25ms before timeout.
    rfid_write_reg(REG_T_RELOAD_LO, 0xE8);
	rfid_write_reg(REG_TX_ASK, 0x40);		// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
	rfid_write_reg(REG_MODE, 0x3D);	

	// output signal on pin TX1 & TX2 delivers the 13.56 MHz energy carrier modulated by the transmission data
	rfid_write_reg(REG_TXCTL, rfid_read_reg(REG_TXCTL) | 0b11);

	printk("setting up transcieve mode\n");
	// for(int i=0; i<10; i++) {
	while(1) {
		printk("\n\n");
		// rfid_write_reg(REG_CMD, (rfid_read_reg(REG_CMD) & ~0b101111) | CMD_IDLE);
		rfid_write_reg(REG_CMD, (rfid_read_reg(REG_CMD) & ~0b1111) | CMD_IDLE);
		// clear all interrupts
		rfid_write_reg(REG_COM_IRQ, 0b1111111);

		// // not sure what collisions about about
		// rfid_write_reg(REG_COLL, rfid_read_reg(REG_COLL) & ~0x80);

		// // output signal on pin TX2 continuously delivers the unmodulated 13.56 MHz energy carrier
		// rfid_write_reg(REG_TXCTL, (rfid_read_reg(REG_TXCTL) & ~0b111) | 0b100);

		// // so that the timer starts immediately after transmitting
		// rfid_write_reg(REG_MODE, rfid_read_reg(REG_MODE) & ~(1<<5));

		// rfid_write_reg(REG_TMODE, )

		// put the wupa command (wake-up) in the FIFO
		// rfid_write_reg(REG_FIFO_DATA, PICC_CMD_WUPA);
		rfid_write_reg(REG_FIFO_DATA, PICC_CMD_REQA);

		// only transmit 7 bits
		rfid_write_reg(REG_BIT_FRAMING, (rfid_read_reg(REG_BIT_FRAMING) &  ~0b111) | 0b111);

		// rfid_write_reg(REG_CMD, (rfid_read_reg(REG_CMD) & ~0b101111) | CMD_TRANSCEIVE);

		// do not allow incomplete / invalid data receipt
		rfid_write_reg(REG_RX_MODE, rfid_read_reg(REG_RX_MODE) | (1<<3));
		
		// // allow incomplete / invalid data receipt
		// rfid_write_reg(REG_RX_MODE, rfid_read_reg(REG_RX_MODE) & ~(1<<3));

		// set cmd to transceive
		rfid_write_reg(REG_CMD, (rfid_read_reg(REG_CMD) & ~0b1111) | CMD_TRANSCEIVE);

		// begin transmission
		rfid_write_reg(REG_BIT_FRAMING, rfid_read_reg(REG_BIT_FRAMING) | (1<<7));
		// do {
		// 	// not timed out yet
		// 	printk(
		// 		"rfid_read_reg(REG_COM_IRQ) = %b\n",
		// 		rfid_read_reg(REG_COM_IRQ)
		// 	);
		// 	printk(
		// 		"rfid_read_reg(REG_STATUS2) = %b\n",
		// 		rfid_read_reg(REG_STATUS2)
		// 	);
		// } while(!(
		// 	(rfid_read_reg(REG_COM_IRQ) & 1) // timeout
		// 	// || (rfid_read_reg(REG_COM_IRQ) & (1 << 5)) // end of received stream
		// ));
		delay_ms(100);
		printk(
			// TODO verify that RxMultiple is set to 0
			"rfid_read_reg(REG_RX_MODE) = %b\n",
			rfid_read_reg(REG_RX_MODE)
		);
		printk(
			// TODO verify that RxLastBits is 0
			"rfid_read_reg(REG_CTL) = %b\n",
			rfid_read_reg(REG_CTL)
		);
		printk(
			// TODO verify that Tx1 and tx2 are on
			"rfid_read_reg(REG_TXCTL) = %b\n",
			rfid_read_reg(REG_TXCTL)
		);
		printk(
			"rfid_read_reg(REG_COLL) = %b\n",
			rfid_read_reg(REG_COLL)
		);
		printk(
			"rfid_read_reg(REG_BIT_FRAMING) = %b\n",
			rfid_read_reg(REG_BIT_FRAMING)
		);
		printk(
			"rfid_read_reg(REG_COM_IRQ) = %b\n",
			rfid_read_reg(REG_COM_IRQ)
		);
		printk(
			"rfid_read_reg(REG_FIFO_LEVEL) = %b\n",
			rfid_read_reg(REG_FIFO_LEVEL)
		);
		printk(
			"rfid_read_reg(REG_STATUS1) = %b\n",
			rfid_read_reg(REG_STATUS1)
		);
		printk(
			"rfid_read_reg(REG_STATUS2) = %b\n",
			rfid_read_reg(REG_STATUS2)
		);
		printk(
			"rfid_read_reg(REG_CMD) = %b\n",
			rfid_read_reg(REG_CMD)
		);
		printk(
			"rfid_read_reg(REG_ERROR) = %b\n",
			rfid_read_reg(REG_ERROR)
		);
		printk(
			"rfid_read_reg(REG_TMODE) = %b\n",
			rfid_read_reg(REG_TMODE)
		);
		printk(
			"rfid_read_reg(REG_MODE) = %b\n",
			rfid_read_reg(REG_MODE)
		);
		printk(
			"rfid_read_reg(REG_VERSION) = %x\n",
			rfid_read_reg(REG_VERSION)
		);
		printk(
			"rfid_read_reg(REG_T_COUNTER_VAL_LO) = %b\n",
			rfid_read_reg(REG_T_COUNTER_VAL_LO)
		);
		printk(
			"rfid_read_reg(REG_T_COUNTER_VAL_HI) = %b\n",
			rfid_read_reg(REG_T_COUNTER_VAL_HI)
		);
		printk(
			"rfid_read_reg(REG_DIV_IRQ) = %b\n",
			rfid_read_reg(REG_DIV_IRQ)
		);
		// show_text_hex(0, rfid_read_reg(REG_STATUS1));
		// show_text_hex(1, rfid_read_reg(REG_STATUS2));
		// show_text_hex(2, rfid_read_reg(REG_CMD));
		// show_text_hex(3, rfid_read_reg(REG_COM_IRQ));
		if(rfid_read_reg(REG_FIFO_LEVEL)) {
			printk("o joyous day\n");
			show_text(2, "card detect");
			clean_reboot();
		}
		delay_ms(500);
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
