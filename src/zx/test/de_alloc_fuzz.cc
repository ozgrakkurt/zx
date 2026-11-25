import zx;

using namespace zx;

struct Ctx {
    SliceMut<u8> mem;
};

constexpr u64 ALLOCS_CAP = 128;

struct AllocRecord {
    u64 start;
    u64 len;
    u8 pattern;
};

static void verify_pattern(SliceMut<u8> slice, u8 value) {
    for (u64 i = 0; i < slice.len(); i++) {
        assert(slice.read_unsafe(i) == value);
    }
}

static void verify_records(SliceMut<u8> mem, AllocRecord const *records,
                           u64 n_records) {
    for (u64 i = 0; i < n_records; i++) {
        SliceMut<u8> const slice =
            mem.slice_unsafe(records[i].start, records[i].len);
        verify_pattern(slice, records[i].pattern);
    }
}

// drops the records that are no longer entirely inside the left region
static u64 compact_left_records(AllocRecord *records, u64 n_records,
                                u64 new_left_offset) {
    u64 kept = 0;
    for (u64 i = 0; i < n_records; i++) {
        u64 const end = add_unsafe_u64(records[i].start, records[i].len);
        if (end <= new_left_offset) {
            records[kept] = records[i];
            kept = add_unsafe_u64(kept, 1);
        }
    }
    return kept;
}

// drops the records that are no longer entirely inside the right region
static u64 compact_right_records(AllocRecord *records, u64 n_records,
                                 u64 new_right_offset) {
    u64 kept = 0;
    for (u64 i = 0; i < n_records; i++) {
        if (records[i].start >= new_right_offset) {
            records[kept] = records[i];
            kept = add_unsafe_u64(kept, 1);
        }
    }
    return kept;
}

static void do_left_alloc(SliceMut<u8> mem, DEAlloc *alloc,
                          FuzzInput *fuzz_input, u64 op_idx,
                          AllocRecord *records, u64 *n_records) {
    u64 const len = rand_mulshift_mod_u64(
        fuzz_input->next(), add_unsafe_u64(alloc->capacity_left(), 1));
    u8 const align_kind =
        u64_to_u8_unsafe(rand_mulshift_mod_u64(fuzz_input->next(), 8));
    u16 const align = shl_unsafe_u16(1, align_kind);

    u64 const old_left = alloc->left_offset();
    u64 const old_right = alloc->right_offset();

    SliceMut<u8> slice = SliceMut<u8>::init_dangling();
    AllocStatus const status =
        alloc->left_aligned_alloc<u8>(nullptr, align, len, &slice);
    assert(alloc->right_offset() == old_right);

    if (status == AllocStatus::Ok) {
        assert(slice.len() == len);
        assert(__builtin_is_aligned(slice.ptr(), align));
        assert(alloc->left_offset() >= old_left);
        assert(alloc->left_offset() <= old_right);

        if (len != 0) {
            u64 const start = sub_unsafe_u64((u64)slice.ptr(), (u64)mem.ptr());
            assert(start >= old_left);
            assert(add_unsafe_u64(start, len) == alloc->left_offset());

            assert(*n_records < ALLOCS_CAP);
            u8 const pattern = u64_to_u8_unsafe(add_unsafe_u64(op_idx, 1));
            records[*n_records] = AllocRecord{
                .start = start,
                .len = len,
                .pattern = pattern,
            };
            *n_records = add_unsafe_u64(*n_records, 1);
            slice.memset(pattern);
        }
    } else {
        assert(status == AllocStatus::OutOfMemory);
        assert(alloc->left_offset() == old_left);
        assert(alloc->right_offset() == old_right);
        assert(slice.ptr() == SliceMut<u8>::init_dangling().ptr());
        assert(slice.len() == 0);
    }
}

