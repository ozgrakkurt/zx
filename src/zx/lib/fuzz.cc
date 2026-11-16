module zx;

namespace zx {

// config {
static constexpr u64 FUZZ_INPUT_CAP = u64{1} << 12;
static constexpr u64 FUZZ_SIGNATURE_DEDUP_CAP = 4096;
static constexpr u64 FUZZ_MAGIC[] = {0, U64_MAX};
static constexpr u64 FUZZ_N_MAGIC = sizeof(FUZZ_MAGIC) / sizeof(FUZZ_MAGIC[0]);
// } config

static u32 *FUZZ_SAN_COV_ = nullptr;
static u64 FUZZ_SAN_COV_LEN_ = 0;

extern "C" void __sanitizer_cov_trace_pc_guard_init(u32_c *start, u32_c *stop) {
    assert(start != nullptr);
    assert(stop != nullptr);

    static_assert(sizeof(void *) == sizeof(u64));

    u64 const start_addr = (u64)start;
    u64 const stop_addr = (u64)stop;

    assert(stop_addr >= start_addr);

    u64 const n_bytes = sub_unsafe_u64(stop_addr, start_addr);

    assert(n_bytes % sizeof(u32) == 0);
    u64 const n_slots = div_unsafe_u64(n_bytes, sizeof(u32));

    assert(FUZZ_SAN_COV_ == nullptr);
    assert(FUZZ_SAN_COV_LEN_ == 0);

    FUZZ_SAN_COV_ = (u32 *)start;
    FUZZ_SAN_COV_LEN_ = n_slots;
}

extern "C" __attribute__((no_builtin, no_sanitize("coverage"))) void
__sanitizer_cov_trace_pc_guard(u32_c *guard_variable) {
    *guard_variable = (*guard_variable + 1__wbu) | u32{1};
}

__attribute__((no_builtin, no_sanitize("coverage"))) static void
fuzz_reset_coverage() {
    u32 *cov = FUZZ_SAN_COV_;
    u64 len = FUZZ_SAN_COV_LEN_;
    for (u64 i = 0; i < len; i++) {
        cov[i] = 0;
    }
}

__attribute__((no_builtin, no_sanitize("coverage"))) static void
fuzz_load_coverage(u32 *dst, u64 dst_len) {
    u32 const *src = FUZZ_SAN_COV_;
    u64 src_len = FUZZ_SAN_COV_LEN_;
    assert(src_len == dst_len);
    for (u64 i = 0; i < dst_len; i++) {
        dst[i] = src[i];
    }
}

constexpr u32 FUZZ_SLOT_SET_CAP_ = 64;
struct FuzzSlotSet {
    u64 values[FUZZ_SLOT_SET_CAP_];
    u32 push_pos;
    u32 n_filled;
};

static void fuzz_slot_set_push(FuzzSlotSet *set, u64 value) {
    assert(set != nullptr);
    constexpr u64 val_cap = sizeof(set->values) / sizeof(set->values[0]);
    static_assert(val_cap > 0);
    u32 const idx = mod_unsafe_u32(set->push_pos, val_cap);
    set->values[idx] = value;
    set->push_pos = wrapping_add_u32(set->push_pos, 1);
    set->n_filled = min_u32(wrapping_add_u32(set->n_filled, 1), val_cap);
}

struct FuzzCtx {
    SliceMut<FuzzSlotSet> slot_sets;
    SliceMut<u32> signature_dedup;
    SliceMut<u64> fuzz_input;
    SliceMut<u32> coverage;
    SliceMut<bool> total_coverage;
};

static FuzzCtx fuzz_ctx_init(Backtrace *err_bt, Alloc *alloc) {
    SliceMut<u32> coverage;
    testing::assert_with_trace(
        AllocStatus::Ok ==
            alloc->alloc<u32>(err_bt, FUZZ_SAN_COV_LEN_, &coverage),
        err_bt);
    coverage.memset(0);

    SliceMut<u64> fuzz_input;
    testing::assert_with_trace(
        AllocStatus::Ok ==
            alloc->alloc<u64>(err_bt, FUZZ_INPUT_CAP, &fuzz_input),
        err_bt);
    fuzz_input.memset(0);

    SliceMut<u32> signature_dedup;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->alloc<u32>(err_bt, FUZZ_SIGNATURE_DEDUP_CAP,
                                             &signature_dedup),
        err_bt);
    signature_dedup.memset(0);

    SliceMut<FuzzSlotSet> slot_sets;
    testing::assert_with_trace(
        AllocStatus::Ok ==
            alloc->alloc<FuzzSlotSet>(err_bt, FUZZ_INPUT_CAP, &slot_sets),
        err_bt);
    slot_sets.memset(0);

    SliceMut<bool> total_coverage;
    testing::assert_with_trace(
        AllocStatus::Ok ==
            alloc->alloc<bool>(err_bt, FUZZ_SAN_COV_LEN_, &total_coverage),
        err_bt);
    total_coverage.memset(0);

    return FuzzCtx{
        .slot_sets = slot_sets,
        .signature_dedup = signature_dedup,
        .fuzz_input = fuzz_input,
        .coverage = coverage,
        .total_coverage = total_coverage,
    };
}

static u32 fuzz_calculate_signature(Slice<u32> coverage) {
    return (u32)rapidhash_v3::hash(coverage.cast_unsafe<u8>(), 0);
}

static u64 fuzz_calculate_score(Slice<u32> coverage) {
    u64 score = 0;

    for (u64 i = 0; i < coverage.len(); i++) {
        if (coverage.read_unsafe(i) > 0) {
            score = saturating_add_u64(score, 1);
        }
    }

    return score;
}

