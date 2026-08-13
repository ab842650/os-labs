extern char uart_getc(void);
extern void uart_putc(char c);
extern void uart_puts(const char* s);
extern void uart_hex(unsigned long h);
extern int hextoi(const char* s, int n);
extern int align(int n, int byte);
extern int memcmp(const void* s1, const void* s2, int n);
extern void* alloc_page();

// TODO: Check the RAM disk base address
#define INITRD_BASE 0xa0200000
#define STACK_SIZE  0x1000

struct cpio_t {
    char magic[6];
    char ino[8];
    char mode[8];
    char uid[8];
    char gid[8];
    char nlink[8];
    char mtime[8];
    char filesize[8];
    char devmajor[8];
    char devminor[8];
    char rdevmajor[8];
    char rdevminor[8];
    char namesize[8];
    char check[8];
};

int exec(const char* filename) {
    char* p = (char*)INITRD_BASE;
    while (memcmp(p + sizeof(struct cpio_t), "TRAILER!!!", 10)) {
        struct cpio_t* hdr = (struct cpio_t*)p;
        int namesize = hextoi(hdr->namesize, 8);
        int filesize = hextoi(hdr->filesize, 8);
        int headsize = align(sizeof(struct cpio_t) + namesize, 4);
        int datasize = align(filesize, 4);
        if (!memcmp(p + sizeof(struct cpio_t), filename, namesize)) {
            // TODO: Finish this function
            unsigned long entry = (unsigned long)(p + headsize);
            unsigned long stack_entry = (unsigned long)alloc_page()+STACK_SIZE; // Allocate a page for the user stack
            asm volatile(
                "csrw sepc, %0;"        // sepc ← entry
                "csrc sstatus, %2;"     // 清掉 sstatus 的 SPP (bit 8)
                "csrw sscratch, sp;"    // 把 kernel sp 藏進 sscratch
                "mv sp, %1;"            // sp ← user stack top
                "sret;"
                :: "r"(entry), "r"(stack_entry), "r"(0x100)
            );

        }
        p += headsize + datasize;
    }
    return -1;
}

// TODO: Define the trap frame structure
struct pt_regs {
    unsigned long ra;       //  0
    unsigned long sp;       //  1
    unsigned long gp;       //  2
    unsigned long tp;       //  3
    unsigned long t0;       //  4
    unsigned long t1;       //  5
    unsigned long t2;       //  6
    unsigned long s0;       //  7
    unsigned long s1;       //  8
    unsigned long a0;       //  9
    unsigned long a1;       // 10
    unsigned long a2;       // 11
    unsigned long a3;       // 12
    unsigned long a4;       // 13
    unsigned long a5;       // 14
    unsigned long a6;       // 15
    unsigned long a7;       // 16
    unsigned long s2;       // 17
    unsigned long s3;       // 18
    unsigned long s4;       // 19
    unsigned long s5;       // 20
    unsigned long s6;       // 21
    unsigned long s7;       // 22
    unsigned long s8;       // 23
    unsigned long s9;       // 24
    unsigned long s10;      // 25
    unsigned long s11;      // 26
    unsigned long t3;       // 27
    unsigned long t4;       // 28
    unsigned long t5;       // 29
    unsigned long t6;       // 30
    unsigned long sepc;     // 31
    unsigned long sstatus;  // 32
    unsigned long scause;   // 33
    unsigned long stval;    // 34
};

void do_trap(struct pt_regs* regs) {
    // TODO: Implement this function
    // (1) Print the sepc and scause registers
    // (2) Increment the sepc register by 4 for traps
    uart_puts("sepc: ");
    uart_hex(regs->sepc);
    uart_puts(", scause: ");
    uart_hex(regs->scause);
    uart_putc('\n');
    regs->sepc += 4;

}

void start_kernel() {
    uart_puts("\nStarting kernel ...\n");
    if (exec("prog.bin"))
        uart_puts("Failed to exec user program!\n");
    while (1) {
        uart_putc(uart_getc());
    }
}
