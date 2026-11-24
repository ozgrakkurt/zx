module compression;
import zx;

namespace compression {

namespace noz {

CompressBoundStatus compress_bound(zx::Backtrace *err_bt, zx::u64 input_size,
                                   zx::u64 *bound) {
    if (bound == nullptr) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressBoundStatus::Nullptr;
    }

    bool overflow = false;

    zx::u64 const n_chunks = zx::div_ceil_u64(input_size, 64, &overflow);

    zx::u64 const header_size =
        zx::mul_u64(n_chunks, sizeof(zx::u64), &overflow);

    zx::u64 const data_size = zx::mul_u64(n_chunks, 64, &overflow);

    *bound = zx::add_u64(header_size, data_size, &overflow);

    if (overflow) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressBoundStatus::IntegerOverflow;
    }

    return CompressBoundStatus::Ok;
}

CompressStatus compress(zx::Backtrace *err_bt, zx::Slice<zx::u8> input,
                        zx::SliceMut<zx::u8> output, zx::u64 *n_written) {
    if (n_written == nullptr) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressStatus::Nullptr;
    }

    if (input.is_overlapping(output.as_const())) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressStatus::InputOutputOverlap;
    }

    zx::u64 bound = zx::U64_MAX;
    if (CompressBoundStatus::Ok != compress_bound(err_bt, input.len(), &bound))
        [[unlikely]] {
        return CompressStatus::BoundFail;
    }

    if (output.len() < bound) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressStatus::OutputTooShort;
    }

    if (input.len() == 0) {
        *n_written = 0;
        return CompressStatus::Ok;
    }

    zx::u8 const *const in = input.ptr();
    zx::u8 *const out = output.ptr();

    zx::u64 in_offset = 0;
    zx::u64_c out_offset = 0;
    for (; zx::add_unsafe_u64(in_offset, 256) <= input.len();
         in_offset = zx::add_unsafe_u64(in_offset, 256)) {
        __asm__ volatile(
            // read input
            "vmovdqu8 (%[input]), %%zmm31\n\t"
            "vmovdqu8 64(%[input]), %%zmm29\n\t"
            "vmovdqu8 128(%[input]), %%zmm27\n\t"
            "vmovdqu8 192(%[input]), %%zmm25\n\t"
            // check zeroes to create the masks
            "vptestmb %%zmm31, %%zmm31, %%k7\n\t"
            "vptestmb %%zmm29, %%zmm29, %%k6\n\t"
            "vptestmb %%zmm27, %%zmm27, %%k5\n\t"
            "vptestmb %%zmm25, %%zmm25, %%k4\n\t"
            // compress to other registers
            "vpcompressb %%zmm31, %%zmm30 %{%%k7%}\n\t"
            "vpcompressb %%zmm29, %%zmm28 %{%%k6%}\n\t"
            "vpcompressb %%zmm27, %%zmm26 %{%%k5%}\n\t"
            "vpcompressb %%zmm25, %%zmm24 %{%%k4%}\n\t"
            // write the first chunk
            "kmovq %%k7, %%r11\n\t"
            "movq %%r11, (%[output], %[out_offset])\n\t"
            "vmovdqu8 %%zmm30, 8(%[output], %[out_offset])\n\t"
            "popcntq %%r11, %%r11\n\t"
            "leaq 8(%[out_offset], %%r11), %[out_offset]\n\t"
            // write the second chunk
            "kmovq %%k6, %%r11\n\t"
            "movq %%r11, (%[output], %[out_offset])\n\t"
            "vmovdqu8 %%zmm28, 8(%[output], %[out_offset])\n\t"
            "popcntq %%r11, %%r11\n\t"
            "leaq 8(%[out_offset], %%r11), %[out_offset]\n\t"
            // write the third chunk
            "kmovq %%k5, %%r11\n\t"
            "movq %%r11, (%[output], %[out_offset])\n\t"
            "vmovdqu8 %%zmm26, 8(%[output], %[out_offset])\n\t"
            "popcntq %%r11, %%r11\n\t"
            "leaq 8(%[out_offset], %%r11), %[out_offset]\n\t"
            // write the fourth chunk
            "kmovq %%k4, %%r11\n\t"
            "movq %%r11, (%[output], %[out_offset])\n\t"
            "vmovdqu8 %%zmm24, 8(%[output], %[out_offset])\n\t"
            "popcntq %%r11, %%r11\n\t"
            "leaq 8(%[out_offset], %%r11), %[out_offset]\n\t"
            : [out_offset] "+&r"(out_offset)
            : [input] "r"(&in[in_offset]), [output] "r"(out)
            : "zmm31", "zmm29", "zmm27", "zmm25", "zmm30", "zmm28", "zmm26",
              "zmm24", "k7", "k6", "k5", "k4", "r11", "memory");
    }

    if (zx::sub_unsafe_u64(input.len(), in_offset) >= 128) {
        __asm__ volatile(
            // read input
            "vmovdqu8 (%[input]), %%zmm31\n\t"
            "vmovdqu8 64(%[input]), %%zmm29\n\t"
            // check zeroes to create the masks
            "vptestmb %%zmm31, %%zmm31, %%k7\n\t"
            "vptestmb %%zmm29, %%zmm29, %%k6\n\t"
            // move masks to gpr
            "kmovq %%k7, %%r11\n\t"
            "kmovq %%k6, %%r10\n\t"
            // compress to other registers
            "vpcompressb %%zmm31, %%zmm30 %{%%k7%}\n\t"
            "vpcompressb %%zmm29, %%zmm28 %{%%k6%}\n\t"
            // calculate the output offset of the second chunk
            "popcntq %%r11, %%r9\n\t"
            "addq %[out_offset], %%r9\n\t"
            // write the first control mask and data
            "movq %%r11, (%[output], %[out_offset])\n\t"
            "vmovdqu8 %%zmm30, 8(%[output], %[out_offset])\n\t"
            // write the second control mask and data
            "movq %%r10, 8(%[output], %%r9)\n\t"
            "vmovdqu8 %%zmm28, 16(%[output], %%r9)\n\t"
            // calculate the new output offset
            "popcntq %%r10, %%r10\n\t"
            "leaq 16(%%r9, %%r10), %[out_offset]\n\t"
            : [out_offset] "+&r"(out_offset)
            : [input] "r"(&in[in_offset]), [output] "r"(out)
            : "zmm31", "zmm29", "zmm30", "zmm28", "k7", "k6", "r9", "r10",
              "r11", "memory");
        in_offset = zx::add_unsafe_u64(in_offset, 128);
    }

    if (zx::sub_unsafe_u64(input.len(), in_offset) >= 64) {
        __asm__ volatile(
            // read input
            "vmovdqu8 (%[input]), %%zmm31\n\t"
            // check zeroes to create the mask
            "vptestmb %%zmm31, %%zmm31, %%k7\n\t"
            // move mask to gpr
            "kmovq %%k7, %%r11\n\t"
            // compress to another register
            "vpcompressb %%zmm31, %%zmm30 %{%%k7%}\n\t"
            // write data
            "vmovdqu8 %%zmm30, 8(%[output], %[out_offset])\n\t"
            // write the 8 byte control mask
            "movq %%r11, (%[output], %[out_offset])\n\t"
            // calculate num total bytes written
            "popcntq %%r11, %%r11\n\t"
            // update output offset
            "leaq 8(%[out_offset], %%r11), %[out_offset]\n\t"
            : [out_offset] "+&r"(out_offset)
            : [input] "r"(&in[in_offset]), [output] "r"(out)
            : "zmm31", "zmm30", "k7", "r11", "memory");
        in_offset = zx::add_unsafe_u64(in_offset, 64);
    }

    zx::u64 const remainder = zx::sub_unsafe_u64(input.len(), in_offset);
    if (remainder > 0) {
        zx::u64_c const loadmask =
            __builtin_ia32_bzhi_di(~(zx::u64){0}, remainder);
        __asm__ volatile("vmovdqu8 (%[input]), %%zmm31 %{%[loadmask]%}%{z%}\n\t"
                         "vptestmb %%zmm31, %%zmm31, %%k7\n\t"
                         "kmovq %%k7, %%r11\n\t"
                         "vpcompressb %%zmm31, %%zmm30 %{%%k7%}\n\t"
                         "vmovdqu8 %%zmm30, 8(%[output], %[out_offset])\n\t"
                         "movq %%r11, (%[output], %[out_offset])\n\t"
                         "popcntq %%r11, %%r11\n\t"
                         "leaq 8(%[out_offset], %%r11), %[out_offset]\n\t"
                         : [out_offset] "+&r"(out_offset)
                         : [input] "r"(&in[in_offset]), [output] "r"(out),
                           [loadmask] "Yk"(loadmask)
                         : "zmm31", "zmm30", "k7", "r10", "r11", "memory");
    }

    *n_written = out_offset;
    return CompressStatus::Ok;
}

