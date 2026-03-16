    .section .multiboot
    .align 4
    .long 0x1BADB002                # magic number
    .long 0x00010003                # flags: memory + framebuffer
    .long -(0x1BADB002 + 0x00010003)

    .section .text
    .global _start
_start:
    cli
    push %ebx
    push %eax
    call kernel_main
    hlt
