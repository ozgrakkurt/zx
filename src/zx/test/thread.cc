import zx;

using namespace zx;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -E
// "__NR_sched_yield\b"
constexpr u64 NR_SCHED_YIELD = 24;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -E "__NR_exit\b"
constexpr u64 NR_EXIT = 60;

// a spawned thread should start running almost immediately, if it doesn't run
// within this many yields then something is wrong.
constexpr u64 SPIN_LIMIT = 100000;

// wait until value becomes expected without burning cpu. panics instead of
// hanging forever if the thread never runs.
static void wait_value(u64_c const *value, u64_c expected) {
    for (u64 i = 0; i < SPIN_LIMIT; i++) {
        if (__atomic_load_n(value, __ATOMIC_ACQUIRE) == expected) {
            return;
        }

        // yield so a spawned thread gets cpu time even on a machine with fewer
        // cpus than threads.
        syscall0(NR_SCHED_YIELD);
    }

    assert(false);
}

// terminate only the calling thread, exit_group would kill the whole test
// process.
[[noreturn]] static void exit_thread() {
    syscall1(NR_EXIT, 0);

    // the syscall never returns, this is only here for the noreturn
    // annotation.
    __builtin_trap();
}

struct MapsEntry {
    u64 start;
    u64 end;
    bool can_read;
    bool can_write;
    bool can_exec;
    bool is_private;
};

// /proc/self/maps has a line per mapping. A 64KiB buffer is plenty for this
// test process.
static u8 maps_buf[1 << 16];
static u64 maps_len = 0;

static bool parse_hex(u8 const **cursor, u8 const *end, u64 *out) {
    u64 value = 0;
    bool found_digit = false;

    while (*cursor < end) {
        u8 const c = **cursor;

        u64 digit = 0;
        if (c >= '0' && c <= '9') {
            digit = (u64)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = (u64)(c - 'a') + 10;
        } else {
            break;
        }

        value = (value << 4) | digit;
        found_digit = true;
        *cursor += 1;
    }

    *out = value;
    return found_digit;
}

// parse a line of the form:
//   7f8c3c000000-7f8c3c100000 rw-p 00000000 00:00 0
static bool parse_maps_entry(u8 const *line, u8 const *line_end,
                             MapsEntry *out) {
    u8 const *cursor = line;
    MapsEntry entry = {};

    if (!parse_hex(&cursor, line_end, &entry.start)) {
        return false;
    }

    if (cursor == line_end || *cursor != '-') {
        return false;
    }
    cursor += 1;

    if (!parse_hex(&cursor, line_end, &entry.end)) {
        return false;
    }

    if (cursor == line_end || *cursor != ' ') {
        return false;
    }
    cursor += 1;

    if ((u64)(line_end - cursor) < 4) {
        return false;
    }

    entry.can_read = cursor[0] == 'r';
    entry.can_write = cursor[1] == 'w';
    entry.can_exec = cursor[2] == 'x';
    entry.is_private = cursor[3] == 'p';

    *out = entry;
    return true;
}

static void load_maps() {
    i32 fd = -1;
    assert(sys::OpenStatus::Ok == sys::open(nullptr,
                                            (u8 const *)"/proc/self/maps",
                                            sys::O_RDONLY, 0, &fd));

    u64 total = 0;
    while (total < (u64)sizeof(maps_buf)) {
        u64 n_read = 0;
        assert(sys::ReadStatus::Ok == sys::read(nullptr, fd, &maps_buf[total],
                                                (u64)sizeof(maps_buf) - total,
                                                &n_read));
        if (n_read == 0) {
            break;
        }

        total = add_unsafe_u64(total, n_read);
    }

    assert(sys::CloseStatus::Ok == sys::close(nullptr, fd));

    maps_len = total;
    assert(maps_len != 0);
}

static bool find_maps_entry(u64 addr, MapsEntry *out) {
    u64 pos = 0;
    while (pos < maps_len) {
        u64 line_end = pos;
        while (line_end < maps_len && maps_buf[line_end] != '\n') {
            line_end = add_unsafe_u64(line_end, 1);
        }

        MapsEntry entry = {};
        if (parse_maps_entry(&maps_buf[pos], &maps_buf[line_end], &entry) &&
            entry.start <= addr && addr < entry.end) {
            *out = entry;
            return true;
        }

        pos = add_unsafe_u64(line_end, 1);
    }

    return false;
}

// the spawned thread has to run on the dedicated stack that spawn allocated
// for it, not on the stack of the thread that spawned it. that stack is laid
// out as [guard page] ---- stack ---- [guard page] and the guard pages are
// PROT_NONE, so they show up as inaccessible mappings in /proc/self/maps.
static void check_thread_stack(u64 stack_probe) {
    load_maps();

    MapsEntry stack = {};
    assert(find_maps_entry(stack_probe, &stack));
    assert(stack.can_read);
    assert(stack.can_write);
    assert(!stack.can_exec);
    assert(stack.is_private);

    assert(sub_unsafe_u64(stack.end, stack.start) == thread::STACK_SIZE);

    MapsEntry lower_guard = {};
    assert(find_maps_entry(sub_unsafe_u64(stack.start, 1), &lower_guard));
    assert(!lower_guard.can_read);
    assert(!lower_guard.can_write);
    assert(!lower_guard.can_exec);
    assert(lower_guard.end == stack.start);
    assert(sub_unsafe_u64(stack.start, lower_guard.start) >=
           thread::GUARD_PAGE_SIZE);

    MapsEntry upper_guard = {};
    assert(find_maps_entry(stack.end, &upper_guard));
    assert(!upper_guard.can_read);
    assert(!upper_guard.can_write);
    assert(!upper_guard.can_exec);
    assert(upper_guard.start == stack.end);
    assert(sub_unsafe_u64(upper_guard.end, stack.end) >=
           thread::GUARD_PAGE_SIZE);
}

