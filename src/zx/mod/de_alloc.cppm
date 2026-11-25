export module zx:de_alloc;
import :def;
import :assert;
import :slice;
import :arith;
import :backtrace;
// do this since we want to use the same error enum
import :alloc;

export namespace zx {

// Casing is DEA so it looks like drugs.
struct DEAlloc {
    SliceMut<u8> m_mem;
    u64 m_left_offset;
    u64 m_right_offset;

    inline static DEAlloc init(SliceMut<u8> mem) {
#ifdef ZX_DEV
        mem.memset(0xaa);
#endif
        return DEAlloc{
            .m_mem = mem, .m_left_offset = 0, .m_right_offset = mem.len()};
    }

    inline void reset() {
        SliceMut<u8> const mem = this->m_mem;

#ifdef ZX_DEV
        mem.slice_unsafe(0, this->m_left_offset).memset(0xaa);
#endif
        this->m_left_offset = 0;

#ifdef ZX_DEV
        u64 const right_len = sub_unsafe_u64(mem.len(), this->m_right_offset);
        m_mem.slice_unsafe(this->m_right_offset, right_len).memset(0xaa);
#endif
        this->m_right_offset = mem.len();
    }

    inline AllocResizeStatus left_resize(Backtrace *err_bt, u64 offset) {
        if (offset > this->m_right_offset) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocResizeStatus::OutOfBounds;
        }

#ifdef ZX_DEV
        u64 const left_offset = this->m_left_offset;
        if (left_offset > offset) {
            // shrinking backwards
            u64 const len = left_offset - offset;
            this->m_mem.slice_unsafe(offset, len).memset(0xaa);
        } else if (left_offset < offset) {
            // extending forwards
            u64 const len = offset - left_offset;
            this->m_mem.slice_unsafe(left_offset, len).memset(0xaa);
        }
#endif

        this->m_left_offset = offset;

        return AllocResizeStatus::Ok;
    }

    inline AllocResizeStatus right_resize(Backtrace *err_bt, u64 offset) {
        if (offset < this->m_left_offset || offset > this->m_mem.len())
            [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocResizeStatus::OutOfBounds;
        }

#ifdef ZX_DEV
        u64 const right_offset = this->m_right_offset;
        if (right_offset > offset) {
            // shrinking forwards
            u64 const len = right_offset - offset;
            this->m_mem.slice_unsafe(offset, len).memset(0xaa);
        } else {
            // shrinking backwards
            u64 const len = offset - right_offset;
            this->m_mem.slice_unsafe(right_offset, len).memset(0xaa);
        }
#endif

        this->m_right_offset = offset;

        return AllocResizeStatus::Ok;
    }

    inline u64 left_offset() const { return this->m_left_offset; }

    inline u64 right_offset() const { return this->m_right_offset; }

    template <typename T>
    inline AllocStatus left_aligned_alloc(Backtrace *err_bt, u16 align, u64 len,
                                          SliceMut<T> *out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::Nullptr;
        }

        if (!is_pow_two_u16(align)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::InvalidAlign;
        }

        if (align > 128) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::InvalidAlign;
        }

        if (align < alignof(T)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::InvalidAlign;
        }

        if (len == 0) {
            *out = SliceMut<T>::aligned_init_dangling_unsafe(align);
            return AllocStatus::Ok;
        }

        SliceMut<u8> const mem = this->m_mem;
        u64 const left_offset = this->m_left_offset;

        bool overflow = false;

        u64 const mem_pos = add_u64((u64)mem.ptr(), left_offset, &overflow);
        u64 const mem_aligned = align_up_u64(mem_pos, align, &overflow);
        u64 const padding = sub_u64(mem_aligned, mem_pos, &overflow);

        u64 const size = mul_u64(len, sizeof(T), &overflow);
        u64 const alloc_offset = add_u64(left_offset, padding, &overflow);
        u64 const new_offset = add_u64(alloc_offset, size, &overflow);

        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::IntegerOverflow;
        }

        if (new_offset > this->m_right_offset) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::OutOfMemory;
        }

        // should be safe since we check bounds beforehand
        SliceMut<u8> out_mem = mem.slice_unsafe(alloc_offset, size);

#ifdef ZX_DEV
        out_mem.memset(0xaa);
