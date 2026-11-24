module zx;

namespace zx {

// this is implemented in start.cc
extern "C" elf_eh_frame::EhFrame ZX_ELF_EH_FRAME_;

#ifdef ZX_DEBUG_INFO

void Backtrace::collect(Backtrace *bt) {
    if (bt == nullptr) {
        return;
    }

    CPUContext cpu_ctx;
    __builtin_memset(&cpu_ctx, 0, sizeof(cpu_ctx));
    zx_getcpucontext_(&cpu_ctx);

    elf_eh_frame::EhFrame::BacktraceStatus const status =
        ZX_ELF_EH_FRAME_.backtrace_panic(&cpu_ctx, bt);
    switch (status) {
    case elf_eh_frame::EhFrame::BacktraceStatus::Ok: {
        break;
    }
    case elf_eh_frame::EhFrame::BacktraceStatus::OutOfCapacity: {
        // we don't care if it truncated.
        break;
    }
    default: {
        __builtin_memset(bt, 0, sizeof(Backtrace));
        break;
    }
    }
}

#else // ifdef ZX_DEBUG_INFO

void Backtrace::collect(Backtrace *bt) {
    if (bt == nullptr) {
        return;
    }
    __builtin_memset(bt, 0, sizeof(Backtrace));
}

#endif // ifdef ZX_DEBUG_INFO

} // namespace zx
