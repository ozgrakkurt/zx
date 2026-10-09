export module zx:alloc;
import :def;
import :assert;
import :slice;
import :arith;

export namespace zx {

enum class [[nodiscard]] AllocStatus : u8_c {
    Ok = 0,
    IntegerOverflow,
    OutOfMemory,
    Nullptr,
};

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

    enum class [[nodiscard]] ResizeStatus : u8_c {
        Ok = 0,
        OutOfBounds,
    };

    ResizeStatus resize(u64 offset) {
        if (offset > this->m_mem.len()) {
            return ResizeStatus::OutOfBounds;
        }

#ifdef ZX_DEV
        if (this->m_offset > offset) {
            // shrinking backwards
            u64 const len = this->m_offset - offset;
            this->m_mem.slice_unsafe(offset, len).memset(0xaa);
        } else if (this->m_offset < offset) {
            // extending forwards
            u64 const len = offset - this->m_offset;
            this->m_mem.slice_unsafe(this->m_offset, len).memset(0xaa);
        }
#endif

        this->m_offset = offset;

        return ResizeStatus::Ok;
    }

    u64 offset() const { return this->m_offset; }

    template <u16 ALIGN, typename T>
    AllocStatus aligned_alloc(u64 len, SliceMut<T> *out) {
        static_assert(ALIGN >= alignof(T));
        static_assert(ALIGN <= 128);
        static_assert(is_pow_two_u16(ALIGN));

        if (out == nullptr) {
            return AllocStatus::Nullptr;
        }

        if (len == 0) {
            *out = SliceMut<T>::template aligned_init_dangling<ALIGN>();
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

    template <typename T> AllocStatus alloc(u64 len, SliceMut<T> *out) {
        return this->aligned_alloc<alignof(T), T>(len, out);
    }

    template <typename T> AllocStatus create(T **out) {
        if (out == nullptr) {
            return AllocStatus::Nullptr;
        }

        SliceMut<T> slice;
        AllocStatus const status = this->alloc<T>(1, &slice);
        if (status != AllocStatus::Ok) {
            return status;
        }

        *out = slice.offset_mut_unsafe(0);
        return AllocStatus::Ok;
    }

    template <typename T>
    AllocStatus copy_from(Slice<T> data, SliceMut<T> *out) {
        if (out == nullptr) {
            return AllocStatus::Nullptr;
        }

        SliceMut<T> mem;
        AllocStatus const status = this->alloc(data.len(), &mem);
        if (status != AllocStatus::Ok) {
            return status;
        }

        mem.memcpy_from_unsafe(data);

        *out = mem;
        return AllocStatus::Ok;
    }
};

} // namespace zx
