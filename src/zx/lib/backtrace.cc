module zx;
import :def;
import :elf_eh_frame;
import :assert;
import :cpu_context;
import :assert;
import :sys;
import :panic;

namespace zx {

// this is implemented in start.cc
extern "C" elf::EhFrame ZX_ELF_EH_FRAME_;

#ifdef ZX_DEBUG_INFO

void Backtrace::collect(Backtrace *bt) {
    if (bt == nullptr) {
        return;
    }

    CPUContext cpu_ctx;
    __builtin_memset(&cpu_ctx, 0, sizeof(cpu_ctx));
    zx_getcpucontext_(&cpu_ctx);

    elf::EhFrame::BacktraceStatus const status =
        ZX_ELF_EH_FRAME_.backtrace_panic(&cpu_ctx, bt);
    // doesn't matter if it truncated
    (void)status;
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
