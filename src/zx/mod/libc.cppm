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
        u64_c const fullmask = __builtin_ia32_bzhi_di(U64_MAX, offset);

        u32_c const mask0 = (u32_c)fullmask;
        u32_c const mask1 = (u32_c)(fullmask >> 32);

        __asm__ volatile(
            "vmovdqu8 (%[input]), %%ymm29 %{%[mask0]%}%{z%}\n\t"
            "vmovdqu8 %%ymm29, (%[output]) %{%[mask0]%}\n\t"
            :
            : [input] "r"(input), [output] "r"(output), [mask0] "Yk"(mask0)
            : "memory", "ymm29");

        __asm__ volatile(
            "vmovdqu8 32(%[input]), %%ymm28 %{%[mask1]%}%{z%}\n\t"
            "vmovdqu8 %%ymm28, 32(%[output]) %{%[mask1]%}\n\t"
            :
            : [input] "r"(input), [output] "r"(output), [mask1] "Yk"(mask1)
            : "memory", "ymm28");
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

    assert_dev(offset == count);

    return dest;
}

__attribute__((no_builtin)) void *memset(void *dest, i32_c ch, u64_c count) {
    u8 *output = (u8 *)dest;
    u32_c const byte = (u8)ch;

    v256_u8 vbytes;
    __asm__("vpbroadcastb %[byte], %[vbytes]\n\t"
            : [vbytes] "=v"(vbytes)
            : [byte] "r"(byte));

    u64 offset = count % 64;
    if (offset > 0) {
        u64_c const fullmask = __builtin_ia32_bzhi_di(U64_MAX, offset);

        u32_c const mask0 = (u32_c)fullmask;
        u32_c const mask1 = (u32_c)(fullmask >> 32);

        __asm__ volatile(
            "vmovdqu8 %[vbytes], (%[output]) %{%[mask0]%}\n\t"
            :
            : [vbytes] "v"(vbytes), [output] "r"(output), [mask0] "Yk"(mask0)
            : "memory");
        __asm__ volatile(
            "vmovdqu8 %[vbytes], 32(%[output]) %{%[mask1]%}\n\t"
            :
            : [vbytes] "v"(vbytes), [output] "r"(output), [mask1] "Yk"(mask1)
            : "memory");
    }

    for (; offset < count; offset += 64__wbu) {
        __asm__ volatile("vmovdqu8 %[vbytes], (%[output])\n\t"
                         "vmovdqu8 %[vbytes], 32(%[output])\n\t"
                         :
                         : [vbytes] "v"(vbytes), [output] "r"(&output[offset])
                         : "memory");
    }

    assert_dev(offset == count);

    return dest;
}

__attribute__((no_builtin)) i32_c memcmp(void const *lhs, void const *rhs,
                                         u64_c count) {
    u8 const *left = (u8 const *)lhs;
    u8 const *right = (u8 const *)rhs;

    u64 offset = count % 64;
    if (offset > 0) {
        u64_c const loadmask = __builtin_ia32_bzhi_di(U64_MAX, offset);
        u64_c nonzero_mask;
        __asm__ volatile(
            "vmovdqu8 (%[left]), %%zmm31 %{%[loadmask]%}%{z%}\n\t"
            "vmovdqu8 (%[right]), %%zmm30 %{%[loadmask]%}%{z%}\n\t"
            // 4 means Not Equal OP
            "vpcmpub $4, %%zmm30, %%zmm31, %[nonzero_mask]\n\t"
            : [nonzero_mask] "=Yk"(nonzero_mask)
            : [left] "r"(left), [right] "r"(right), [loadmask] "Yk"(loadmask)
            : "memory", "zmm31", "zmm30");
        if (nonzero_mask != 0) {
            // find first diff
            u8 const idx = (u8)__builtin_ctzg(nonzero_mask, 64);
            // idx can't be oob of the vector.
            assert_dev(idx < 64);
            // idx can't be oob of remainder.
            assert_dev(idx < offset);
            u64 const pos = idx;
            i32 const ret = i32{left[pos]} - i32{right[pos]};
            // can't be equal
            assert_dev(ret != 0);
            return ret;
        }
    }

    for (; offset < count; offset += 64__wbu) {
        u64_c nonzero_mask;
        __asm__ volatile(
            "vmovdqu8 (%[left]), %%zmm29\n\t"
            "vmovdqu8 (%[right]), %%zmm28\n\t"
            // 4 means Not Equal OP
            "vpcmpub $4, %%zmm28, %%zmm29, %[nonzero_mask]\n\t"
            : [nonzero_mask] "=Yk"(nonzero_mask)
            : [left] "r"(&left[offset]), [right] "r"(&right[offset])
            : "memory", "zmm29", "zmm28");
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

    assert_dev(offset == count);

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
