#include "rpi.h"


#define AUX_ENABLES     0x20215004

#define SPI_CPOL_HIGH (1 << 3)

// page 152
// struct cs_reg {
//     // table on page 153
//     volatile unsigned
//         SBZ:6,
//         LEN_LONG:1, DMA_LEN:1, CSPOL2:1, CSPOL1:1, CSPOL0:1, RXF:1,
//         RXR:1, TXD:1, RXD:1, DONE:1, TE_EN:1, LMONO:1, LEN:1,
//         REN:1, ADCS:1, INTR:1, INTD:1, DMAEN:1, TA:1, CSPOL:1,
//         CLEAR:2, CPOL:1, CPHA:1, CS:2;
// };
struct spi_periph {
    // volatile struct cs_reg CS;
    volatile unsigned
        CS,
        FIFO,
        CLK,
        DLEN,
        LTOH,
        DC;
};
volatile struct spi_periph* const spi0 = (struct spi_periph*)0x20204000;

void spi_init() {
    // dwelch enables spi1, but this should not matter
    // unsigned ra=GET32(AUX_ENABLES);
    // ra|=2; //enable spi0
    // PUT32(AUX_ENABLES,ra);
    // GPIO Alternate Functions
    // Table 6.2 on page 102 of BCM2835 ARM Peripherals manual
    // ALT0 = SPI0_CE1_N on GPIO7
    gpio_set_function(7, GPIO_FUNC_ALT0);
    // ALT0 = SPI0_CE0_N on GPIO8
    gpio_set_function(8, GPIO_FUNC_ALT0);
    // ALT0 = SPI0_MISO on GPIO9
    gpio_set_function(9, GPIO_FUNC_ALT0);
    // ALT0 = SPI0_MOSI on GPIO10
    gpio_set_function(10, GPIO_FUNC_ALT0);
    // ALT0 = SPI0_SCLK on GPIO11
    gpio_set_function(11, GPIO_FUNC_ALT0);

    // printk("spi0->CS = %b\n", spi0->CS);
    // printk("get32(0x20204000) = %b\n", get32(0x20204000));
    // ENABLE SPI0
    spi0->CS = 0x0000030;
    // spi0->CS.SBZ = 0;
    // spi0->CS.CLEAR = 0b11;
    spi0->CLK = 0; // divisor is 65536
    return;
}


unsigned long spi_getc() {
    spi0->CS = 0x004000B0;
    while (!((1 << 16) & spi0->CS)) {} // DONE
    while (!((1 << 17) & spi0->CS)) {} // RXD
    unsigned long data = spi0->FIFO;
    spi0->CS = 0x00400000;
    return data;
}


void spi_putc(unsigned long c) {
    spi0->CS = 0x004000B0;
    while (!((1 << 18) & spi0->CS)) {} // TXD
    spi0->FIFO = c;
    while (!((1 << 16) & spi0->CS)) {} // DONE
    spi0->CS = 0x00400000;
    return;
}

