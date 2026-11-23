import zx;

using namespace zx;

static constexpr u64 PAGE = 4096;

// Marker written to the memory right outside of the range a function is
// allowed to touch. Any write there is a bug.
static constexpr u8 OUTSIDE = 0xA5;

// How many bytes on each side of a range are painted/checked. One block wide:
// a full 64 byte block is the most any of these functions can touch past the
// end of the range it was given.
static constexpr u64 MARGIN = 64;

// Byte at index `i` of a test payload. `i * 37 + 11` has a full period modulo
// 256, so every byte value appears once per 256 bytes.
static u8 pattern(u64 i) { return (u8)(i * 37 + 11); }

struct Buf {
    u8 *ptr;
    u64 len;
};

// Maps `usable` bytes with a PROT_NONE page on each side. Any read or write one
// byte before/after the buffer faults instead of silently touching a
// neighbouring mapping.
static Buf make_guarded(Backtrace *err_bt, u64 usable) {
    assert(usable > 0);
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

    return Buf{
        .ptr = &base[PAGE],
        .len = usable,
    };
}

static void fill(u8 *ptr, u64 len, u8 val) {
    for (u64 i = 0; i < len; i++) {
        ptr[i] = val;
    }
}

static void fill_pattern(u8 *ptr, u64 len) {
    for (u64 i = 0; i < len; i++) {
        ptr[i] = pattern(i);
    }
}

static bool is_all(u8 const *ptr, u64 len, u8 val) {
    bool ok = true;
    for (u64 i = 0; i < len; i++) {
        ok &= ptr[i] == val;
    }
    return ok;
}

static bool is_pattern(u8 const *ptr, u64 len) {
    bool ok = true;
    for (u64 i = 0; i < len; i++) {
        ok &= ptr[i] == pattern(i);
    }
    return ok;
}

static void paint_outside(Buf b, u64 off, u64 len, u8 val) {
    assert(add_unsafe_u64(off, len) <= b.len);
    u64 const before = min_u64(off, MARGIN);
    fill(&b.ptr[sub_unsafe_u64(off, before)], before, val);

    u64 const end = add_unsafe_u64(off, len);
    u64 const after = min_u64(sub_unsafe_u64(b.len, end), MARGIN);
    fill(&b.ptr[end], after, val);
}

static void check_outside(Buf b, u64 off, u64 len, u8 val) {
    u64 const before = min_u64(off, MARGIN);
    assert(is_all(&b.ptr[sub_unsafe_u64(off, before)], before, val));

    u64 const end = add_unsafe_u64(off, len);
    u64 const after = min_u64(sub_unsafe_u64(b.len, end), MARGIN);
    assert(is_all(&b.ptr[end], after, val));
}

static void check_memcpy(Buf dst, Buf src, u64 dst_off, u64 src_off, u64 len) {
    assert(add_unsafe_u64(dst_off, len) <= dst.len);
    assert(add_unsafe_u64(src_off, len) <= src.len);

    fill_pattern(&src.ptr[src_off], len);
    paint_outside(dst, dst_off, len, OUTSIDE);

    void *const expected = &dst.ptr[dst_off];
    assert(memcpy(expected, &src.ptr[src_off], len) == expected);
    assert(is_pattern(&dst.ptr[dst_off], len));

    check_outside(dst, dst_off, len, OUTSIDE);
}

static void check_memset(Buf b, u64 off, u64 len, u8 byte) {
    assert(add_unsafe_u64(off, len) <= b.len);
    // The fill byte has to be distinguishable from the outside marker.
    assert(byte != OUTSIDE);

    paint_outside(b, off, len, OUTSIDE);

    void *const expected = &b.ptr[off];
    assert(memset(expected, byte, len) == expected);
    assert(is_all(&b.ptr[off], len, byte));

    check_outside(b, off, len, OUTSIDE);
}

// `diff_at < len` means the first differing byte is at `diff_at`; the byte is
// inverted so it never wraps around and the expected sign is obvious.
static void check_memcmp(Buf left, Buf right, u64 left_off, u64 right_off,
                         u64 len, u64 diff_at) {
    assert(add_unsafe_u64(left_off, len) <= left.len);
    assert(add_unsafe_u64(right_off, len) <= right.len);
    assert(diff_at <= len);

    for (u64 i = 0; i < len; i++) {
        u8 const v = pattern(i);
        left.ptr[add_unsafe_u64(left_off, i)] = v;
        right.ptr[add_unsafe_u64(right_off, i)] = v;
    }

    // Different garbage on each side right outside of the compared ranges so
    // that comparing past `len` is caught even without a guard page there.
    paint_outside(left, left_off, len, 0x5A);
    paint_outside(right, right_off, len, 0xA5);

    u8 replaced = 0;
    bool is_greater = false;
    if (diff_at < len) {
        u8 const v = pattern(diff_at);
        replaced = (u8)(v ^ 0xFF__wbu);
        is_greater = replaced > v;
        left.ptr[add_unsafe_u64(left_off, diff_at)] = replaced;
    }

    i32 const got = memcmp(&left.ptr[left_off], &right.ptr[right_off], len);
    if (diff_at == len) {
        assert(got == 0);
    } else if (is_greater) {
        assert(got > 0);
    } else {
        assert(got < 0);
    }

    // memcmp must not write anything.
    bool unchanged = true;
    for (u64 i = 0; i < len; i++) {
        u8 expected = pattern(i);
        if (i == diff_at) {
            expected = replaced;
        }
        unchanged &= left.ptr[add_unsafe_u64(left_off, i)] == expected;
        unchanged &= right.ptr[add_unsafe_u64(right_off, i)] == pattern(i);
    }
    assert(unchanged);
    check_outside(left, left_off, len, 0x5A);
    check_outside(right, right_off, len, 0xA5);
}

