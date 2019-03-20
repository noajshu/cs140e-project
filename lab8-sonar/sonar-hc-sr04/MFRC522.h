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
    // DISABLE RCVOFF (bit pos 5)
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



char detect_rfid_card() {
    // printk("setting up transcieve mode\n");
    // printk("\n\n");
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
    #if 0
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
    #endif

    if(rfid_read_reg(REG_FIFO_LEVEL)) {
        // TODO clear fifo, read card prefix
        return 1;
    } else {
        return 0;
    }
}

void rfid_config() {
    rfid_write_reg(REG_TMODE, 0x80);			// start timer on transmission end
    rfid_write_reg(REG_TPRESCALER, 0xA9);	// TPreScaler = 0xA9 = 169
	// reload timer with 0x3E8 = 1000
    rfid_write_reg(REG_T_RELOAD_HI, 0x04);
    rfid_write_reg(REG_T_RELOAD_LO, 0xE8);
    rfid_write_reg(REG_TX_ASK, 0x40);		// force ASK modulation independent of the ModGsPReg register
    rfid_write_reg(REG_MODE, 0x3D);	
    rfid_write_reg(REG_TXCTL, rfid_read_reg(REG_TXCTL) | 0b11); // output signal on pin TX1 & TX2 delivers the 13.56 MHz energy carrier modulated by the transmission data

}


void test_mfrc522() {
    char buf1[100];
    rfid_loopback_simple(buf1);
    printk("rfid_loopback_simple = %s\n", buf1);
    char buf2[100];
    rfid_loopback_regs(buf2);
    printk("rfid_loopback_regs = %s\n", buf2);
}