// touch a large part of the stack to make sure the stack that spawn allocated
// is actually usable. leave half of it untouched so the rest of the call chain
// has plenty of room above the bottom guard page.
static bool touch_deep_stack() {
    volatile u8 buf[thread::STACK_SIZE / 2];

    for (u64 i = 0; i < (u64)sizeof(buf); i += 4096) {
        // make every page hold a different value so writing then reading back
        // actually verifies something.
        buf[i] = (u8)(i / 4096);
    }

    for (u64 i = 0; i < (u64)sizeof(buf); i += 4096) {
        if (buf[i] != (u8)(i / 4096)) {
            return false;
        }
    }

    return true;
}

struct BasicCtx {
    u64_c done;
    u64 self_seen;
    u64 stack_probe;
    u64 deep_stack_ok;
    u64 ppid;
#ifdef ZX_DEBUG_INFO
    u64 altstack_ok;
#endif
};

// has to use the GNU attribute instead of [[noreturn]] because the noreturn
// attribute is part of the thread_fn function pointer type.
__attribute__((noreturn)) static void basic_fn(void *usr_ctx) {
    BasicCtx *ctx = (BasicCtx *)usr_ctx;

    u8 probe;
    ctx->self_seen = (u64)usr_ctx;
    ctx->stack_probe = (u64)&probe;
    ctx->deep_stack_ok = touch_deep_stack() ? 1 : 0;
    ctx->ppid = sys::getppid();

#ifdef ZX_DEBUG_INFO
    // thread entry should have installed a per-thread alternate signal stack.
    signal::Stack old_stack = {};
    Backtrace err_bt = Backtrace::init();
    ctx->altstack_ok = (sys::SigAltStackStatus::Ok ==
                        sys::sigaltstack(&err_bt, nullptr, &old_stack)) &&
                       old_stack.ss_sp != nullptr && old_stack.ss_flags == 0 &&
                       old_stack.ss_size >= 4096;
#endif

    __atomic_store_n(&ctx->done, 1, __ATOMIC_RELEASE);

    exit_thread();
}

static void test_invalid_args() {
    Backtrace err_bt = Backtrace::init();

    // a null thread function is rejected.
    assert(thread::SpawnStatus::Nullptr ==
           thread::spawn(&err_bt, nullptr, nullptr));
}

static void test_basic_spawn() {
    Backtrace err_bt = Backtrace::init();

    BasicCtx ctx = {};

    testing::assert_with_trace(thread::SpawnStatus::Ok ==
                                   thread::spawn(&err_bt, basic_fn, &ctx),
                               &err_bt);

    wait_value(&ctx.done, 1);

    // the thread got the exact context pointer that was passed in.
    assert(ctx.self_seen == (u64)&ctx);

    // the thread is in the same process so it sees the same parent.
    assert(ctx.ppid == sys::getppid());

    // the thread should have been able to use a large part of the stack that
    // spawn allocated for it.
    assert(ctx.deep_stack_ok != 0);

    // the thread has to run on its own stack, not on the stack of the thread
    // that spawned it, and that stack has to have a guard page on each end.
    check_thread_stack(ctx.stack_probe);

#ifdef ZX_DEBUG_INFO
    assert(ctx.altstack_ok != 0);
#endif

    // intentionally not freeing the stack: there is no join() yet so the
    // thread could still be between the flag store and its exit syscall, and
    // spawn owns the stack anyway. The process tears everything down when the
    // tests are done.
}

constexpr u64 N_THREADS = 3;

struct MultiCtx {
    u64_c counter;
    u64_c done;
};

struct MultiItem {
    MultiCtx *shared;
    u64 self_seen;
    u64 stack_probe;
};

__attribute__((noreturn)) static void multi_fn(void *usr_ctx) {
    MultiItem *item = (MultiItem *)usr_ctx;

    __atomic_fetch_add(&item->shared->counter, 1, __ATOMIC_ACQ_REL);

    u8 probe;
    item->self_seen = (u64)usr_ctx;
    item->stack_probe = (u64)&probe;

    __atomic_fetch_add(&item->shared->done, 1, __ATOMIC_ACQ_REL);

    exit_thread();
}

static void test_multiple_threads() {
    Backtrace err_bt = Backtrace::init();

    MultiCtx shared = {};
    MultiItem items[N_THREADS] = {};

    for (u64 i = 0; i < N_THREADS; i++) {
        items[i].shared = &shared;

        testing::assert_with_trace(
            thread::SpawnStatus::Ok ==
                thread::spawn(&err_bt, multi_fn, &items[i]),
            &err_bt);
    }

    wait_value(&shared.done, N_THREADS);

    // every thread incremented the shared counter exactly once.
    assert(__atomic_load_n(&shared.counter, __ATOMIC_ACQUIRE) == N_THREADS);

    for (u64 i = 0; i < N_THREADS; i++) {
        // each thread got its own context.
        assert(items[i].self_seen == (u64)&items[i]);

        // and each one ran on its own stack.
        check_thread_stack(items[i].stack_probe);

        for (u64 j = 0; j < i; j++) {
            assert(items[i].stack_probe != items[j].stack_probe);
        }
    }

    // stacks are not freed for the same reason as in test_basic_spawn.
}

int main() {
    test_invalid_args();
    test_basic_spawn();
    test_multiple_threads();
}