static void do_right_alloc(SliceMut<u8> mem, DEAlloc *alloc,
                           FuzzInput *fuzz_input, u64 op_idx,
                           AllocRecord *records, u64 *n_records) {
    u64 const len = rand_mulshift_mod_u64(
        fuzz_input->next(), add_unsafe_u64(alloc->capacity_left(), 1));
    u8 const align_kind =
        u64_to_u8_unsafe(rand_mulshift_mod_u64(fuzz_input->next(), 8));
    u16 const align = shl_unsafe_u16(1, align_kind);

    u64 const old_left = alloc->left_offset();
    u64 const old_right = alloc->right_offset();

    SliceMut<u8> slice = SliceMut<u8>::init_dangling();
    AllocStatus const status =
        alloc->right_aligned_alloc<u8>(nullptr, align, len, &slice);
    assert(alloc->left_offset() == old_left);

    if (status == AllocStatus::Ok) {
        assert(slice.len() == len);
        assert(__builtin_is_aligned(slice.ptr(), align));
        assert(alloc->right_offset() <= old_right);
        assert(alloc->right_offset() >= alloc->left_offset());

        if (len != 0) {
            u64 const start = sub_unsafe_u64((u64)slice.ptr(), (u64)mem.ptr());
            assert(start == alloc->right_offset());
            assert(add_unsafe_u64(start, len) <= old_right);

            assert(*n_records < ALLOCS_CAP);
            u8 const pattern = u64_to_u8_unsafe(add_unsafe_u64(op_idx, 1));
            records[*n_records] = AllocRecord{
                .start = start,
                .len = len,
                .pattern = pattern,
            };
            *n_records = add_unsafe_u64(*n_records, 1);
            slice.memset(pattern);
        }
    } else {
        assert(status == AllocStatus::OutOfMemory);
        assert(alloc->left_offset() == old_left);
        assert(alloc->right_offset() == old_right);
        assert(slice.ptr() == SliceMut<u8>::init_dangling().ptr());
        assert(slice.len() == 0);
    }
}

static void do_left_resize(DEAlloc *alloc, FuzzInput *fuzz_input,
                           AllocRecord *records, u64 *n_records) {
    u64 const old_right = alloc->right_offset();
    u64 const new_offset =
        rand_mulshift_mod_u64(fuzz_input->next(), add_unsafe_u64(old_right, 1));

    assert(AllocResizeStatus::Ok == alloc->left_resize(nullptr, new_offset));
    assert(alloc->left_offset() == new_offset);
    assert(alloc->right_offset() == old_right);

    *n_records = compact_left_records(records, *n_records, new_offset);
}

static void do_right_resize(SliceMut<u8> mem, DEAlloc *alloc,
                            FuzzInput *fuzz_input, AllocRecord *records,
                            u64 *n_records) {
    u64 const old_left = alloc->left_offset();
    u64 const span = sub_unsafe_u64(mem.len(), old_left);
    u64 const new_offset = add_unsafe_u64(
        old_left,
        rand_mulshift_mod_u64(fuzz_input->next(), add_unsafe_u64(span, 1)));

    assert(new_offset >= old_left);
    assert(new_offset <= mem.len());
    assert(AllocResizeStatus::Ok == alloc->right_resize(nullptr, new_offset));
    assert(alloc->left_offset() == old_left);
    assert(alloc->right_offset() == new_offset);

    *n_records = compact_right_records(records, *n_records, new_offset);
}

static void do_reset(SliceMut<u8> mem, DEAlloc *alloc, FuzzInput *fuzz_input,
                     u64 *n_left, u64 *n_right) {
    u64 const old_left = alloc->left_offset();
    u64 const old_right = alloc->right_offset();
    u64 const kind = rand_mulshift_mod_u64(fuzz_input->next(), 3);

    if (kind == 0) {
        alloc->left_reset();
        assert(alloc->left_offset() == 0);
        assert(alloc->right_offset() == old_right);
        *n_left = 0;
    } else if (kind == 1) {
        alloc->right_reset();
        assert(alloc->left_offset() == old_left);
        assert(alloc->right_offset() == mem.len());
        *n_right = 0;
    } else {
        alloc->full_reset();
        assert(alloc->left_offset() == 0);
        assert(alloc->right_offset() == mem.len());
        *n_left = 0;
        *n_right = 0;
    }
}

static void assert_no_overlap_same(SliceMut<u8> mem, AllocRecord const *records,
                                   u64 n_records) {
    for (u64 i = 0; i < n_records; i++) {
        SliceMut<u8> const a =
            mem.slice_unsafe(records[i].start, records[i].len);
        for (u64 j = add_unsafe_u64(i, 1); j < n_records; j++) {
            SliceMut<u8> const b =
                mem.slice_unsafe(records[j].start, records[j].len);
            assert(!a.as_const().is_overlapping(b.as_const()));
        }
    }
}

