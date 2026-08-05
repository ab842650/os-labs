.section ".text.boot"
.global _start
_start:
    // TODO: Clear the .bss section
    // TODO: Set up the stack pointer
    la t0, bss_start
    la t1, bss_end

clear_bss_loop:

    bge t0,t1 ,clear_bss_done
    sd zero,0(t0)
    addi t0,t0,8
    j    clear_bss_loop


clear_bss_done:
    la sp , stack_top


    tail start_kernel