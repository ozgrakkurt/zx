export module zx:list;
import :def;
import :arith;
import :slice;

export namespace zx {

enum class [[nodiscard]] ListPushStatus : u8_c {
    Ok = 0,
    OutOfCapacity,
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

    ListPushStatus push(T val) {
        assert_dev(this->m_offset <= this->m_slice.len());
        if (this->m_offset == this->m_slice.len()) {
            return ListPushStatus::OutOfCapacity;
        }

        this->m_slice.write_unsafe(this->m_offset, val);
        this->m_offset += 1;
        return ListPushStatus::Ok;
    }
};

} // namespace zx
