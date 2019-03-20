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
	BUTTON_4 = 17,
	NO_BUTTON
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

char is_any_button_pressed() {
	return (gpio_read(BUTTON_0) || gpio_read(BUTTON_1) || gpio_read(BUTTON_2) || gpio_read(BUTTON_3) || gpio_read(BUTTON_4));
}

char get_button_stickiness(int sticky_ms) {
	if (gpio_read(BUTTON_0)) {
		delay_ms(sticky_ms);
		return BUTTON_0;
	}
	if (gpio_read(BUTTON_1)) {
		delay_ms(sticky_ms);
		return BUTTON_1;
	}
	if (gpio_read(BUTTON_2)) {
		delay_ms(sticky_ms);
		return BUTTON_2;
	}
	if (gpio_read(BUTTON_3)) {
		delay_ms(sticky_ms);
		return BUTTON_3;
	}
	if (gpio_read(BUTTON_4)) {
		delay_ms(sticky_ms);
		return BUTTON_4;
	}
}
char get_button() {
	if (gpio_read(BUTTON_0)) {
		while(gpio_read(BUTTON_0)){}
		delay_ms(sticky_key_wait_ms);
		return BUTTON_0;
	}
	if (gpio_read(BUTTON_1)) {
		while(gpio_read(BUTTON_1)){}
		delay_ms(sticky_key_wait_ms);
		return BUTTON_1;
	}
	if (gpio_read(BUTTON_2)) {
		while(gpio_read(BUTTON_2)){}
		delay_ms(sticky_key_wait_ms);
		return BUTTON_2;
	}
	if (gpio_read(BUTTON_3)) {
		while(gpio_read(BUTTON_3)){}
		delay_ms(sticky_key_wait_ms);
		return BUTTON_3;
	}
	if (gpio_read(BUTTON_4)) {
		while(gpio_read(BUTTON_4)){}
		delay_ms(sticky_key_wait_ms);
		return BUTTON_4;
	}
	return NO_BUTTON;
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
		return 1;
	} else {
		show_text(2, "ACCESS DENIED");
		return 0;
	}
}


