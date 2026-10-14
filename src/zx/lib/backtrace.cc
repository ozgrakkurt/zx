module zx;
import :def;
import :elf_eh_frame;
import :slice;
import :list;
import :assert;
import :cpu_context;

namespace zx {

#ifdef ZX_DEBUG_INFO

extern elf::EhFrame ZX_ELF_EH_FRAME_;

void Backtrace::collect(Backtrace *bt) {
    if (bt == nullptr) {
        return;
    }

    List<u64> data =
        List<u64>::init(SliceMut<u64>::init_unsafe(bt->m_data, Backtrace::LEN));

    CPUContext cpu_ctx;
    __builtin_memset(&cpu_ctx, 0, sizeof(cpu_ctx));
    zx_getcpucontext_(&cpu_ctx);

    elf::EhFrame::BacktraceStatus const status =
        ZX_ELF_EH_FRAME_.backtrace_panic(&cpu_ctx, bt);
    // doesn't matter if it truncated
    (void)status;

    assert_dev(data.n_written() <= Backtrace::LEN);
    for (u64 i = data.n_written(); i < Backtrace::LEN; i++) {
        // fll zeroes
        bt->m_data[i] = 0;
    }
}

#else // ifdef ZX_DEBUG_INFO

struct Backtrace {
    static void collect(Backtrace *bt) {
        if (bt == nullptr) {
            return;
        }
        __builtin_memset(bt, 0, sizeof(Backtrace));
    }
}

#endif // ifdef ZX_DEBUG_INFO

} // namespace zx
