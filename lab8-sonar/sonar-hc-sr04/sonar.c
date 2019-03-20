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


const unsigned BUTTON_HI_PIN = 4;
const unsigned OLED_RST_PIN = 6;


const int trigger_pin = 12;
const int echo_pin = 16;
const int led_pin = 21;

int distance_in_cm(int t1) {
	return t1/58;
}

void oled_reset(void) {
	gpio_set_output(OLED_RST_PIN);
	gpio_write(OLED_RST_PIN, 1);
	delay_ms(50);
	gpio_write(OLED_RST_PIN, 0);
	delay_ms(50);
	gpio_write(OLED_RST_PIN, 1);
	delay_ms(50);
}

enum BUTTONS {
	// these define the GPIO pins used
	BUTTON_0 = 23,
	BUTTON_1 = 18,
	BUTTON_2 = 22,
	BUTTON_3 = 27,
	BUTTON_4 = 17
};

enum {
	ASCII_0 = 48,
	ASCII_1 = 49,
	ASCII_2 = 50,
	ASCII_3 = 51,
	ASCII_4 = 52,
	ASCII_5 = 53,
	ASCII_6 = 54,
	ASCII_7 = 55,
	ASCII_8 = 56,
	ASCII_9 = 57
};


void button_init() {
	gpio_set_input(BUTTON_0);
	gpio_set_input(BUTTON_1);
	gpio_set_input(BUTTON_2);
	gpio_set_input(BUTTON_3);
	gpio_set_input(BUTTON_4);

	gpio_set_output(BUTTON_HI_PIN);
	gpio_write(BUTTON_HI_PIN, 1);
	// from now on, do gpio_read(button)
	// to detect a button press
}


const unsigned sticky_key_wait_ms = 100;
char await_button_value() {
	while(1) {
		if (gpio_read(BUTTON_0)) {
			while(gpio_read(BUTTON_0)){}
			delay_ms(sticky_key_wait_ms);
			return ASCII_0;
		}
		if (gpio_read(BUTTON_1)) {
			while(gpio_read(BUTTON_1)){}
			delay_ms(sticky_key_wait_ms);
			return ASCII_1;
		}
		if (gpio_read(BUTTON_2)) {
			while(gpio_read(BUTTON_2)){}
			delay_ms(sticky_key_wait_ms);
			return ASCII_2;
		}
		if (gpio_read(BUTTON_3)) {
			while(gpio_read(BUTTON_3)){}
			delay_ms(sticky_key_wait_ms);
			return ASCII_3;
		}
		if (gpio_read(BUTTON_4)) {
			while(gpio_read(BUTTON_4)){}
			delay_ms(sticky_key_wait_ms);
			return ASCII_4;
		}
	}
}

void display_current_buttons_pressed() {
	char * button_msg = kmalloc(16*8);
	if(gpio_read(BUTTON_0) || gpio_read(BUTTON_1) || gpio_read(BUTTON_2) || gpio_read(BUTTON_3) || gpio_read(BUTTON_4)) {
		strcpy(button_msg, "buttons         ");
		if (gpio_read(BUTTON_0)) {
			button_msg[8] = ASCII_0;
		}
		if (gpio_read(BUTTON_1)) {
			button_msg[9] = ASCII_1;
		}
		if (gpio_read(BUTTON_2)) {
			button_msg[10] = ASCII_2;
		}
		if (gpio_read(BUTTON_3)) {
			button_msg[11] = ASCII_3;
		}
		if (gpio_read(BUTTON_4)) {
			button_msg[12] = ASCII_4;
		}
		show_text(3, button_msg);
	} else {
		show_text(3, "                ");
	}
}


char * PASSWORD = "111";
char PWDLEN = 3;
char password_attempt[4];
int password_authenticate() {
	int i = 0;
	char correct = 1;
	while(i < PWDLEN) {
		password_attempt[i] = await_button_value();
		show_text(1, password_attempt);
		correct = correct && (PASSWORD[i] == password_attempt[i]);
		
		i++;
	}
	if(correct) {
		show_text(2, "ACCESS GRANTED");
	} else {
		show_text(2, "ACCESS DENIED");
	}
}


