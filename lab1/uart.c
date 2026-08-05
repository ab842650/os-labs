// uart.c
#define UART_BASE 0x10000000UL

// UART register offsets
#define UART_THR  0x00   // Transmitter Holding Register (write data here)
#define UART_RBR  0x00   // Receiver Buffer Register (read data here)
#define UART_LSR  0x05   // Line Status Register (status register)

// LSR bit definitions
#define LSR_DR    0x01   // Data Ready: bit0, set when data is available to read
#define LSR_THRE  0x20   // Transmitter Holding Register Empty: bit5, set when ready to write

// Convert base + offset into a volatile pointer for MMIO access
#define UART_REG(offset) (*(volatile unsigned char *)(UART_BASE + offset))

char uart_getc() {

    while(! (UART_REG(UART_LSR) & LSR_DR) ){
        
    }
    return UART_REG(UART_RBR);
}

void uart_putc(char c) {

    while(! (UART_REG(UART_LSR) & LSR_THRE) ){
        
    }
    UART_REG(UART_THR) = c ;
}

void uart_puts(const char* s) {
    while(*s != '\0'){
        uart_putc(*s);
        s++;
    }
}

void uart_hex(unsigned long h) {
    uart_puts("0x");
    unsigned long n;
    for (int c = 60; c >= 0; c -= 4) {
        n = (h >> c) & 0xf;
        n += n > 9 ? 0x57 : '0';
        uart_putc(n);
    }
}