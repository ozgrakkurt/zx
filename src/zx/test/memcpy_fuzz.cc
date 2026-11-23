import zx;

using namespace zx;

static constexpr u64 PAGE = 4096;

// Marker written to the memory right outside of the range memcpy is allowed to
// touch.
static constexpr u8 OUTSIDE = 0xA5;
static constexpr u64 OUTSIDE_MARGIN = 64;

// Mirrors the `rep movsb` threshold in `libc.cppm`. A copy has to be at least
// this big before memcpy takes that branch.
static constexpr u64 REP_MOVSB_MIN = u64{48} << 20__wbu;

struct Ctx {
    SliceMut<u8> dst;
    SliceMut<u8> src;
    // Only faulted in when the (rare) `rep movsb` case is selected: mmap does
    // not populate them.
    SliceMut<u8> big_dst;
    SliceMut<u8> big_src;
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
static bool is_payload(u8 const *ptr, u64 len) {
    bool ok = true;
    for (u64 i = 0; i < len; i++) {
        ok &= ptr[i] == payload(i);
    }
    return ok;
}

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

static void do_copy(SliceMut<u8> dst, SliceMut<u8> src, u64 dst_off,
                    u64 src_off, u64 len) {
    assert(add_unsafe_u64(dst_off, len) <= dst.len());
    assert(add_unsafe_u64(src_off, len) <= src.len());

    u8 *const src_ptr = src.ptr();
    for (u64 i = 0; i < len; i++) {
        src_ptr[add_unsafe_u64(src_off, i)] = payload(i);
    }
    paint_outside(dst, dst_off, len, OUTSIDE);

    u8 *const expected = &dst.ptr()[dst_off];
    assert(memcpy(expected, &src_ptr[src_off], len) == expected);
    assert(is_payload(expected, len));

    check_outside(dst, dst_off, len, OUTSIDE);
}

// Same as `do_copy` but only fills and checks a few chunks spread over the
// range. Filling and verifying all 48 MiB on every hit would dominate the
// fuzzing time; the chunks still include the first and the last bytes, so a
// short copy is caught.
static void do_big_copy(SliceMut<u8> dst, SliceMut<u8> src, u64 dst_off,
                        u64 src_off, u64 len) {
    assert(len >= REP_MOVSB_MIN);
    assert(add_unsafe_u64(dst_off, len) <= dst.len());
    assert(add_unsafe_u64(src_off, len) <= src.len());

    u8 *const dst_ptr = dst.ptr();
    u8 *const src_ptr = src.ptr();

    u64 const chunk = 4096;
    u64 const chunks[5] = {0, len / 4, len / 2, len / 4 * 3,
                           sub_unsafe_u64(len, chunk)};
    for (u64 c = 0; c < 5; c++) {
        u64 const base = chunks[c];
        for (u64 i = 0; i < chunk; i++) {
            src_ptr[add_unsafe_u64(src_off, add_unsafe_u64(base, i))] =
                payload(base + i);
        }
    }

    paint_outside(dst, dst_off, len, OUTSIDE);

    void *const expected = &dst_ptr[dst_off];
    assert(memcpy(expected, &src_ptr[src_off], len) == expected);

    bool ok = true;
    for (u64 c = 0; c < 5; c++) {
        u64 const base = chunks[c];
        for (u64 i = 0; i < chunk; i++) {
            ok &= dst_ptr[add_unsafe_u64(dst_off, add_unsafe_u64(base, i))] ==
                  payload(base + i);
        }
    }
    assert(ok);

    check_outside(dst, dst_off, len, OUTSIDE);
}

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    Ctx const *const ctx = (Ctx const *)usr_ctx;

    u64 const r = fuzz_input->next();

    // Rarely take the `rep movsb` path. It requires a copy of at least 48 MiB,
    // which is far too slow to do for every input, but it still has to run
    // often enough for the fuzzer to find it.
    //
    // The trigger is a bit pattern that the fuzzer's magic values (0 and
    // U64_MAX) can't produce, otherwise those two would take this path on
    // every iteration that mutates the first word.
    if ((r & 0xFFF__wbu) == 0x5AA__wbu) {
        SliceMut<u8> const big_dst = ctx->big_dst;
        SliceMut<u8> const big_src = ctx->big_src;
        u64 const big_cap = big_dst.len();
        assert(big_src.len() == big_cap);

        u64 const slack = sub_unsafe_u64(big_cap, REP_MOVSB_MIN);
        u64 const big_len = add_unsafe_u64(
            REP_MOVSB_MIN,
            mod_unsafe_u64(shr_unsafe_u64(r, 8), add_unsafe_u64(slack, 1)));
        u64 const max_off = sub_unsafe_u64(big_cap, big_len);
        u64 const dst_off = pick_offset(shr_unsafe_u64(r, 16), max_off);
        u64 const src_off = pick_offset(shr_unsafe_u64(r, 32), max_off);
        do_big_copy(big_dst, big_src, dst_off, src_off, big_len);
        return;
    }

    SliceMut<u8> const dst = ctx->dst;
    SliceMut<u8> const src = ctx->src;
    u64 const cap = dst.len();
    assert(src.len() == cap);

    // `1/16` of the time copy nothing, so the zero length path is covered.
    u64 const len =
        (r % 16__wbu) == 0
            ? 0
            : min_u64(add_unsafe_u64(mod_unsafe_u64(r, cap), 1), cap);
    u64 const max_off = sub_unsafe_u64(cap, len);
    u64 const dst_off = pick_offset(fuzz_input->next(), max_off);
    u64 const src_off = pick_offset(fuzz_input->next(), max_off);

    do_copy(dst, src, dst_off, src_off, len);
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
        .dst = make_guarded(&err_bt, 8192),
        .src = make_guarded(&err_bt, 8192),
        .big_dst = make_guarded(&err_bt, add_unsafe_u64(REP_MOVSB_MIN, PAGE)),
        .big_src = make_guarded(&err_bt, add_unsafe_u64(REP_MOVSB_MIN, PAGE)),
    };
    fuzz(FuzzParams{
        .alloc = &alloc,
        .usr_ctx = (void *)&ctx,
        .fuzz_one = fuzz_one,
    });
    return 0;
}
