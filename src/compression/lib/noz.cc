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

    zx::u64 in_offset = 0;
    zx::u64_c out_offset = 0;
    zx::u8 const *const in = input.ptr();
    zx::u8 *const out = output.ptr();
    for (; zx::add_unsafe_u64(in_offset, 64) <= input.len();
         in_offset = zx::add_unsafe_u64(in_offset, 64)) {
        __asm__ volatile(
            // read input
            "vmovdqu8 (%[input]), %%ymm31\n\t"
            "vmovdqu8 32(%[input]), %%ymm30\n\t"
            // check zeroes to create the masks
            "vptestmb %%ymm31, %%ymm31, %%k7\n\t"
            "vptestmb %%ymm30, %%ymm30, %%k6\n\t"
            // move masks to gpr
            "kmovq %%k7, %%r11\n\t"
            "kmovq %%k6, %%r10\n\t"
            // compress to other registers
            "vpcompressb %%ymm31, %%ymm29 %{%%k7%}\n\t"
            "vpcompressb %%ymm30, %%ymm28 %{%%k6%}\n\t"
            // calculate offset for second write
            "popcntq %%r11, %%r9\n\t"
            "addq %[out_offset], %%r9\n\t"
            // write data
            "vmovdqu8 %%ymm29, 8(%[output], %[out_offset])\n\t"
            "vmovdqu8 %%ymm28, 8(%[output], %%r9)\n\t"
            // combine two control masks into one
            "movq %%r10, %%r9\n\t"
            "shlq $32, %%r9\n\t"
            "orq %%r11, %%r9\n\t"
            // write the combined 8 byte control mask
            "movq %%r9, (%[output], %[out_offset])\n\t"
            // calculate num total bytes written
            "popcntq %%r9, %%r9\n\t"
            // update output offset
            "addq %%r9, %[out_offset]\n\t"
            "addq $8, %[out_offset]\n\t"
            : [out_offset] "+&r"(out_offset)
            : [input] "r"(&in[in_offset]), [output] "r"(out)
            : "ymm31", "ymm30", "ymm29", "ymm28", "k7", "k6", "r9", "r10",
              "r11", "memory");
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
                         "addq %%r11, %[out_offset]\n\t"
                         "addq $8, %[out_offset]\n\t"
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
            // split the control mask into two 32-bit masks
            "movq %[control], %%r11\n\t"
            "movl %%r11d, %%r9d\n\t"
            "shrq $32, %%r11\n\t"
            "kmovd %%r9d, %%k6\n\t"
            "kmovd %%r11d, %%k5\n\t"
            // calculate the input offset of the second half
            "popcntq %%r9, %%r9\n\t"
            "addq %[control_end], %%r9\n\t"
            // expand the first 32 bytes
            "vpexpandb (%[input], %[control_end]), %%ymm31 %{%%k6%}%{z%}\n\t"
            // expand the second 32 bytes
            "vpexpandb (%[input], %%r9), %%ymm30 %{%%k5%}%{z%}\n\t"
            // write the first 32 bytes
            "vmovdqu8 %%ymm31, (%[output], %[out_offset])\n\t"
            // write the second 32 bytes
            "vmovdqu8 %%ymm30, 32(%[output], %[out_offset])\n\t"
            :
            : [input] "r"(in), [output] "r"(out),
              [control_end] "r"(control_end), [out_offset] "r"(out_offset),
              [control] "r"(control)
            : "ymm31", "ymm30", "k6", "k5", "r9", "r11", "memory");

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
