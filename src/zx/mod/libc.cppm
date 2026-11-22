export module zx:libc;
import :def;
import :vec;
import :assert;

export namespace zx {

// Should strive to make these implementations SMALL. As in the machine code
// output should be as small as possible.
//
// For example memcpy doesn't have to be the absolute best at copying 10GB of
// memory, the caller should benchmark and write something that is actually good
// at that size if they have a usecase like this.
//
// We can keep the size small and code basic here so we don't trash the
// instruction cache for imaginary performance gains.

__attribute__((no_builtin)) void *
memcpy(void *__restrict dest, void const *__restrict src, u64_c count) {
    u8 const *input = (u8 const *)src;
    u8 *output = (u8 *)dest;

    u64 offset = 0;

    if (!__builtin_is_aligned(output, 64)) {
        // align output here by copying x bytes from input to output.
        // This will make the rest of the function run faster.
        u64 const addr = (u64)output;
        u64 const up_addr = __builtin_align_up(addr, 64);
        assert_dev(up_addr > addr);
        u64 const diff = up_addr - addr;
        assert_dev(diff < 64);

        if (diff <= count) {
            u64_c const storemask = __builtin_ia32_bzhi_di(~(u64){0}, diff);
            v512_u8 scratch;
            __asm__("vmovdqu8 %[input], %[scratch] %{%[storemask]%}%{z%}\n\t"
                    "vmovdqu8 %[scratch], %[output] %{%[storemask]%}\n\t"
                    : [output] "=m"(*(u8(*)[64])output), [scratch] "=v"(scratch)
                    : [input] "m"(*(u8 const(*)[64])input), [storemask] "Yk"(
                                                                storemask));
            offset = diff;
        }
    }
    for (; offset + 64__wbu <= count; offset += 64__wbu) {
        v512_u8 scratch;
        __asm__("vmovups %[input], %[scratch]\n\t"
                "vmovups %[scratch], %[output]\n\t"
                : [output] "=m"(*(u8(*)[64]) & output[offset]), [scratch] "=v"(
                                                                    scratch)
                : [input] "m"(*(u8 const(*)[64]) & input[offset]));
    }

    u64 const remainder = count - offset;
    if (remainder > 0) {
        u64_c const storemask = __builtin_ia32_bzhi_di(~(u64){0}, remainder);
        v512_u8 scratch;
        __asm__(
            "vmovdqu8 %[input], %[scratch] %{%[storemask]%}%{z%}\n\t"
            "vmovdqu8 %[scratch], %[output] %{%[storemask]%}\n\t"
            : [output] "=m"(*(u8(*)[64]) & output[offset]), [scratch] "=v"(
                                                                scratch)
            : [input] "m"(*(u8 const(*)[64]) & input[offset]), [storemask] "Yk"(
                                                                   storemask));
    }

    return dest;
}

__attribute__((no_builtin)) void *memset(void *dest, i32_c ch, u64_c count) {
    u8 *output = (u8 *)dest;
    u32_c const byte = (u8)ch;

    v512_u8 vbytes;
    __asm__("vpbroadcastb %[byte], %[vbytes]\n\t"
            : [vbytes] "=v"(vbytes)
            : [byte] "r"(byte));

    u64 offset = 0;

    for (; offset + 64__wbu <= count; offset += 64) {
        __asm__("vmovdqu8 %[vbytes], %[output]\n\t"
                : [output] "=m"(*(u8(*)[64]) & output[offset])
                : [vbytes] "v"(vbytes));
    }

    u64 const remainder = count - offset;

    if (remainder > 0) {
        u64_c const storemask = __builtin_ia32_bzhi_di(~(u64){0}, remainder);
        __asm__("vmovdqu8 %[vbytes], %[output] %{%[storemask]%}\n\t"
                : [output] "=m"(*(u8(*)[64]) & output[offset])
                : [vbytes] "v"(vbytes), [storemask] "Yk"(storemask));
    }

    return dest;
}

__attribute__((no_builtin)) i32_c memcmp(void const *lhs, void const *rhs,
                                         u64_c count) {
    u8 const *left = (u8 const *)lhs;
    u8 const *right = (u8 const *)rhs;

    u64 offset = 0;
    for (; offset + 64__wbu <= count; offset += 64__wbu) {
        u64_c nonzero_mask;
        __asm__("vmovdqu8 %[left], %%zmm31\n\t"
                "vmovdqu8 %[right], %%zmm30\n\t"
                // 4 means Not Equal OP
                "vpcmpub $4, %%zmm30, %%zmm31, %[nonzero_mask]\n\t"
                : [nonzero_mask] "=&k"(nonzero_mask)
                : [left] "m"(*(u8 const(*)[64]) & left[offset]),
                  [right] "m"(*(u8 const(*)[64]) & right[offset])
                : "zmm30", "zmm31");
        if (nonzero_mask != 0) {
            // find first diff
            u8 const idx = (u8)__builtin_ctzg(nonzero_mask, 64);
            // idx can't be oob of the vector.
            assert_dev(idx < 64);
            u64 const pos = offset + idx;
            i32 const ret = i32{left[pos]} - i32{right[pos]};
            // can't be equal
            assert_dev(ret != 0);
            return ret;
        }
    }

    u64 const remainder = count - offset;
    if (remainder > 0) {
        register u64_c const storemask __asm__("k7") =
            __builtin_ia32_bzhi_di(~(u64){0}, remainder);
        u64_c nonzero_mask;
        __asm__("vmovdqu8 %[left], %%zmm31 %{%[storemask]%}%{z%}\n\t"
                "vmovdqu8 %[right], %%zmm30 %{%[storemask]%}%{z%}\n\t"
                // 4 means Not Equal OP
                "vpcmpub $4, %%zmm30, %%zmm31, %[nonzero_mask]\n\t"
                : [nonzero_mask] "=&k"(nonzero_mask)
                : [left] "m"(*(u8 const(*)[64]) & left[offset]),
                  [right] "m"(*(u8 const(*)[64]) & right[offset]),
                  [storemask] "k"(storemask)
                : "zmm30", "zmm31");
        if (nonzero_mask != 0) {
            // find first diff
            u8 const idx = (u8)__builtin_ctzg(nonzero_mask, 64);
            // idx can't be oob of the vector.
            assert_dev(idx < 64);
            // idx can't be oob of remainder.
            assert_dev(idx < remainder);
            u64 const pos = offset + idx;
            i32 const ret = i32{left[pos]} - i32{right[pos]};
            // can't be equal
            assert_dev(ret != 0);
            return ret;
        }
    }

    return 0;
}

__attribute__((no_builtin)) u64_c strlen(char const *str) {
    // The key to vectorizing this is to realize that we can read anything as
    // long as we don't go over page boundaries prematurely.
    //
    // If the regular strlen semantics allow us to read memory location x then
    // we can read anywhere in the 4096KB page that includes x.
    //
    // We can use vectorization by exploiting this fact.

    v512_u8 zeros;
    __asm__("vpxorq %[zeros], %[zeros], %[zeros]\n\t" : [zeros] "=v"(zeros));

    // align down if needed so we don't go over a page boundary (4096 > 64)
    if (!__builtin_is_aligned(str, 64)) {
        u64 const addr = (u64)str;
        u64 const down_addr = __builtin_align_down(addr, 64);
        assert_dev(addr >= down_addr);
        u64 const diff64 = addr - down_addr;
        assert_dev(diff64 < 64);
        u8 const diff = (u8)diff64;
        register u64_c const storemask __asm__("k7") = U64_MAX << diff;
        char const *down = (char const *)down_addr;
        u64_c nonzero_mask;
        __asm__("vmovdqu8 %[down], %%zmm31 %{%[storemask]%}\n\t"
                "kxorq %[nonzero_mask], %[nonzero_mask], %[nonzero_mask]\n\t"
                // 0 means Equal OP
                "vpcmpub $0, %%zmm31, %[zeros], %[nonzero_mask] "
                "%{%[storemask]%}\n\t"
                : [nonzero_mask] "=&k"(nonzero_mask)
                : [down] "m"(*(char const(*)[64])down), [zeros] "v"(zeros),
                  [storemask] "k"(storemask)
                : "zmm31");
        if (nonzero_mask != 0) {
            // find first diff
            u8 const idx = (u8)__builtin_ctzg(nonzero_mask, 64);
            // idx can't be oob of the vector.
            assert_dev(idx < 64);
            assert_dev(idx >= diff);
            return idx - diff;
        }
    }

    // read 64 byte aligned chunks so we never go over a page boundary (4096 >
    // 64)
    u64 const up_addr = __builtin_align_up((u64)str, 64);
    assert_dev(up_addr >= (u64)str);
    u64 offset = up_addr - (u64)str;

    for (;; offset += 64__wbu) {
        u64_c nonzero_mask;
        __asm__(
            "vmovdqu8 %[str], %%zmm31\n\t"
            // 0 means Equal OP
            "vpcmpub $0, %%zmm31, %[zeros], %[nonzero_mask]\n\t"
            : [nonzero_mask] "=&k"(nonzero_mask)
            : [str] "m"(*(char const(*)[64]) & str[offset]), [zeros] "v"(zeros)
            : "zmm31");
        if (nonzero_mask != 0) {
            // find first diff
            u8 const idx = (u8)__builtin_ctzg(nonzero_mask, 64);
            // idx can't be oob of the vector.
            assert_dev(idx < 64);
            return offset + idx;
        }
    }
}

} // namespace zx
