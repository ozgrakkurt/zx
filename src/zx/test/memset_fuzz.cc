import zx;

using namespace zx;

static constexpr u64 PAGE = 4096;

// How many bytes on each side of the filled range are painted/checked.
static constexpr u64 OUTSIDE_MARGIN = 64;

struct Ctx {
    SliceMut<u8> mem;
};

// Allocates `usable` bytes with a PROT_NONE page on each side so that any read
// or write one byte outside of the returned slice faults.
//
// The page allocator only hands out power-of-two sizes, so the usable part is
// rounded up to whatever the next power of two leaves after the two guard
// pages.
static SliceMut<u8> make_guarded(Backtrace *err_bt, u64 usable) {
    bool overflow = false;
    u64 const total =
        next_pow_two_u64(add_unsafe_u64(usable, 2 * PAGE), &overflow);
    assert(!overflow);
    assert(total >= add_unsafe_u64(usable, 2 * PAGE));

    u8 *base = nullptr;
    testing::assert_with_trace(page_alloc::AllocStatus::Ok ==
                                   page_alloc::alloc_4kb(err_bt, total, &base),
                               err_bt);
    assert(base != nullptr);

    testing::assert_with_trace(
        sys::MProtectStatus::Ok ==
            sys::mprotect(err_bt, base, PAGE, sys::PROT_NONE),
        err_bt);
    testing::assert_with_trace(
        sys::MProtectStatus::Ok ==
            sys::mprotect(err_bt, &base[sub_unsafe_u64(total, PAGE)], PAGE,
                          sys::PROT_NONE),
        err_bt);

    return SliceMut<u8>::init_unsafe(&base[PAGE],
                                     sub_unsafe_u64(total, 2 * PAGE));
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

// Accumulate instead of asserting per byte so the loop vectorizes.
static bool is_all(u8 const *ptr, u64 len, u8 val) {
    bool ok = true;
    for (u64 i = 0; i < len; i++) {
        ok &= ptr[i] == val;
    }
    return ok;
}

__attribute__((no_builtin)) static void fuzz_one(void *usr_ctx,
                                                 FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    SliceMut<u8> const mem = ((Ctx const *)usr_ctx)->mem;
    u8 *const ptr = mem.ptr();
    u64 const cap = mem.len();
    assert(cap > 0);

    u64 const r = fuzz_input->next();
    // `1/16` of the time fill nothing, so the zero length path is covered.
    u64 const len =
        (r % 16__wbu) == 0
            ? 0
            : min_u64(add_unsafe_u64(mod_unsafe_u64(r, cap), 1), cap);
    u64 const max_off = sub_unsafe_u64(cap, len);
    u64 const off = pick_offset(fuzz_input->next(), max_off);
    u8 const byte = (u8)fuzz_input->next();
    // `wrapping_add` never yields `byte` itself, so writes outside of the
    // filled range can't hide behind the fill value.
    u8 const marker = wrapping_add_u8(byte, 1__wbu);

    u64 const end = add_unsafe_u64(off, len);
    u64 const before = min_u64(off, OUTSIDE_MARGIN);
    for (u64 i = 1; i <= before; i++) {
        ptr[sub_unsafe_u64(off, i)] = marker;
    }
    u64 const after = min_u64(sub_unsafe_u64(cap, end), OUTSIDE_MARGIN);
    for (u64 i = 0; i < after; i++) {
        ptr[add_unsafe_u64(end, i)] = marker;
    }

    // Dirty the range with the marker (it can't equal `byte`) so a fill that
    // writes nothing (or skips part of the range) can't pass by matching what
    // was already there.
    for (u64 i = 0; i < len; i++) {
        ptr[add_unsafe_u64(off, i)] = marker;
    }

    void *const expected = &ptr[off];
    assert(memset(expected, byte, len) == expected);

    assert(is_all(&ptr[off], len, byte));
    assert(is_all(&ptr[sub_unsafe_u64(off, before)], before, marker));
    assert(is_all(&ptr[end], after, marker));
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
