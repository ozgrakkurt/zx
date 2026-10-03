export module zx:list;
import :def;
import :arith;
import :slice;

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

    bool is_full() const { return this->m_offset == this->m_slice; }

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

    ListPushStatus push(T val) {
        assert_dev(this->m_offset <= this->m_slice.len());
        if (this->m_offset == this->m_slice.len()) {
            return ListPushStatus::OutOfCapacity;
        }

        this->m_slice.write_unsafe(this->m_offset, val);
        this->m_offset += 1;
        return ListPushStatus::Ok;
    }

    void push_panic(T val) {
        assert(this->m_offset < this->m_slice.len());
        this->m_slice.write_unsafe(this->m_offset, val);
        this->m_offset += 1;
    }

    ListPopStatus pop(T *out) {
        assert_dev(this->m_offset <= this->m_slice.len());

        if (out == nullptr) {
            return ListPopStatus::Nullptr;
        }

        if (this->m_offset == 0) {
            return ListPopStatus::Empty;
        }

        this->m_offset -= 1;
        *out = this->m_slice.read_unsafe(this->m_offset);
        return ListPopStatus::Ok;
    }

    T pop_panic() {
        assert(this->m_offset > 0);
        this->m_offset -= 1;
        return this->m_slice.read_unsafe(this->m_offset);
    }
};

} // namespace zx
