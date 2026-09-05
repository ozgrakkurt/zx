export module zx:alloc;
import :def;
import :assert;
import :slice;
import :arith;

export namespace zx {

struct Alloc {
    SliceMut<u8> m_mem;
    u64 m_offset;

    static Alloc init(SliceMut<u8> mem) {
#ifdef ZX_DEV
        mem.memset(0xaa);
#endif
        return Alloc{
            .m_mem = mem,
            .m_offset = 0,
        };
    }

    u64 capacity_left() const {
        return sub_unsafe_u64(this->m_mem.len(), this->m_offset);
    }

    void reset() {
#ifdef ZX_DEV
        this->m_mem.memset(0xaa);
#endif
        this->m_offset = 0;
    }

    enum class [[nodiscard]] AllocStatus : u8_c {
        Ok = 0,
        IntegerOverflow,
        OutOfMemory,
        Nullptr,
    };

    template <u16 ALIGN, typename T>
    AllocStatus aligned_alloc(u64 len, SliceMut<T> *out) {
        static_assert(ALIGN >= alignof(T));
        static_assert(ALIGN <= 128);
        static_assert(is_pow_two_u16(ALIGN));

        if (out == nullptr) {
            return AllocStatus::Nullptr;
        }

        if (len == 0) {
            *out = SliceMut<T>::aligned_init_dangling();
            return AllocStatus::Ok;
        }

        SliceMut<u8> const mem = this->m_mem;
        u64 const offset = this->m_offset;

        bool overflow = false;

        u64 const mem_pos = add_u64((u64)mem.ptr(), offset, &overflow);
        u64 const mem_aligned = align_up_u64(mem_pos, ALIGN, &overflow);
        u64 const padding = sub_u64(mem_aligned, mem_pos, &overflow);

        u64 const size = mul_u64(len, sizeof(T), &overflow);
        u64 const alloc_offset = add_u64(offset, padding, &overflow);
        u64 const new_offset = add_u64(alloc_offset, size, &overflow);

        if (overflow) {
            return AllocStatus::IntegerOverflow;
        }

        if (new_offset > mem.len()) {
            return AllocStatus::OutOfMemory;
        }

        // should be safe since we check bounds beforehand
        SliceMut<u8> out_mem = mem.slice_unsafe(alloc_offset, size);

#ifdef ZX_DEV
        out_mem.memset(0xaa);
#endif

        this->m_offset = new_offset;
        // safe since we precisely construct the out_mem to be a valid slice of
        // T
        *out = out_mem.cast_unsafe<T>();

        return AllocStatus::Ok;
    }

    template <typename T> AllocStatus alloc(u64 len, SliceMut<u8> *out) {
        return this->aligned_alloc<alignof(T), T>(len, out);
    }
};

} // namespace zx