DecompressStatus decompress(zx::Backtrace *err_bt, zx::Slice<zx::u8> input,
                            zx::SliceMut<zx::u8> output, zx::u64 *n_read) {
    if (n_read == nullptr) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return DecompressStatus::Nullptr;
    }

    if (input.is_overlapping(output.as_const())) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return DecompressStatus::InputOutputOverlap;
    }

    if (output.len() == 0) {
        *n_read = 0;
        return DecompressStatus::Ok;
    }

    zx::u64 in_offset = 0;
    zx::u64_c out_offset = 0;
    zx::u8 const *const in = input.ptr();
    zx::u8 *const out = output.ptr();
    for (; zx::add_unsafe_u64(out_offset, 64) <= output.len();
         out_offset = zx::add_unsafe_u64(out_offset, 64)) {
        zx::u64_c const control_end =
            zx::add_unsafe_u64(in_offset, sizeof(zx::u64));
        if (control_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        zx::u64_c const control =
            input.read_unaligned_unsafe<zx::u64_c>(in_offset);

        zx::u64_c const step_end =
            zx::add_unsafe_u64(control_end, zx::popcount_u64(control));
        if (step_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        __asm__ volatile(
            // move the control mask to a vector mask register
            "kmovq %[control], %%k5\n\t"
            // expand all 64 bytes
            "vpexpandb (%[input], %[control_end]), %%zmm31 %{%%k5%}%{z%}\n\t"
            // write all 64 bytes
            "vmovdqu8 %%zmm31, (%[output], %[out_offset])\n\t"
            :
            : [input] "r"(in), [output] "r"(out),
              [control_end] "r"(control_end), [out_offset] "r"(out_offset),
              [control] "r"(control)
            : "zmm31", "k5", "memory");

        in_offset = step_end;
    }

    zx::u64 const remainder = zx::sub_unsafe_u64(output.len(), out_offset);
    if (remainder > 0) {
        zx::u64_c const control_end =
            zx::add_unsafe_u64(in_offset, sizeof(zx::u64));
        if (control_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        zx::u64_c const control =
            input.read_unaligned_unsafe<zx::u64>(in_offset);

        zx::u64 const step_end =
            zx::add_unsafe_u64(control_end, zx::popcount_u64(control));
        if (step_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        zx::u64_c const storemask =
            __builtin_ia32_bzhi_di(zx::U64_MAX, remainder);

        __asm__ volatile(
            "vpexpandb (%[input], %[control_end]), %%zmm31 "
            "%{%[control]}%{z%}\n\t"
            "vmovdqu8 %%zmm31, (%[output], %[out_offset]) %{%[storemask]%}\n\t"
            :
            : [output] "r"(out), [input] "r"(in), [out_offset] "r"(out_offset),
              [control_end] "r"(control_end), [control] "Yk"(control),
              [storemask] "Yk"(storemask)
            : "zmm31", "memory");

        in_offset = step_end;
    }

    *n_read = in_offset;
    return DecompressStatus::Ok;
}

} // namespace noz

} // namespace compression
