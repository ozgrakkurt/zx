import zx;
import compression;

namespace noz = compression::noz;

struct Ctx {
    zx::SliceMut<zx::u8> output;
    zx::SliceMut<zx::u8> compressed;
};

static void fuzz_one(void *usr_ctx, zx::FuzzInput *fuzz_input) {
    zx::assert(usr_ctx != nullptr);
    zx::assert(fuzz_input != nullptr);

    Ctx const *ctx = (Ctx const *)usr_ctx;

    zx::u64 const compressed_len =
        zx::rand_mod_u64(fuzz_input->next(), ctx->compressed.len());
    zx::u64 const compressed_offset =
        zx::sub_unsafe_u64(ctx->compressed.len(), compressed_len);

    zx::u64 const output_len =
        zx::rand_mod_u64(fuzz_input->next(), ctx->output.len());
    zx::u64 const output_offset =
        zx::sub_unsafe_u64(ctx->output.len(), output_len);

    zx::SliceMut<zx::u8> const output =
        ctx->output.slice_unsafe(output_offset, output_len);
    zx::SliceMut<zx::u8> const compressed =
        ctx->compressed.slice_unsafe(compressed_offset, compressed_len);

    zx::Xoshiro256 rand = zx::Xoshiro256::init(fuzz_input->next());

    zx::u64 offset = 0;
    for (zx::u64 n_iter = 0; offset < compressed.len() && n_iter < 100;
         n_iter++) {
        zx::u64 const diff = zx::sub_unsafe_u64(compressed.len(), offset);
        zx::u64 const len = zx::rand_mod_u64(fuzz_input->next(), diff);
        zx::SliceMut<zx::u8> const slice = compressed.slice_unsafe(offset, len);

        zx::u64 const data_kind = fuzz_input->next() % 3;
        switch (data_kind) {
        case 0: {
            rand.fill(slice);
            break;
        }
        case 1: {
            slice.memset(0);
            break;
        }
        case 2: {
            slice.memset(0xaa);
            break;
        }
        default: {
            zx::panic();
            break;
        }
        }

        offset = zx::add_unsafe_u64(offset, len);
    }
    rand.fill(compressed.slice_at_unsafe(offset));

    zx::u64 n_read = zx::U64_MAX;
    (void)noz::decompress(nullptr, compressed.as_const(), output, &n_read);
}

zx::Alloc make_alloc(zx::Backtrace *err_bt, zx::u64 size) {
    zx::u8 *mem = nullptr;
    zx::testing::assert_with_trace(
        zx::page_alloc::AllocStatus::Ok ==
            zx::page_alloc::alloc_4kb(err_bt, size, &mem),
        err_bt);

    zx::Alloc alloc =
        zx::Alloc::init(zx::SliceMut<zx::u8>::init_unsafe(mem, size));

    return alloc;
}

static zx::SliceMut<zx::u8> make_guarded_alloc(zx::Backtrace *err_bt,
                                               zx::u64 capacity) {
    zx::assert(zx::is_pow_two_u64(capacity));
    zx::assert(capacity >= 4096);

    zx::u64 const mem_size = zx::mul_unsafe_u64(capacity, 2);
    zx::u8 *mem = nullptr;
    zx::page_alloc::AllocStatus const alloc_status =
        zx::page_alloc::alloc_4kb(err_bt, mem_size, &mem);
    zx::testing::assert_with_trace(
        alloc_status == zx::page_alloc::AllocStatus::Ok, err_bt);

    zx::sys::MProtectStatus sys_status =
        zx::sys::mprotect(err_bt, &mem[capacity], capacity, zx::sys::PROT_NONE);
    zx::testing::assert_with_trace(sys_status == zx::sys::MProtectStatus::Ok,
                                   err_bt);

    zx::SliceMut<zx::u8> ret = zx::SliceMut<zx::u8>::init_unsafe(mem, capacity);
    ret.memset(0);

    return ret;
}

int main(void) {
    zx::Backtrace err_bt = zx::Backtrace::init();

    zx::Alloc alloc = make_alloc(&err_bt, zx::FUZZ_MEM_CAP);

    Ctx const ctx = Ctx{
        .output = make_guarded_alloc(&err_bt, 4096),
        .compressed = make_guarded_alloc(&err_bt, 8192),
    };

    zx::fuzz(zx::FuzzParams{
        .alloc = &alloc,
        .usr_ctx = (void *)&ctx,
        .fuzz_one = fuzz_one,
    });

    return 0;
}
