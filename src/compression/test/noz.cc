import compression;
import zx;

namespace noz = compression::noz;

static void do_test(zx::Backtrace *err_bt, zx::SliceMut<zx::u8> input,
                    zx::SliceMut<zx::u8> output,
                    zx::SliceMut<zx::u8> compressed,
                    zx::SliceMut<zx::u8> compressed_copy) {
    zx::assert(input.len() == output.len());

    zx::u64 bound = zx::U64_MAX;
    zx::testing::assert_with_trace(
        noz::CompressBoundStatus::Ok ==
            noz::compress_bound(err_bt, input.len(), &bound),
        err_bt);

    zx::assert(bound == compressed.len());

    compressed.memset(0xaa);
    zx::u64 n_written = zx::U64_MAX;
    zx::testing::assert_with_trace(
        noz::CompressStatus::Ok ==
            noz::compress(err_bt, input.as_const(), compressed, &n_written),
        err_bt);
    zx::assert(n_written <= bound);

    zx::SliceMut<zx::u8> copy = compressed_copy.slice_unsafe(
        zx::sub_unsafe_u64(compressed_copy.len(), n_written), n_written);
    copy.memcpy_from_unsafe(compressed.as_const().slice_unsafe(0, n_written));

    zx::u64 n_read = zx::U64_MAX;
    zx::testing::assert_with_trace(
        noz::DecompressStatus::Ok ==
            noz::decompress(err_bt, copy.as_const(), output, &n_read),
        err_bt);
    zx::assert(n_read == n_written);
    zx::assert(n_read == copy.len());
    zx::assert(input.as_const().memcmp_unsafe(output.as_const()) == 0);
}

