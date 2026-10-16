import zx;

using namespace zx;

static void handler(i32_c signum, sys::SigInfo *info, void *ucontext) {
    (void)info;
    (void)ucontext;

    assert((u32)signum == (u32)sys::SIG::SEGV);
    sys::exit_group(0);
}

extern "C" void zx_test_cause_stack_overflow_() {
    // do this in ASM so the compiler doesn't break it
    asm volatile("1:\n\t"
                 "callq zx_test_cause_stack_overflow_\n\t" ::
                     : "memory");
}

int main() {
    constexpr u64 MEM_SIZE = u64{1} << 21;

    u8 *mem = nullptr;
    assert(page_alloc::AllocStatus::Ok ==
           page_alloc::alloc_4kb(nullptr, MEM_SIZE, &mem));
    assert(mem != nullptr);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, MEM_SIZE));

    SliceMut<u8> signal_stack_mem;
    assert(AllocStatus::Ok == alloc.aligned_alloc<u8>(nullptr, 16, u64{1} << 20,
                                                      &signal_stack_mem));

    sys::Stack const signal_stack = sys::Stack{
        .ss_sp = signal_stack_mem.ptr(),
        .ss_flags = 0,
        .ss_size = signal_stack_mem.len(),
    };
    assert(sys::SigAltStackStatus::Ok ==
           sys::sigaltstack(nullptr, &signal_stack, nullptr));

    sys::SigAction act = sys::SigAction{
        .sigaction = handler,
        .flags = sys::SA_SIGINFO | sys::SA_RESETHAND | sys::SA_NODEFER |
                 sys::SA_ONSTACK,
        .mask = sys::SigSet::init(),
    };

    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(nullptr, sys::SIG::SEGV, &act, nullptr));

    zx_test_cause_stack_overflow_();

    return 69;
}
