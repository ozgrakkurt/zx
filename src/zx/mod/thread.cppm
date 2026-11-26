export module zx:thread;
import :signal;
import :page_alloc;
import :def;
import :sys;
import :slice;
import :backtrace;
import :testing;

export namespace zx {

namespace thread {

enum class [[nodiscard]] SpawnStatus : u8_c {
    Ok = 0,
    StackTooSmall,
    StackUnaligned,
    UnexpectedErr,
    EAgain,
    EInval,
    ENoMem,
    EPerm,
};

constexpr u64 STACK_ALIGN = 16;

typedef void thread_fn(void *usr_ctx);

extern "C" zx_thread_spawn_();

SpawnStatus spawn(thread_fn, void *usr_ctx, SliceMut<u8> stack);

} // namespace thread

} // namespace zx
