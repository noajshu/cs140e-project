// Unfortunately this is for the mini SPI 1/2
// not for the SPI0 which is wired up to the pins
#include "rpi.h"



// Page 22 of ARM peripherals manual
struct spi_cntl {
    volatile unsigned
        SPEED: 12, // 31:20
        CHIP_SELECTS: 3, // 19:17
        POST_INPUT_MODE: 1, // 16
        VARIABLE_CS: 1, // 15
        VARIABLE_WIDTH: 1, // 14
        DOUT_HOLD_TIME: 2, // 13:12
        ENABLE: 1, // 11
        IN_RISING: 1, // 10
        CLEAR_FIFOS: 1, // 9
        OUT_RISING: 1, // 8
        INVERT_SPI_CLK: 1, // 7
        SHIFT_OUT_MS_BIT_FIRST: 1, // 6
        SHIFT_LENGTH: 6; // 5:0
};
volatile struct spi_cntl* const spi_periph_cntl = (struct spi_cntl*)0x20215080;

// see errata https://elinux.org/BCM2835_datasheet_errata#p25_table
struct spi_stat {
    volatile unsigned
        TX_FIFO_LEVEL: 8, // 31:24
        RX_FIFO_LEVEL: 8, // 23:16
        SBZ: 6, // 15:10 SBZ
        TX_FULL: 1, // 9
        TX_EMPTY: 1, // 8
        RX_EMPTY: 1, // 7
        BUSY: 1, // 6
        BIT_COUNT: 6; // 5:0
};
volatile struct spi_stat* const spi_periph_stat = (struct spi_stat*)0x20215088;


struct spi_fifo {
    volatile unsigned
        SBZ: 16, //31:16
        DATA: 16; // 15:0
};
volatile struct spi_fifo* const spi_periph_fifo = (struct spi_fifo*)0x202150A0;

volatile void* aux_enable = (void*)0x20215004;

void spi_init(unsigned spi_index) {
    // demand((spi_index==1) || (spi_index==2), "only supports SPI1 or SPI2\n");
    demand((spi_index==1), "only supports SPI1\n");


    // GPIO Alternate Functions
    // Table 6.2 on page 102 of BCM2835 ARM Peripherals manual
    // ALT0 = SPI0_CE1_N on GPIO7
    gpio_set_function(7, 2);
    // ALT0 = SPI0_CE0_N on GPIO8
    gpio_set_function(8, 2);
    // ALT0 = SPI0_MISO on GPIO9
    gpio_set_function(9, 2);
    // ALT0 = SPI0_MOSI on GPIO10
    gpio_set_function(10, 2);
    // ALT0 = SPI0_SCLK on GPIO11
    gpio_set_function(11, 2);

    // ENABLE SPI0
    put32(aux_enable, get32(aux_enable) | 0b10);

    // larger --> lower clock frequency
    // this is about as low as we can go
    spi_periph_cntl->SPEED = 4000;
    // spi_periph_cntl->CHIP_SELECTS = 0b111;
    // spi_periph_cntl->POST_INPUT_MODE = 0;
    // spi_periph_cntl->VARIABLE_CS = 1;
    // spi_periph_cntl->VARIABLE_WIDTH = 1;
    // spi_periph_cntl->DOUT_HOLD_TIME = 0b11;
    // spi_periph_cntl->IN_RISING = 1;
    spi_periph_cntl->CLEAR_FIFOS = 0;
    // spi_periph_cntl->OUT_RISING = 1;
    // spi_periph_cntl->INVERT_SPI_CLK = 0;
    // spi_periph_cntl->SHIFT_OUT_MS_BIT_FIRST = 0;
    // spi_periph_cntl->SHIFT_LENGTH; // ignored
    spi_periph_cntl->ENABLE = 1;
    printk("spi_periph_cntl = %x\n", spi_periph_cntl);
    printk("*spi_periph_cntl = %b\n", *spi_periph_cntl);

    return;
}


int spi_getc(void) {
    printk("spi_periph_stat->TX_FIFO_LEVEL = %d\n", spi_periph_stat->TX_FIFO_LEVEL);
    printk("spi_periph_stat->RX_FIFO_LEVEL = %d\n", spi_periph_stat->RX_FIFO_LEVEL);
    printk("spi_periph_stat->BUSY = %d\n", spi_periph_stat->BUSY);
	while (spi_periph_stat->RX_EMPTY) {
        spi_periph_stat->SBZ = 0;
	}
	// grab all 8 bits from io reg (will grab off the fifo)
    spi_periph_fifo->SBZ = 0;
    unsigned data = spi_periph_fifo->DATA;
    printk("spi_periph_stat->TX_FIFO_LEVEL = %d\n", spi_periph_stat->TX_FIFO_LEVEL);
    printk("spi_periph_stat->RX_FIFO_LEVEL = %d\n", spi_periph_stat->RX_FIFO_LEVEL);
    printk("spi_periph_stat->BUSY = %d\n", spi_periph_stat->BUSY);
    return data;
}
void spi_putc(unsigned c) {
    while (spi_periph_stat->TX_FULL) {
        spi_periph_stat->SBZ = 0;
	}
    spi_periph_fifo->DATA = c;
}


