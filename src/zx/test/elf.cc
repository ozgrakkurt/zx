import zx;

using namespace zx;

#ifdef ZX_DEBUG_INFO

void cmmb() {
    CPUContext cpu_ctx;
    zx_getcpucontext_(&cpu_ctx);

    elf::print_stack_trace_panic(&cpu_ctx);
}

void callme_maybe(u64 iter) {
    if (iter > 5) {
        CPUContext cpu_ctx;
        zx_getcpucontext_(&cpu_ctx);
        elf::print_stack_trace_panic(&cpu_ctx);
    } else {
        callme_maybe(iter + 1);
    }
}

int main() {
    callme_maybe(0);
    cmmb();

    // uncomment to actually see the print outputs in the terminal.
    // sys::exit_group(sys::EXIT_FAILURE);

    return 0;
}

#else // ifdef ZX_DEBUG_INFO

int main() { return 0; }

#endif // ifdef ZX_DEBUG_INFO
