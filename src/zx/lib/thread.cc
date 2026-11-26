module zx;

namespace zx {

namespace thread {

#ifdef ZX_DEV
extern "C" void sigaltstack_() {
    Backtrace err_bt = Backtrace::init();

    constexpr u64 STACK_SIZE = u64{1} << 20;

    u8 *mem = nullptr;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_bt, STACK_SIZE, &mem),
        &err_bt);

    assert(mem != nullptr);
    assert(__builtin_is_aligned(mem, STACK_ALIGN));

    signal::Stack const signal_stack = signal::Stack{
        .ss_sp = mem,
        .ss_flags = 0,
        .ss_size = STACK_SIZE,
    };
    testing::assert_with_trace(
        sys::SigAltStackStatus::Ok ==
            sys::sigaltstack(&err_bt, &signal_stack, nullptr),
        &err_bt);
}
#else  // ifdef ZX_DEV
extern "C" void sigaltstack_() {}
#endif // ifdef ZX_DEV

extern "C" void thread_wrapper_(thread_fn tfn) { sigaltstack_(); }

inline SpawnStatus spawn(thread_fn, void *usr_ctx, SliceMut<u8> stack) {
    if (!__builtin_is_aligned(stack.ptr(), STACK_ALIGN)) {
        return SpawnStatus::StackUnaligned;
    }

    if (MIN_STACK_SIZE > stack.len()) {
        return SpawnStatus::StackTooSmall;
    }

    return SpawnStatus::Ok;
}

} // namespace thread

} // namespace zx
