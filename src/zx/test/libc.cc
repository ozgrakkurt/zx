import zx;

using namespace zx;

__attribute__((no_builtin)) static void check_memcpy(u8 *dst, u8 *src,
                                                     u64 len) {
    for (u64 i = 0; i < len; i++) {
        src[i] = (u8)i;
        dst[i] = 69__wbu;
    }

    void *out_dst = memcpy(dst, src, len);
    assert(out_dst == dst);

    for (u64 i = 0; i < len; i++) {
        assert(src[i] == dst[i]);
    }
}

__attribute__((no_builtin)) static void check_memset(u8 *dst, u64 len) {
    for (u64 i = 0; i < len; i++) {
        dst[i] = (u8)i;
    }

    void *out_dst = memset(dst, 69, len);
    assert(out_dst == dst);

    for (u64 i = 0; i < len; i++) {
        assert(dst[i] == 69);
    }
}

__attribute__((no_builtin)) static void check_memcmp(u8 *left, u8 *right,
                                                     u64 len) {
    // this should have a fuzz test
    // diff pos first, last, middle. put some diffs after the first diff too
    // (after 5 chars?). lt, gt, eq
    //

    for (u64 i = 0; i < len; i++) {
        left[i] = (u8)i;
        right[i] = (u8)i;
    }

    assert(0 == memcmp(left, right, len));

    // the rest of tests don't make sense for zero length;
    if (len == 0) {
        return;
    }

    u64 const indices[] = {0, len - 1, len / 2__wbu};
    u64 const n_indices = sizeof(indices) / sizeof(indices[0]);

    // lt
    for (u64 i = 0; i < n_indices; i++) {
        u64 const idx = indices[i];
        assert(idx < len);
        u8 const orig = left[idx];
        left[idx] = 69;
        right[idx] = 79;
        assert(0 > memcmp(left, right, len));
        left[idx] = orig;
        right[idx] = orig;
    }

    // gt
    for (u64 i = 0; i < n_indices; i++) {
        u64 const idx = indices[i];
        assert(idx < len);
        u8 const orig = left[idx];
        left[idx] = 79;
        right[idx] = 69;
        assert(0 < memcmp(left, right, len));
        left[idx] = orig;
        right[idx] = orig;
    }
}

__attribute__((no_builtin)) static void check_strlen(u8 *str, u64 len) {
    // We can't put a zero byte if the length is zero.
    if (len == 0) {
        return;
    }
    for (u64 i = 0; i < len; i++) {
        str[i] = 69;
    }
    if (len >= 2) {
        u64 const pos = len - 2;
        str[pos] = 0;
        assert(pos == strlen((char const *)str));
        str[pos] = 69;
    }

    u64 const pos = len - 1;
    str[pos] = 0;
    assert(pos == strlen((char const *)str));
}

__attribute__((no_builtin)) static u8 *make_alloc(Backtrace *err_bt,
                                                  u64 capacity) {
    // has to be power of two
    assert(__builtin_popcountg(capacity) == 1);
    assert(capacity >= 4096);

    u8 *alloc;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(err_bt, capacity * 2__wbu, &alloc),
        err_bt);

    sys::MProtectStatus const sys_status =
        sys::mprotect(err_bt, &alloc[capacity], capacity, sys::PROT_NONE);
    testing::assert_with_trace(sys_status == sys::MProtectStatus::Ok, err_bt);

    return alloc;
}

int main() {
    Backtrace err_bt = Backtrace::init();

    constexpr u64 capacity = 4096;

    u8 *const buf0 = make_alloc(&err_bt, capacity);
    u8 *const buf1 = make_alloc(&err_bt, capacity);

    u64 const lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                           65, 63, 64,   128,  127, 129};
    u64 const n_lengths = sizeof(lengths) / sizeof(lengths[0]);

    for (u64 i = 0; i < n_lengths; i++) {
        u64 const len = lengths[i];
        assert(capacity >= len);
        u64 const offset = capacity - len;
        u8 *b0 = &buf0[offset];
        u8 *b1 = &buf1[offset];
        check_memcpy(b0, b1, len);
        check_memset(b0, len);
        check_memcmp(b0, b1, len);
        check_strlen(b0, len);
    }

    return 0;
}
