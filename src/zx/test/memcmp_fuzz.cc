import zx;

using namespace zx;

static constexpr u64 PAGE = 4096;

// Markers written right outside of the compared ranges. The two sides get
// different values so that comparing past `count` is caught even where no
// guard page is adjacent.
static constexpr u8 LEFT_OUTSIDE = 0x5A;
static constexpr u8 RIGHT_OUTSIDE = 0xA5;
static constexpr u64 OUTSIDE_MARGIN = 64;

struct Ctx {
    SliceMut<u8> left;
    SliceMut<u8> right;
};

// `i * 37 + 11` has a full period modulo 256, so every byte value appears once
// per 256 bytes.
static u8 payload(u64 i) { return (u8)(i * 37 + 11); }

// Maps `usable` bytes with a PROT_NONE page on each side so that any read or
// write one byte outside of the returned slice faults.
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

// Accumulate instead of asserting per byte so the loop vectorizes.
static bool is_all(u8 const *ptr, u64 len, u8 val) {
    bool ok = true;
    for (u64 i = 0; i < len; i++) {
        ok &= ptr[i] == val;
    }
    return ok;
}

static void paint_outside(SliceMut<u8> mem, u64 off, u64 len, u8 val) {
    u8 *const ptr = mem.ptr();
    u64 const before = min_u64(off, OUTSIDE_MARGIN);
    for (u64 i = 1; i <= before; i++) {
        ptr[sub_unsafe_u64(off, i)] = val;
    }

    u64 const end = add_unsafe_u64(off, len);
    u64 const after = min_u64(sub_unsafe_u64(mem.len(), end), OUTSIDE_MARGIN);
    for (u64 i = 0; i < after; i++) {
        ptr[add_unsafe_u64(end, i)] = val;
    }
}

static void check_outside(SliceMut<u8> mem, u64 off, u64 len, u8 val) {
    u8 const *const ptr = mem.ptr();
    u64 const before = min_u64(off, OUTSIDE_MARGIN);
    assert(is_all(&ptr[sub_unsafe_u64(off, before)], before, val));

    u64 const end = add_unsafe_u64(off, len);
    u64 const after = min_u64(sub_unsafe_u64(mem.len(), end), OUTSIDE_MARGIN);
    assert(is_all(&ptr[end], after, val));
}

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    Ctx const *const ctx = (Ctx const *)usr_ctx;
    SliceMut<u8> const left = ctx->left;
    SliceMut<u8> const right = ctx->right;
    u64 const cap = left.len();
    assert(right.len() == cap);
    assert(cap > 0);

    u8 *const left_ptr = left.ptr();
    u8 *const right_ptr = right.ptr();

    u64 const r = fuzz_input->next();
    // `1/16` of the time compare nothing, so the zero length path is covered.
    u64 const len =
        (r % 16__wbu) == 0
            ? 0
            : min_u64(add_unsafe_u64(mod_unsafe_u64(r, cap), 1), cap);
    u64 const max_off = sub_unsafe_u64(cap, len);
    u64 const left_off = pick_offset(fuzz_input->next(), max_off);
    u64 const right_off = pick_offset(fuzz_input->next(), max_off);

    for (u64 i = 0; i < len; i++) {
        u8 const v = payload(i);
        left_ptr[add_unsafe_u64(left_off, i)] = v;
        right_ptr[add_unsafe_u64(right_off, i)] = v;
    }
    paint_outside(left, left_off, len, LEFT_OUTSIDE);
    paint_outside(right, right_off, len, RIGHT_OUTSIDE);

    // Half the time introduce a single difference. The index selection makes
    // sure it lands in the first block, the last block, the first 64 bytes and
    // at random positions (which covers both the 64 byte aligned loop and the
    // masked remainder at the end).
    u64 const d = fuzz_input->next();
    bool const differ = len > 0 && (d % 2__wbu) == 0;
    u64 diff_idx = 0;
    u8 replaced = 0;
    if (differ) {
        switch (shr_unsafe_u64(d, 1) % 4__wbu) {
        case 0: {
            diff_idx = 0;
            break;
        }
        case 1: {
            diff_idx = sub_unsafe_u64(len, 1);
            break;
        }
        case 2: {
            diff_idx = min_u64(63__wbu, sub_unsafe_u64(len, 1));
            break;
        }
        default: {
            diff_idx = mod_unsafe_u64(shr_unsafe_u64(d, 3), len);
            break;
        }
        }

        // Inverting never wraps, so the expected sign is unambiguous.
        replaced = (u8)(payload(diff_idx) ^ 0xFF__wbu);
        left_ptr[add_unsafe_u64(left_off, diff_idx)] = replaced;
    }

    // Expected result, computed before the call so that a write by memcmp
    // can't fake it.
    i32 expected = 0;
    for (u64 i = 0; i < len; i++) {
        u8 const l = left_ptr[add_unsafe_u64(left_off, i)];
        u8 const r2 = right_ptr[add_unsafe_u64(right_off, i)];
        if (l != r2) {
            expected = l > r2 ? 1 : -1;
            break;
        }
    }

    i32 const got = memcmp(&left_ptr[left_off], &right_ptr[right_off], len);
    if (expected == 0) {
        assert(got == 0);
    } else if (expected > 0) {
        assert(got > 0);
    } else {
        assert(got < 0);
    }

    // memcmp must not write anything.
    bool unchanged = true;
    for (u64 i = 0; i < len; i++) {
        u8 const v = payload(i);
        u8 const expected_left = differ && i == diff_idx ? replaced : v;
        unchanged &= left_ptr[add_unsafe_u64(left_off, i)] == expected_left;
        unchanged &= right_ptr[add_unsafe_u64(right_off, i)] == v;
    }
    assert(unchanged);
    check_outside(left, left_off, len, LEFT_OUTSIDE);
    check_outside(right, right_off, len, RIGHT_OUTSIDE);
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
        .left = make_guarded(&err_bt, 8192),
        .right = make_guarded(&err_bt, 8192),
    };
    fuzz(FuzzParams{
        .alloc = &alloc,
        .usr_ctx = (void *)&ctx,
        .fuzz_one = fuzz_one,
    });
    return 0;
}