#endif

        this->m_left_offset = new_offset;
        // safe since we precisely construct the out_mem to be a valid slice of
        // T
        *out = out_mem.cast_unsafe<T>();

        return AllocStatus::Ok;
    }

    template <typename T>
    inline AllocStatus right_aligned_alloc(Backtrace *err_bt, u16 align,
                                           u64 len, SliceMut<T> *out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::Nullptr;
        }

        if (!is_pow_two_u16(align)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::InvalidAlign;
        }

        if (align > 128) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::InvalidAlign;
        }

        if (align < alignof(T)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::InvalidAlign;
        }

        if (len == 0) {
            *out = SliceMut<T>::aligned_init_dangling_unsafe(align);
            return AllocStatus::Ok;
        }

        SliceMut<u8> const mem = this->m_mem;
        u64 const right_offset = this->m_right_offset;

        bool overflow = false;

        u64 const mem_pos = add_u64((u64)mem.ptr(), right_offset, &overflow);
        u64 const mem_aligned = align_down_u64(mem_pos, align, &overflow);
        u64 const padding = sub_u64(mem_pos, mem_aligned, &overflow);

        u64 const size = mul_u64(len, sizeof(T), &overflow);
        u64 const alloc_size = add_u64(size, padding, &overflow);

        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::IntegerOverflow;
        }

        // Overflow means out of memory here so we seperate the overflow check
        // for this operation from the rest of them.
        u64 const alloc_offset = sub_u64(right_offset, alloc_size, &overflow);

        if (overflow || alloc_offset < this->m_left_offset) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::OutOfMemory;
        }

        // should be safe since we check bounds beforehand
        SliceMut<u8> out_mem = mem.slice_unsafe(alloc_offset, size);

#ifdef ZX_DEV
        out_mem.memset(0xaa);
#endif

        this->m_right_offset = alloc_offset;
        // safe since we precisely construct the out_mem to be a valid slice of
        // T
        *out = out_mem.cast_unsafe<T>();

        return AllocStatus::Ok;
    }

    template <typename T>
    inline AllocStatus left_alloc(Backtrace *err_bt, u64 len,
                                  SliceMut<T> *out) {
        return this->left_aligned_alloc<T>(err_bt, alignof(T), len, out);
    }

    template <typename T>
    inline AllocStatus right_alloc(Backtrace *err_bt, u64 len,
                                   SliceMut<T> *out) {
        return this->right_aligned_alloc<T>(err_bt, alignof(T), len, out);
    }

    template <typename T>
    inline AllocStatus left_create(Backtrace *err_bt, T **out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::Nullptr;
        }

        SliceMut<T> slice;
        AllocStatus const status = this->left_alloc<T>(err_bt, 1, &slice);
        if (status != AllocStatus::Ok) {
            return status;
        }

        *out = slice.offset_mut_unsafe(0);
        return AllocStatus::Ok;
    }

    template <typename T>
    inline AllocStatus right_create(Backtrace *err_bt, T **out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::Nullptr;
        }

        SliceMut<T> slice;
        AllocStatus const status = this->right_alloc<T>(err_bt, 1, &slice);
        if (status != AllocStatus::Ok) {
            return status;
        }

        *out = slice.offset_mut_unsafe(0);
        return AllocStatus::Ok;
    }

    template <typename T>
    inline AllocStatus left_copy_from(Backtrace *err_bt, Slice<T> data,
                                      SliceMut<T> *out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::Nullptr;
        }

        SliceMut<T> mem;
        AllocStatus const status = this->left_alloc(err_bt, data.len(), &mem);
        if (status != AllocStatus::Ok) {
            return status;
        }

        mem.memcpy_from_unsafe(data);

        *out = mem;
        return AllocStatus::Ok;
    }

    template <typename T>
    inline AllocStatus right_copy_from(Backtrace *err_bt, Slice<T> data,
                                       SliceMut<T> *out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return AllocStatus::Nullptr;
        }

        SliceMut<T> mem;
        AllocStatus const status = this->right_alloc(err_bt, data.len(), &mem);
        if (status != AllocStatus::Ok) {
            return status;
        }

        mem.memcpy_from_unsafe(data);

        *out = mem;
        return AllocStatus::Ok;
    }
};

} // namespace zx