static void assert_no_overlap_cross(SliceMut<u8> mem, AllocRecord const *a,
                                    u64 n_a, AllocRecord const *b, u64 n_b) {
    for (u64 i = 0; i < n_a; i++) {
        SliceMut<u8> const sa = mem.slice_unsafe(a[i].start, a[i].len);
        for (u64 j = 0; j < n_b; j++) {
            SliceMut<u8> const sb = mem.slice_unsafe(b[j].start, b[j].len);
            assert(!sa.as_const().is_overlapping(sb.as_const()));
        }
    }
}

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    assert(fuzz_input != nullptr);

    Ctx const *ctx = (Ctx const *)usr_ctx;

    // use a shifted base so alignment padding is exercised with a base that is
    // not aligned to the requested alignments. the slice still ends at the
    // guard page so out of bounds writes to the right are caught.
    u64 const base_offset = rand_mulshift_mod_u64(fuzz_input->next(), 128);
    assert(base_offset < ctx->mem.len());
    SliceMut<u8> const mem = ctx->mem.slice_unsafe(
        base_offset, sub_unsafe_u64(ctx->mem.len(), base_offset));

    DEAlloc alloc = DEAlloc::init(mem);

    AllocRecord left_records[ALLOCS_CAP];
    AllocRecord right_records[ALLOCS_CAP];
    __builtin_memset(left_records, 0, sizeof(left_records));
    __builtin_memset(right_records, 0, sizeof(right_records));

    u64 n_left = 0;
    u64 n_right = 0;

    u64 const n_ops = rand_mulshift_mod_u64(fuzz_input->next(),
                                            add_unsafe_u64(ALLOCS_CAP, 1));
    for (u64 i = 0; i < n_ops; i++) {
        u64 const op = rand_mulshift_mod_u64(fuzz_input->next(), 10);

        if (op <= 3) {
            do_left_alloc(mem, &alloc, fuzz_input, i, left_records, &n_left);
        } else if (op <= 6) {
            do_right_alloc(mem, &alloc, fuzz_input, i, right_records, &n_right);
        } else if (op == 7) {
            do_left_resize(&alloc, fuzz_input, left_records, &n_left);
        } else if (op == 8) {
            do_right_resize(mem, &alloc, fuzz_input, right_records, &n_right);
        } else {
            do_reset(mem, &alloc, fuzz_input, &n_left, &n_right);
        }

        assert(alloc.left_offset() <= alloc.right_offset());
        assert(alloc.right_offset() <= mem.len());
        assert(alloc.capacity_left() ==
               sub_unsafe_u64(alloc.right_offset(), alloc.left_offset()));

        verify_records(mem, left_records, n_left);
        verify_records(mem, right_records, n_right);
    }

    verify_records(mem, left_records, n_left);
    verify_records(mem, right_records, n_right);

    assert_no_overlap_same(mem, left_records, n_left);
    assert_no_overlap_same(mem, right_records, n_right);
    assert_no_overlap_cross(mem, left_records, n_left, right_records, n_right);

    // resetting must free everything on both ends
    alloc.full_reset();
    assert(alloc.left_offset() == 0);
    assert(alloc.right_offset() == mem.len());
    assert(alloc.capacity_left() == mem.len());
}

Alloc make_alloc(Backtrace *err_bt, u64 size) {
    u8 *mem = nullptr;
    testing::assert_with_trace(page_alloc::AllocStatus::Ok ==
                                   page_alloc::alloc_4kb(err_bt, size, &mem),
                               err_bt);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, size));

    return alloc;
}

static SliceMut<u8> make_guarded_alloc(Backtrace *err_bt, u64 capacity) {
    assert(is_pow_two_u64(capacity));
    assert(capacity >= 4096);

    u64 const mem_size = mul_unsafe_u64(capacity, 2);
    u8 *mem = nullptr;
    page_alloc::AllocStatus const alloc_status =
        page_alloc::alloc_4kb(err_bt, mem_size, &mem);
    testing::assert_with_trace(alloc_status == page_alloc::AllocStatus::Ok,
                               err_bt);

    sys::MProtectStatus sys_status =
        sys::mprotect(err_bt, &mem[capacity], capacity, sys::PROT_NONE);
    testing::assert_with_trace(sys_status == sys::MProtectStatus::Ok, err_bt);

    SliceMut<u8> ret = SliceMut<u8>::init_unsafe(mem, capacity);
    ret.memset(0);

    return ret;
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    Alloc alloc = make_alloc(&err_bt, FUZZ_MEM_CAP);
    Ctx const ctx = Ctx{
        .mem = make_guarded_alloc(&err_bt, 8192),
    };
    fuzz(FuzzParams{
        .alloc = &alloc,
        .usr_ctx = (void *)&ctx,
        .fuzz_one = fuzz_one,
    });
    return 0;
}
