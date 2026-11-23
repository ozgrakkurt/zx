export module zx:libc;
import :def;
import :vec;
import :assert;

export namespace zx {

__attribute__((no_builtin)) void *
memcpy(void *__restrict dest, void const *__restrict src, u64_c count) {
    u8 const *input = (u8 const *)src;
    u8 *output = (u8 *)dest;

    // Large copies: the CPU's rep movsb path streams better than our vector
    // loop. Only use it when the destination is 64 byte aligned or when source
    // and destination share the same 64 byte phase; otherwise rep movsb can hit
    // a very slow alignment case on AMD.
    if (count >= (u64{48} << 20__wbu) && ((((u64)dest ^ (u64)src) & 63) == 0 ||
                                          ((u64)dest & 63) == 0)) [[unlikely]] {
        u64_c n = count;
        void *d = output;
        void const *s = input;
        __asm__ __volatile__("rep movsb"
                             : "+D"(d), "+S"(s), "+c"(n)
                             :
                             : "memory");
        return dest;
    }

    u64 offset = count % 64;
    if (offset > 0) {
        #pragma clang loop vectorize(disable) unroll(disable)
        for (u64 i=0; i<offset; i++) {
            output[i] = input[i];
        }
    }

    for (; offset < count; offset += 64__wbu) {
        __asm__ volatile(
            "vmovups (%[input]), %%ymm31\n\t"
            "vmovups 32(%[input]), %%ymm30\n\t"
            "vmovups %%ymm31, (%[output])\n\t"
            "vmovups %%ymm30, 32(%[output])\n\t"
            :
            : [input] "r"(&input[offset]), [output] "r"(&output[offset])
            : "ymm31", "ymm30", "memory");
    }

    return dest;
}

__attribute__((no_builtin)) void *memset(void *dest, i32_c ch, u64_c count) {
    u8 *output = (u8 *)dest;
    u32_c const byte = (u8)ch;

    v256_u8 vbytes;
    __asm__("vpbroadcastb %[byte], %[vbytes]\n\t"
            : [vbytes] "=v"(vbytes)
            : [byte] "r"(byte));

    v512_u8 vbytes512;
    __asm__("vpbroadcastb %[byte], %[vbytes512]\n\t"
            : [vbytes512] "=v"(vbytes512)
            : [byte] "r"(byte));

    u64 offset = 0;

    for (; offset + 64__wbu <= count; offset += 64__wbu) {
        __asm__("vmovdqu8 %[vbytes], (%[output])\n\t"
                "vmovdqu8 %[vbytes], 32(%[output])\n\t"
                : "=m"(*(u8(*)[64]) & output[offset])
                : [vbytes] "v"(vbytes), [output] "r"(&output[offset]));
    }

    u64 const remainder = count - offset;

    if (remainder > 0) {
        u64_c const storemask = __builtin_ia32_bzhi_di(~(u64){0}, remainder);
        __asm__("vmovdqu8 %[vbytes], %[output] %{%[storemask]%}\n\t"
                : [output] "=m"(*(u8(*)[64]) & output[offset])
                : [vbytes] "v"(vbytes512), [storemask] "Yk"(storemask));
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
    // we can read anywhere in the 4KB page that includes x.
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