static zx::SliceMut<zx::u8> make_alloc(zx::Backtrace *err_bt,
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

static void test_compress_bound_errors(void) {
    zx::u64 bound = zx::U64_MAX;

    zx::assert(noz::CompressBoundStatus::Nullptr ==
               noz::compress_bound(nullptr, 0, nullptr));

    zx::assert(noz::CompressBoundStatus::IntegerOverflow ==
               noz::compress_bound(nullptr, zx::U64_MAX, &bound));
}

static void test_compress_errors(void) {
    zx::u64 n_written = zx::U64_MAX;

    zx::u8 in_buf[16] = {0};
    zx::u8 out_buf[16] = {0};
    zx::Slice<zx::u8> in =
        zx::Slice<zx::u8>::init_unsafe(in_buf, sizeof(in_buf));
    zx::SliceMut<zx::u8> out =
        zx::SliceMut<zx::u8>::init_unsafe(out_buf, sizeof(out_buf));

    // Nullptr
    zx::assert(noz::CompressStatus::Nullptr ==
               noz::compress(nullptr, in, out, nullptr));

    // InputOutputOverlap
    zx::u8 shared[16] = {0};
    zx::Slice<zx::u8> shared_in =
        zx::Slice<zx::u8>::init_unsafe(shared, sizeof(shared));
    zx::SliceMut<zx::u8> shared_out =
        zx::SliceMut<zx::u8>::init_unsafe(shared, sizeof(shared));
    zx::assert(noz::CompressStatus::InputOutputOverlap ==
               noz::compress(nullptr, shared_in, shared_out, &n_written));

    // OutputTooShort: bound for a single byte is 72.
    zx::u8 one_in_buf[1] = {0};
    zx::u8 one_out_buf[1] = {0};
    zx::Slice<zx::u8> one_in =
        zx::Slice<zx::u8>::init_unsafe(one_in_buf, sizeof(one_in_buf));
    zx::SliceMut<zx::u8> one_out =
        zx::SliceMut<zx::u8>::init_unsafe(one_out_buf, sizeof(one_out_buf));
    zx::assert(noz::CompressStatus::OutputTooShort ==
               noz::compress(nullptr, one_in, one_out, &n_written));
}

static void test_decompress_errors(void) {
    zx::u64 n_read = zx::U64_MAX;

    zx::u8 in_buf[16] = {0};
    zx::u8 out_buf[16] = {0};
    zx::Slice<zx::u8> in =
        zx::Slice<zx::u8>::init_unsafe(in_buf, sizeof(in_buf));
    zx::SliceMut<zx::u8> out =
        zx::SliceMut<zx::u8>::init_unsafe(out_buf, sizeof(out_buf));

    // Nullptr
    zx::assert(noz::DecompressStatus::Nullptr ==
               noz::decompress(nullptr, in, out, nullptr));

    // InputOutputOverlap
    zx::u8 shared[16] = {0};
    zx::Slice<zx::u8> shared_in =
        zx::Slice<zx::u8>::init_unsafe(shared, sizeof(shared));
    zx::SliceMut<zx::u8> shared_out =
        zx::SliceMut<zx::u8>::init_unsafe(shared, sizeof(shared));
    zx::assert(noz::DecompressStatus::InputOutputOverlap ==
               noz::decompress(nullptr, shared_in, shared_out, &n_read));

    // InputTooShort: non-empty output with no input bytes at all.
    zx::Slice<zx::u8> empty = zx::Slice<zx::u8>::init_dangling();
    zx::u8 one_out_buf[1] = {0};
    zx::SliceMut<zx::u8> one_out =
        zx::SliceMut<zx::u8>::init_unsafe(one_out_buf, sizeof(one_out_buf));
    zx::assert(noz::DecompressStatus::InputTooShort ==
               noz::decompress(nullptr, empty, one_out, &n_read));
}

int main(void) {
    zx::Backtrace err_bt = zx::Backtrace::init();

    constexpr zx::u64 capacity = 4096;

    zx::SliceMut<zx::u8> const buf0 = make_alloc(&err_bt, capacity);
    zx::SliceMut<zx::u8> const buf1 = make_alloc(&err_bt, capacity);
    zx::SliceMut<zx::u8> const buf2 = make_alloc(&err_bt, capacity);
    zx::SliceMut<zx::u8> const buf3 = make_alloc(&err_bt, capacity);

    // Include the boundaries of every compression loop: the one-chunk tail,
    // the 64/128/256 byte loops and the 256 byte unrolled loop.
    constexpr zx::u64 lengths[] = {
        0,    1,    13,   63,   64,   65,   100,  127,  128,  129,
        191,  192,  193,  255,  256,  257,  383,  384,  385,  511,
        512,  513,  767,  768,  769,  1000, 1023, 1024, 1025, 1535,
        1536, 1537, 2047, 2048, 2049, 3071, 3072, 3073, 3583, 3584,
    };
    constexpr zx::u64 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

    constexpr zx::u64 aligns[] = {0, 1, 7, 31, 63};
    constexpr zx::u64 n_aligns = sizeof(aligns) / sizeof(aligns[0]);

    zx::Xoshiro256 rand = zx::Xoshiro256::init(69);

    for (zx::u64 i = 0; i < n_lengths; i++) {
        zx::u64 const len = lengths[i];

        zx::u64 bound = zx::U64_MAX;
        zx::testing::assert_with_trace(
            noz::CompressBoundStatus::Ok ==
                noz::compress_bound(&err_bt, len, &bound),
            &err_bt);

        for (zx::u64 a = 0; a < n_aligns; a++) {
            zx::u64 const align = aligns[a];

            zx::u64 const offset =
                zx::sub_unsafe_u64(capacity, zx::add_unsafe_u64(len, align));
            zx::SliceMut<zx::u8> b0 = buf0.slice_unsafe(offset, len);
            zx::SliceMut<zx::u8> b1 = buf1.slice_unsafe(offset, len);

            zx::u64 const compr_offset =
                zx::sub_unsafe_u64(capacity, zx::add_unsafe_u64(bound, align));
            zx::SliceMut<zx::u8> b2 = buf2.slice_unsafe(compr_offset, bound);
            zx::SliceMut<zx::u8> b3 = buf3.slice_unsafe(compr_offset, bound);

            b0.memset(0);
            do_test(&err_bt, b0, b1, b2, b3);
            b0.memset(1);
            do_test(&err_bt, b0, b1, b2, b3);
            rand.fill(b0);
            do_test(&err_bt, b0, b1, b2, b3);
        }
    }

    test_compress_bound_errors();
    test_compress_errors();
    test_decompress_errors();

    return 0;
}
