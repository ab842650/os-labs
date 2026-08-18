extern char uart_getc(void);
extern void uart_putc(char c);
extern void uart_puts(const char* s);
extern void uart_hex(unsigned long h);
extern void video_init();
extern void video_bmp_display(unsigned int* bmp_image, int width, int height);

#define TIME_FREQ 10000000
int usleep(unsigned int usec) {
    // TODO: Implement this function
    unsigned long start;
    asm volatile("rdtime %0" : "=r"(start));
    unsigned long end = start + (TIME_FREQ / 1000000) * usec;
    while (1) {
        unsigned long now;
        asm volatile("rdtime %0" : "=r"(now));
        if (now >= end) {
            break;
        }
    }
    return 0;   
}

void display_video() {
#include "bird.h"
    while (1) {
        for (int f = 0; f < FRAME_COUNT; f++) {
            unsigned int* frame = (frames + (f * FRAME_WIDTH * FRAME_HEIGHT));
            video_bmp_display(frame, FRAME_WIDTH, FRAME_HEIGHT);
            usleep(50000);
        }
    }
}

void start_kernel() {
    uart_puts("\nStarting kernel ...\n");
    // TODO: Initialize the QEMU frame buffer device
    video_init();
    display_video();
}
