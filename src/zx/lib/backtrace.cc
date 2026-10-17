module zx;
import :def;
import :elf_eh_frame;
import :assert;
import :cpu_context;
import :assert;
import :sys;
import :panic;
import :elf;

namespace zx {

static void print_panic_(char const *message) {
    assert(message != nullptr);

    u64 const len = __builtin_strlen(message);
    u64 n_written = 0;
    assert(sys::WriteStatus::Ok == sys::write(nullptr, sys::STDERR,
                                              (u8 const *)message, len,
                                              &n_written));
    assert(n_written == len);
}

#ifdef ZX_DEBUG_INFO

void Backtrace::collect(Backtrace *bt) {
    if (bt == nullptr) {
        return;
    }

    CPUContext cpu_ctx;
    __builtin_memset(&cpu_ctx, 0, sizeof(cpu_ctx));
    zx_getcpucontext_(&cpu_ctx);

    assert(bt->eh_frame_ != nullptr);
    assert(bt->debug_line_ != nullptr);

    elf::EhFrame const *eh_frame = (elf::EhFrame const *)bt->eh_frame_;

    elf::EhFrame::BacktraceStatus const status =
        eh_frame->backtrace_panic(&cpu_ctx, bt);
    // doesn't matter if it truncated
    (void)status;
}

void Backtrace::assert_with_trace(bool cond, Backtrace *bt) {
    if (bt == nullptr) {
        return;
    }

    assert(bt->eh_frame_ != nullptr);
    assert(bt->debug_line_ != nullptr);

    if (!cond) [[unlikely]] {
        print_panic_(
            "Printing stack trace because testing error assertion failed.\n");

        elf::print_stack_trace_panic((elf::DebugLine const *)bt->debug_line_,
                                     bt);
        panic();
    }
}

#else // ifdef ZX_DEBUG_INFO

void Backtrace::collect(Backtrace *bt) {
    if (bt == nullptr) {
        return;
    }
    assert(bt->eh_frame_ == nullptr);
    assert(bt->debug_line_ == nullptr);
    __builtin_memset(bt, 0, sizeof(Backtrace));
}

void Backtrace::assert_with_trace(bool cond, Backtrace *bt) {
    if (bt == nullptr) {
        return;
    }
    assert(bt->eh_frame_ == nullptr);
    assert(bt->debug_line_ == nullptr);

    if (!cond) [[unlikely]] {
        print_panic_("assert_with_trace failed. Can't print stack trace "
                     "because debug info is disabled.");
        panic();
    }
}

#endif // ifdef ZX_DEBUG_INFO

} // namespace zx
