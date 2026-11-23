import zx;

using namespace zx;

static constexpr u64 PAGE = 4096;

struct Ctx {
    SliceMut<u8> mem;
};

// `i * 37 + 11` has a full period modulo 256, so every byte value appears once
// per 256 bytes.
static u8 payload(u64 i) { return (u8)(i * 37 + 11); }

// Maps `usable` bytes with a PROT_NONE page on each side so that any read one
// byte before/after the returned slice faults.
static SliceMut<u8> make_guarded(Backtrace *err_bt, u64 usable) {
    u64 const mapped = add_unsafe_u64(usable, 2 * PAGE);

    u8 *base = nullptr;
    testing::assert_with_trace(
        sys::MMapStatus::Ok ==
            sys::mmap(err_bt, nullptr, mapped, sys::PROT_READ | sys::PROT_WRITE,
                      sys::MAP_PRIVATE | sys::MAP_ANONYMOUS, -1, 0, &base),
        err_bt);
    assert(base != nullptr);

    testing::assert_with_trace(
        sys::MProtectStatus::Ok ==
            sys::mprotect(err_bt, base, PAGE, sys::PROT_NONE),
        err_bt);
    testing::assert_with_trace(
        sys::MProtectStatus::Ok ==
            sys::mprotect(err_bt, &base[mapped - PAGE], PAGE, sys::PROT_NONE),
        err_bt);

    return SliceMut<u8>::init_unsafe(&base[PAGE], usable);
}

// Picks an offset in `[0, max_off]`. Covers the placements that matter: the
// start of the window (touches the leading guard page), the end of the window
// (touches the trailing guard page), small offsets that cover every 64 byte
// phase, and arbitrary offsets.
static u64 pick_offset(u64 bits, u64 max_off) {
    switch (bits % 4__wbu) {
    case 0: {
        return 0;
    }
    case 1: {
        return max_off;
    }
    case 2: {
        return min_u64(bits % 64__wbu, max_off);
    }
    default: {
        return mod_unsafe_u64(bits, add_unsafe_u64(max_off, 1));
    }
    }
}

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    SliceMut<u8> const mem = ((Ctx const *)usr_ctx)->mem;
    u64 const cap = mem.len();
    assert(cap > 0);

    u64 const off = pick_offset(fuzz_input->next(), sub_unsafe_u64(cap, 1));
    u64 const max_pos = sub_unsafe_u64(cap, add_unsafe_u64(off, 1));

    // Bytes before the string start are zero. For an unaligned `str` they are
    // part of the 64 byte aligned-down block that strlen loads, so a broken
    // mask in that path makes strlen return 0.
    for (u64 i = 0; i < off; i++) {
        mem.write_unsafe(i, 0);
    }
    // Everything from the start on is nonzero except for the terminator.
    for (u64 i = off; i < cap; i++) {
        mem.write_unsafe(i, (u8)(payload(i) | 1__wbu));
    }

    // Bias the terminator position so that the first (masked, when `off` is
    // unaligned) block, the last byte of the window and arbitrary blocks are
    // all covered.
    u64 const s = fuzz_input->next();
    u64 pos;
    switch (s % 4__wbu) {
    case 0: {
        u64 const diff = mod_unsafe_u64(off, 64);
        u64 const span = diff == 0 ? min_u64(64, add_unsafe_u64(max_pos, 1))
                                   : min_u64(sub_unsafe_u64(64, diff),
                                             add_unsafe_u64(max_pos, 1));
        pos = mod_unsafe_u64(s, span);
        break;
    }
    case 1: {
        // Terminator at the very last byte of the window: the aligned block
        // that contains it ends right at the trailing guard page.
        pos = max_pos;
        break;
    }
    default: {
        pos = mod_unsafe_u64(s, add_unsafe_u64(max_pos, 1));
        break;
    }
    }

    mem.write_unsafe(add_unsafe_u64(off, pos), 0);

    u64 const got = strlen((char const *)&mem.ptr()[off]);
    assert(got == pos);
}

static Alloc make_alloc(Backtrace *err_bt, u64 size) {
    u8 *mem = nullptr;
    testing::assert_with_trace(page_alloc::AllocStatus::Ok ==
                                   page_alloc::alloc_4kb(err_bt, size, &mem),
                               err_bt);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, size));

    return alloc;
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    Alloc alloc = make_alloc(&err_bt, FUZZ_MEM_CAP);
    Ctx const ctx = Ctx{
        .mem = make_guarded(&err_bt, 8192),
    };
    fuzz(FuzzParams{
        .alloc = &alloc,
        .usr_ctx = (void *)&ctx,
        .fuzz_one = fuzz_one,
    });
    return 0;
}
