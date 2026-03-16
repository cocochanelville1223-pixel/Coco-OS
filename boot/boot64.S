    .section .multiboot
    .align 4
    .long 0x1BADB002                # magic number
    .long 0x00010003                # flags: memory + framebuffer
    .long -(0x1BADB002 + 0x00010003)

    .section .text
    .global _start
    .type _start, @function
_start:
    cli
    xor %rbp, %rbp
    mov %rsp, %rbp
    mov %rdi, %rdi
    mov %rsi, %rsi
    call kernel_main
    hlt
