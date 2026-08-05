.section .text
.global _start

_start:
    li a0, 0x1234      # 隨便放個值到暫存器，方便待會用gdb驗證
    li a1, 0x5678
loop:
    j loop              # 無窮迴圈，讓CPU停在這裡