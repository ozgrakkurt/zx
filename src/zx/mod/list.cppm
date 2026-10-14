export module zx:list;
import :def;
import :arith;
import :slice;
import :backtrace;

export namespace zx {

enum class [[nodiscard]] ListPushStatus : u8_c {
    Ok = 0,
    OutOfCapacity,
};

enum class [[nodiscard]] ListPopStatus : u8_c {
    Ok = 0,
    Nullptr,
    Empty,
};

template <typename T> struct List {
    SliceMut<T> m_slice;
    u64 m_offset;

    static List init(SliceMut<T> slice) {
        return List{
            .m_slice = slice,
            .m_offset = 0,
        };
    }

    u64 offset() const { return this->m_offset; }

    SliceMut<T> slice() const { return this->m_slice; }

    bool is_full() const { return this->m_offset == this->m_slice.len(); }

    Slice<T> written() const {
        return this->m_slice.slice_unsafe(0, this->m_offset).as_const();
    }

    u64 n_written() const { return this->m_offset; }

    void reset() {
#ifdef ZX_DEV
        this->m_slice.memset(0xaa);
#endif
        this->m_offset = 0;
    }

    ListPushStatus push(Backtrace *err_bt, T val) {
        assert_dev(this->m_offset <= this->m_slice.len());
        if (this->m_offset == this->m_slice.len()) [[unlikely]] {
            Backtrace::collect(err_bt);
            return ListPushStatus::OutOfCapacity;
        }

        this->m_slice.write_unsafe(this->m_offset, val);
        this->m_offset += 1;
        return ListPushStatus::Ok;
    }

    ListPopStatus pop(Backtrace *err_bt, T *out) {
        assert_dev(this->m_offset <= this->m_slice.len());

        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return ListPopStatus::Nullptr;
        }

        if (this->m_offset == 0) [[unlikely]] {
            Backtrace::collect(err_bt);
            return ListPopStatus::Empty;
        }

        this->m_offset -= 1;
        *out = this->m_slice.read_unsafe(this->m_offset);
        return ListPopStatus::Ok;
    }
};

} // namespace zx