void welcome_screen() {
	show_text(0, "WELCOME");
	// char* smileys = {1, 2, 1, 2, 1, 2};
	char* smileys = "- - - - - - - - ";
	for(int i=0; i<10; i++) {
		for(int j=0; j<16; j++) {
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
char* num2OP = "+-*";
int ASCII_PLUS = 43;
int ASCII_SUB = 45;
int ASCII_EQ = 61;
int ASCII_PIPE= 124;
int ASCII_SPACE = 32;

void calculator_program(void* args) {
	int num1 = -1;
	int num2 = -1;
    int op = -1;
    char text[9];
    int text_idx = 0;
    int result = 0;
    char cursor[9];

    for (int i = 0; i < 9; i++) {
    	text[i] = 0;
    	cursor[i] = 0;
    }

    while(1){
    	switch (await_button_value())
        {
        	case '0':
        		ClearScreen();
			    rpi_yield();
			    break;
        	case '1':
                if(text_idx == 0){
                	num1 += 1;
                    text[text_idx] = num2ASCII[num1/10];
                    text[text_idx + 1] = num2ASCII[num1%10];
                } else if (text_idx == 2) {
                	op += 1;
                	if(op == 3) op = 0;
                	text[text_idx] = num2OP[op];
                } else if (text_idx == 3){
                	num2 += 1;
                    text[text_idx] = num2ASCII[num2/10];
                    text[text_idx + 1] = num2ASCII[num2%10];
                } 
    	    	break;
        	case '2':
        		if(text_idx == 3) {
        	    	text_idx = 2;
        	    } else if (text_idx == 2) {
        	    	text_idx = 0;
        	    }
        		break;
        	case '3':
        	    if(text_idx == 0) {
        	    	text_idx = 2;
        	    } else if (text_idx == 2) {
        	    	text_idx = 3;
        	    }
        	    break;
        	case '4':
            	if(op == 0) {
            		result = num1 + num2;
            	} else if (op == 1) {
            		result = num1 - num2;
            	} else if (op == 2) {
            		result = num1 * num2;
            	}
            	text[5] = ASCII_EQ;
            	text[6] = num2ASCII[result/10];
            	text[7] = num2ASCII[result%10];
    	    	break;
        	default: 
        	    break;
        }
        for (int i = 0; i < text_idx; i++) {
    		cursor[i] = ASCII_SPACE;
    	}
        cursor[text_idx] = ASCII_PIPE;
        for(int j = text_idx + 1; j< 9; j++) {
        	cursor[j] = 0;
        }
        show_text(0, "calculator");
		show_text(2, text);	
		show_text(3, cursor);
    }   	
}


// void calculator_program(void* args) {
// 	while(1){
// 		int num1 = -1;
// 		int num2 = -1;
// 	    int op = 0;
// 	    char first = 0;
// 	    char second = 0;
// 	    char operation = 0;
// 	    char * result = {0, 0};
// 	    int num_res = 0;
// 	    char text[6];
// 		while(1) {
// 			char next_val = await_button_value();
// 	        switch (next_val)
// 	        {
// 	        	case '0':
// 	        		ClearScreen();
// 				    rpi_yield();
// 				    break;
// 	        	case '1':
// 	        	    if(num1 == 9){
// 	        	    	num1 = -1;
// 	        	    } else if (num2 == 9) {
// 	        	    	num2 = -1;
// 	        	    }
// 	    	    	if(op){
// 	    	    		num2 += 1;
// 	    	    		second = num2ASCII[num2];
// 		    		} else {
// 	    	    		num1 += 1;
// 	    	    		first = num2ASCII[num1];
// 		    		}
// 	    	    	break;
// 	        	case '2':
// 	        		operation = ASCII_PLUS;
// 	        		op = 1;
// 	        		break;
// 	        	case '3':
// 	        	    operation = ASCII_SUB;
// 	        	    op = 1;
// 	        	    break;
// 	        	case '4':
// 	        	    if(operation == ASCII_PLUS){
// 	        	    	num_res = num1 + num2;
// 	        	    } else if(operation == ASCII_SUB) {
// 	        	    	num_res = num1 - num2;
// 	        	    }
// 	        	    result[1] = num2ASCII[num_res];
// 	        	    result[0] = ASCII_EQ;
// 	        	    num1 = 0;
// 	        	    num2 = 0;
// 	        	    op = 0;
// 	        	    break;
// 	        	default: 
// 	        	    break;
// 	        }
// 	        text[0] = first;
// 	        text[1] = operation;
// 	        text[2] = second;
// 	        text[3] = result[0];
// 	        text[4] = result[1];
// 	        text[5] = 0;
// 			show_text(0, "calculator");
// 			show_text(2, text);		
// 		}
// 	}
// }


void strncpy(char* dst, char* src, size_t len) {
	for(unsigned i=0; i<len; i++) {
		dst[i] = src[i];
	}
	dst[len] = 0;
}

int SCREEN_CHAR_WIDTH = 16;
void ereader_program(void* args) {

	unsigned cursor_pos = 0;
	char* text = "He finishes his cereal and is about to disconnect when an anonynous message slices onto the screen. SCREEN Do you want to know what the Matrix is, Neo? Neo is frozen when he reads his name. SCREEN SUPERASTIC: Who said that? JACKON: Who's Neo? GIBSON: This is a private board. If you want to know, follow the white rabbit. NEO What the hell... SCREEN TIMAXE: Someone is hacking the hackers! FOS4: It's Morpheus!!!!! JACKON: Identify yourself. Knock, knock, Neo. A chill runs down his spine and when someone KNOCKS on his door he almost jumps out of his chair. He looks at the door, then back at the computer but the message is gone. He shakes his head, not completely sure what happened. Again, someone knocks. Cautiously, Neo approaches the door. VOICE (O.S.) Hey, Tommy-boy! You in there? Recognizing the voice, he relaxes and opens it. ANTHONY, who lives down the hall, is standing outside with a group of friends. NEO What do you want, Anthony? ANTHONY I need your help, man. Desperate. They got me, man. The shackles of fascism. He holds up the red notice that accompanies the Denver boot. NEO You got the money this time? He holds up two hundred dollars and Neo opens the door. Anthony's girlfriend, DUJOUR, stops in front of Neo. DUJOUR You can really get that thing off, right now? ANTHONY I told you, honey, he may look like just another geek but this here is all we got left standing between Big Brother and the New World Order. EXT. STREET A police officer unlocks a yellow metal boot from the wheel of an enormous oldsmobile. INT. NEO'S APARTMENT They watch from the window as the cops, silently, robotically, climb into their van. ANTHONY Look at 'em. Automatons. Don't think about what they're- doing or why. Computer tells 'em what to do and they do it. FRIEND #l Thc banality of evil. He slaps the money in Neo's hand. ANTHONY Thanks, neighbor. DUJOUR Why don't you come to the party with us? NEO I don't know. I have to work tomorrow. DUJOUR Come on. It'll be fun. He looks up at her and suddenly notices on her black leather motorcycle jacket dozens of pins: bands, symbols, slogans, military medals and -- A small white rabbit. The ROOM TILTS. NEO Yeah, yeah. Sure, I'll go. INT. APARTMENT An older Chicago apartment; a series of halls connects a chain of small high-ceilinged rooms lined with heavy casements. Smoke hangs like a veil, blurring the few lights there are. Dressed predominantly in black, people are everywhere, gathered in cliques around pieces of furniture like jungle cats around a tree. Neo stands against a wall, alone, sipping from a bottle of beer, feeling completely out of place, he is about to leave when he notices a woman staring at him. The woman is Trinity. She walks straight up to him. In the nearest room, shadow-like figures grind against each other to the pneumatic beat of INDUSTRIAL MUSIC. TRINITY Hello, Neo. NEO How did you know that -- TRINITY I know a lot about you. I've been wanting to meet you for some time. NEO Who are you? TRINITY My name is Trinity. NEO Trinity? The Trinity? The Trinity that cracked the I.R.S. Kansas City D-Base? TRINITY That was a long time ago. NEO Gee-zus. TRINITY What? NEO I just thought... you were a guy. TRINITY Most guys do. Neo is a little embarrassed. NEO Do you want to go sorewhere and talk? TRINITY No. It's safe here and I don't have much time. The MUSIC is so loud they must stand very close, talking directly into each other's ear. NEO That was you on the board tonight. That was your note, wasn't it? TRINITY I had to gamble that you would see and they wouldn't. NEO Who wouldn't? TRINITY I can't explain everything to you. I'm sure that it's all going to seem very strange, but I brought you here to warn you, Neo. You are in a lot of danger. NEO What? Why? TRINITY They're watching you. Something happened and they found out about you. Normally, if our target is exposed we let it go. But this time, we can't do that. NEO I don't understand -- TRINITY You came here because you wanted to know the answer to a hacker's question. NEO The Matrix. What is the Matrix? TRINITY Twelve years ago I met a man, a great man, who said that no one could be told the answer to that question. That they had to see it, to believe it. Her body is against his; her lips very close to his ear. TRINITY He told me that no one should look for the answer unless they have to because once you see it, everything changes. Your life and the world you live in will never be the same. It's as if you wake up one morning and the sky is falling. There is a hypnotic quality to her voice and Neo feels the words like a drug, seeping into him. TRINITY The truth is out there, Neo. It's looking for you and it will find you, if you want it to. She takes hold of him with her eyes. TRINITY That's all I can tell you right now. Good-bye, Neo. And good luck.";
	char * line_buf = kmalloc(SCREEN_CHAR_WIDTH*8);

	while(1) {
		if(gpio_read(BUTTON_0)) {
			while(gpio_read(BUTTON_0)){}
			ClearScreen();
			rpi_yield();
		} else if (gpio_read(BUTTON_4)) {
			clean_reboot();
		}
		if(is_any_button_pressed()) {
			switch (get_button_stickiness(100)) {
				case BUTTON_3:
					// scroll down
					cursor_pos++;
					break;
				case BUTTON_2:
					// scroll up
					cursor_pos--;
					break;
				default:
					break;
			}
		}
		show_text(0, "EREADER");
		for(unsigned i=0; i<5; i++) {
			strncpy(line_buf, &text[(i+cursor_pos)*SCREEN_CHAR_WIDTH], SCREEN_CHAR_WIDTH);
			show_text(i+1, line_buf);
		}
	}
}

void info_program(void* args) {
	while(1) {
		if(gpio_read(BUTTON_0)) {
			while(gpio_read(BUTTON_0)){}
			ClearScreen();
			rpi_yield();
		} else if (gpio_read(BUTTON_4)) {
			clean_reboot();
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

	while(!password_authenticate()) {}
	show_text(4, "rfid 2FA");
	while (!detect_rfid_card()) {}

	ClearScreen();
	welcome_screen();
	rpi_fork(calculator_program, (void*)0);
	rpi_fork(ereader_program, (void*)0);
	rpi_fork(info_program, (void*)0);
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
