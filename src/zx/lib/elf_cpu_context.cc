#ifdef ZX_DEBUG_INFO

__asm__(".text\n"
        ".type zx_elf_getcpucontext_, @function\n"
        ".global zx_elf_getcpucontext_\n"
        "zx_elf_getcpucontext_:\n\t"
        "movq %r12, 0(%rdi)\n\t"
        "movq %r13, 8(%rdi)\n\t"
        "movq %r14, 16(%rdi)\n\t"
        "movq %r15, 24(%rdi)\n\t"
        "movq %rbp, 32(%rdi)\n\t"
        "movq %rbx, 40(%rdi)\n\t"
        // save rsp, exclude this function's frame
        "leaq 8(%rsp), %rax\n\t"
        "movq %rax, 48(%rdi)\n\t"
        // save rip, exclude this function's frame
        "movq 0(%rsp), %rax\n\t"
        "movq %rax, 56(%rdi)\n\t"
        "xorq %rax, %rax\n\t"
        "retq\n\t");

#endif // ifdef ZX_DEBUG_INFO
