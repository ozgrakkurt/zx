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

extern "C" [[noreturn]] void thread_wrapper_(thread_fn tfn) {
    sigaltstack_();

    // This exits the thread but doesn't exit the whole process like exit_group does.
    // We can't normally return here because there is no return address on the stack.
    sys::exit();
}

constexpr u64 RED_ZONE_SIZE = 16;
constexpr u64 MIN_STACK_SIZE = u64{1} << 17;

inline SpawnStatus spawn(thread_fn, void *usr_ctx, SliceMut<u8> stack) {
    if (!__builtin_is_aligned(stack.ptr(), STACK_ALIGN)) {
        return SpawnStatus::StackUnaligned;
    }

    if (MIN_STACK_SIZE > stack.len()) {
        return SpawnStatus::StackTooSmall;
    }

    u64 const stack_offset = sub_unsafe_u64(stack.len(), RED_ZONE_SIZE + sizeof(u64));
    u8* stack_ptr = stack.offset_unsafe(stack_offset);

    u64_c res;
    __asm__ volatile(
        "\n\t"
        "\n\t"
        : [res]"=a"(res)
        : [stack_ptr] "r"(stack_ptr)
        : "memory"
    );

    return SpawnStatus::Ok;
}

} // namespace thread

} // namespace zx