// Puts the string at `off` with its first zero byte at `off + pos`.
static void check_strlen(Buf b, u64 off, u64 pos) {
    assert(add_unsafe_u64(off, pos) < b.len);
    u64 const end = add_unsafe_u64(off, pos);

    // Zeroed bytes before the string start: for an unaligned `str` these are
    // inside the 64 byte aligned-down block that strlen loads, so a broken head
    // mask would return 0 here. Everything up to the terminator is nonzero.
    fill(b.ptr, off, 0);
    for (u64 i = off; i < end; i++) {
        b.ptr[i] = (u8)(pattern(i) | 1__wbu);
    }
    b.ptr[end] = 0;

    assert(strlen((char const *)&b.ptr[off]) == pos);
}

int main() {
    Backtrace err_bt = Backtrace::init();

    // 2 pages of usable memory: long enough for a copy that goes through
    // several 64 byte iterations while still touching both guard pages.
    Buf const dst = make_guarded(&err_bt, 8192);
    Buf const src = make_guarded(&err_bt, 8192);
    u64 const cap = dst.len;
    assert(src.len == cap);

    u64 const lens[] = {0,  1,  2,   3,   15,  16,  17,  31,  32,  33,   63,
                        64, 65, 127, 128, 129, 255, 256, 257, 512, 1024, 8192};
    u64 const n_lens = sizeof(lens) / sizeof(lens[0]);

    // Destination/source phase pairs. Together they cover: both aligned, one
    // aligned/one not, equal phases, different phases, and the 32 byte mask
    // boundary (31/32/33) plus the maximum misalignment (63).
    u64 const pairs[][2] = {{0, 0},  {0, 1},   {0, 31}, {0, 32},  {0, 33},
                            {0, 63}, {1, 1},   {1, 0},  {31, 31}, {32, 32},
                            {32, 0}, {63, 63}, {63, 0}};
    u64 const n_pairs = sizeof(pairs) / sizeof(pairs[0]);

    u64 const str_offs[] = {0, 1, 2, 3, 5, 8, 16, 31, 32, 33, 47, 63, 64, 127};
    u64 const n_str_offs = sizeof(str_offs) / sizeof(str_offs[0]);
    u64 const positions[] = {0,   1,   2,   63,   64,   65,
                             127, 255, 511, 1023, 2047, 4095};
    u64 const n_positions = sizeof(positions) / sizeof(positions[0]);

    for (u64 i = 0; i < n_lens; i++) {
        u64 const len = lens[i];
        assert(len <= cap);
        u64 const tail = sub_unsafe_u64(cap, len);

        for (u64 p = 0; p < n_pairs; p++) {
            u64 const dst_off = pairs[p][0];
            u64 const src_off = pairs[p][1];
            if (add_unsafe_u64(dst_off, len) > cap ||
                add_unsafe_u64(src_off, len) > cap) {
                continue;
            }

            check_memcpy(dst, src, dst_off, src_off, len);

            check_memcmp(dst, src, dst_off, src_off, len, len);
            if (len > 0) {
                check_memcmp(dst, src, dst_off, src_off, len, 0);
                check_memcmp(dst, src, dst_off, src_off, len,
                             sub_unsafe_u64(len, 1));
            }
        }

        // Placements where one of the buffers touches its trailing guard page.
        check_memcpy(dst, src, 0, tail, len);
        check_memcpy(dst, src, tail, 0, len);
        check_memcpy(dst, src, tail, tail, len);

        // memset with the bytes that are interesting for the broadcast (all
        // ones, all zeros and something in between).
        u64 const memset_offs[] = {0, 1, 32, 63, tail};
        u64 const n_memset_offs = sizeof(memset_offs) / sizeof(memset_offs[0]);
        for (u64 m = 0; m < n_memset_offs; m++) {
            u64 const off = memset_offs[m];
            if (add_unsafe_u64(off, len) > cap) {
                continue;
            }
            check_memset(dst, off, len, 0x69);
            check_memset(dst, off, len, 0xFF);
            check_memset(dst, off, len, 0x00);
        }
    }

    // strlen: every start phase, with the terminator inside the first block, at
    // each block boundary, in the middle and at the last byte of the window.
    for (u64 a = 0; a < n_str_offs; a++) {
        u64 const off = str_offs[a];
        for (u64 b = 0; b < n_positions; b++) {
            u64 const pos = positions[b];
            if (add_unsafe_u64(off, pos) < cap) {
                check_strlen(dst, off, pos);
            }
        }

        // Terminator at the very last byte of the usable window: the aligned
        // block that contains it ends at the trailing guard page.
        check_strlen(dst, off, sub_unsafe_u64(sub_unsafe_u64(cap, off), 1));
    }

    return 0;
}
