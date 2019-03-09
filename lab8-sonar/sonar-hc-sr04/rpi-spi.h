// Page 22 of ARM peripherals manual
volatile void* AUXSPI0_CNTL0 = 0x7e215080;

struct {
    unsigned SPI_SPEED: 11; // 31:20
} spicntl;

void spi_init() {
    return;    
}