static bool fuzz_update_total_coverage(SliceMut<bool> total_coverage,
                                       Slice<u32> coverage) {
    assert(total_coverage.len() == coverage.len());

    bool new_coverage = false;
    for (u64 i = 0; i < total_coverage.len(); i++) {
        if (coverage.read_unsafe(i) > 0) {
            total_coverage.write_unsafe(i, true);
            new_coverage = true;
        }
    }

    return new_coverage;
}

struct FuzzResult {
    u64 score;
    u64 consumed_count;
    bool improvement;
};

static bool fuzz_push_signature(SliceMut<u32> signature_dedup, u32 signature) {
    if (signature_dedup.len() == 0) {
        panic();
    }

    u64 const idx = rand_mod_u64(signature, signature_dedup.len());
    u32 const old_sig = signature_dedup.read_unsafe(idx);
    bool const is_new = old_sig != signature;
    signature_dedup.write_unsafe(idx, signature);
    return is_new;
}

static FuzzResult fuzz_wrapper(FuzzCtx *ctx, u64 base_score, u64 mut_idx,
                               void *usr_ctx, fuzz_one_fn fuzz_one,
                               u64 *iter_no) {
    FuzzInput fuzz_input = FuzzInput::init_(ctx->fuzz_input.as_const());
    u32 *coverage = ctx->coverage.ptr();
    u64 coverage_len = ctx->coverage.len();
    // don't break these up
    fuzz_reset_coverage();
    fuzz_one(usr_ctx, &fuzz_input);
    fuzz_load_coverage(coverage, coverage_len);
    // don't break these up

    u64 const consumed_count = fuzz_input.consumed_count_();

    Slice<u32> const cov = ctx->coverage.as_const();
    bool const new_coverage =
        fuzz_update_total_coverage(ctx->total_coverage, cov);
    u32 const signature = fuzz_calculate_signature(cov);
    u64 const score = fuzz_calculate_score(cov);
    bool const sig_is_new =
        fuzz_push_signature(ctx->signature_dedup, signature);

    bool const score_improvement = score > base_score;
    bool const improvement = sig_is_new && (new_coverage || score_improvement);

    if (improvement) {
        fuzz_slot_set_push(ctx->slot_sets.offset_mut_unsafe(mut_idx),
                           ctx->fuzz_input.read_unsafe(mut_idx));
    }

    assert(iter_no != nullptr);
    *iter_no = add_unsafe_u64(*iter_no, 1);

    return FuzzResult{
        .score = score,
        .improvement = improvement,
        .consumed_count = consumed_count,
    };
}

void fuzz(FuzzParams params) {
    Backtrace err_bt = Backtrace::init();

    Xoshiro256 rand = Xoshiro256::init(0);

    FuzzCtx ctx = fuzz_ctx_init(&err_bt, params.alloc);

    u64 mut_idx = 0;

#ifdef ZX_FUZZ
    // do the real number of iterations if fuzzing is explicitly
    // enabled.
    u64 const iter_limit = params.iter_limit;
#else
    // just do some iterations starting from random input if
    // fuzzing isn't explicitly enabled.
    rand.fill(ctx.fuzz_input.cast_static<u8>());
    u64 const iter_limit = 10;
#endif

    for (u64 iter_no = 0; iter_no < iter_limit;) {
        assert(mut_idx < ctx.fuzz_input.len());

        // do a baseline run
        FuzzResult const baseline_result = fuzz_wrapper(
            &ctx, U64_MAX, mut_idx, params.usr_ctx, params.fuzz_one, &iter_no);
        // select a random position to mutate
        mut_idx = rand_mod_u64(rand.next(), baseline_result.consumed_count);

        // try magic values
        for (u64 i = 0; i < FUZZ_N_MAGIC; i++) {
            ctx.fuzz_input.write_unsafe(mut_idx, FUZZ_MAGIC[i]);
            fuzz_wrapper(&ctx, baseline_result.score, mut_idx, params.usr_ctx,
                         params.fuzz_one, &iter_no);
        }

        // try previous interesting values for this mut_idx
        FuzzSlotSet const *slot_set = ctx.slot_sets.offset_unsafe(mut_idx);
        for (u32 i = 0; i < slot_set->n_filled; i++) {
            ctx.fuzz_input.write_unsafe(mut_idx, slot_set->values[i]);
            fuzz_wrapper(&ctx, baseline_result.score, mut_idx, params.usr_ctx,
                         params.fuzz_one, &iter_no);
        }

        // try some random values in this position
        // move on if we can't improve coverage for some number of
        // iterations
        // do more iterations for interesting mut_idx
        u32 const max_stall = saturating_add_u32(slot_set->n_filled, 8);
        u32 stall = 0;
        while (stall < max_stall) {
            u64 const new_val = rand.next();
            ctx.fuzz_input.write_unsafe(mut_idx, new_val);

            FuzzResult const result =
                fuzz_wrapper(&ctx, baseline_result.score, mut_idx,
                             params.usr_ctx, params.fuzz_one, &iter_no);
            if (result.improvement) {
                stall = 0;
            } else {
                stall = saturating_add_u32(stall, 1);
            }
        }

        if (slot_set->n_filled > 0) {
            u32 const val_idx =
                rand_mod_u32((u32)rand.next(), slot_set->n_filled);
            ctx.fuzz_input.write_unsafe(mut_idx, slot_set->values[val_idx]);
        } else {
            ctx.fuzz_input.write_unsafe(mut_idx, rand.next());
        }
    }
}

} // namespace zx