void welcome_screen() {
	show_text(0, "WELCOME");
	// char* smileys = {1, 2, 1, 2, 1, 2};
	char* smileys = "- - - - - - - - ";
	for(char i=0; i<10; i++) {
		for(char j=0; j<16; j++) {
			if((j+i)%2) {
				smileys[j] = j % 2 ? 1 : 2;
			} else {
				smileys[j] = 32;
			}
		}
		show_text(1, smileys);
		show_text(2, smileys + 1);
		show_text(3, smileys + 2);
		delay_ms(500);
	}
}

// const char* USER_NAME = {BUTTON_0, BUTTON_1, BUTTON_2}
char* num2ASCII = "0123456789";
int ASCII_PLUS = 43;
int ASCII_SUB = 45;
int ASCII_EQ = 61;

void calculator_program(void* args) {
	int num1 = -1;
	int num2 = -1;
    int op = 0;
    char first = 0;
    char second = 0;
    char operation = 0;
    char * result = {0, 0};
    int num_res = 0;
	while(1) {
		if(result[0] != 0){
			first = -1;
			second = -1;
			operation = 0;
			result[0] = 0;
			result[1] = 0;
		}
        switch (await_button_value())
        {
        	case '0':
        		ClearScreen();
			    rpi_yield();
			    break;
        	case '1':
        	    if(num1 != 9 || num2 != 9){
        	    	if(op){
	    	    		num2 += 1;
	    	    		second = num2ASCII[num2];
    	    		} else {
	    	    		num1 += 1;
	    	    		first = num2ASCII[num1];
    	    		}
        	    }
    	    	break;
        	case '2':
        		operation = ASCII_PLUS;
        		op = 1;
        		break;
        	case '3':
        	    operation = ASCII_SUB;
        	    op = 1;
        	    break;
        	case '4':
        	    if(operation == ASCII_PLUS){
        	    	num_res = num1 + num2;
        	    } else if(operation == ASCII_SUB) {
        	    	num_res = num1 - num2;
        	    }
        	    result[1] = num2ASCII[num_res];
        	    result[0] = ASCII_EQ;
        	    num1 = 0;
        	    num2 = 0;
        	    op = 0;
        	    break;
        }
        char text[6];
        text[0] = first;
        text[1] = operation;
        text[2] = second;
        text[3] = result[0];
        text[4] = result[1];
        text[5] = 0;
		show_text(0, "calculator");
		show_text(2, text);		
	}
}

void ereader_program(void* args) {
	while(1) {
		if(gpio_read(BUTTON_0)) {
			while(gpio_read(BUTTON_0)){}
			ClearScreen();
			rpi_yield();
		}
		show_text(0, "ereader");
	}
}

void info_program(void* args) {
	while(1) {
		if(gpio_read(BUTTON_0)) {
			while(gpio_read(BUTTON_0)){}
			ClearScreen();
			rpi_yield();
		}
		char* info_msg = "This computer was designed and built by Nadin and Noah in Dawson Engler's CS140e course.";
		show_text(0, "Information");
		// for(i=0; i)
		// show_text(1, info_msg);
		// show_text(2, "designed and built by");
		// show_text(4, "");
	}
}


void notmain(void) {

    uart_init();
	printk("initializing buttons\n");
	button_init();
	printk("initializing spi0\n");
	spi_init();
	printk("initializing rfid\n");
	rfid_init();
	printk("resetting oled\n");
	oled_reset();
	printk("initializing oled\n");
	oled_init();
    show_text(0,"LOGIN PASSWORD:");
	
	// clean_reboot();
	test_mfrc522();

	rfid_config();
	// for(int i=0; i<10; i++) {

	// while(!password_authenticate()) {}
	// show_text(4, "rfid 2FA");
	// while (!detect_rfid_card()) {}

	ClearScreen();
	// welcome_screen();
	rpi_fork(calculator_program, (void*)0);
	rpi_fork(ereader_program, (void*)0);
	rpi_thread_start(0);


	clean_reboot();


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
