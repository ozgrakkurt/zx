export module zx:fuzz;
import :def;
import :slice;
import :arith;
import :assert;
import :alloc;

export namespace zx {

struct FuzzInput {
    Slice<u64> m_input;
    u64 m_consumed_count;

    static FuzzInput init_(Slice<u64> input) {
        return FuzzInput{
            .m_input = input,
            .m_consumed_count = 0,
        };
    }

    u64 next() {
        u64 const offset = this->m_consumed_count;
        this->m_consumed_count = add_unsafe_u64(offset, 1);
        assert(this->m_input.len() > offset);
        return this->m_input.read_unsafe(offset);
    }

    u64 consumed_count_() const { return this->m_consumed_count; }
};

typedef void (*fuzz_one_fn)(void *usr_ctx, FuzzInput *fuzz_input);

struct FuzzParams {
    Alloc *alloc;
    void *usr_ctx;
    fuzz_one_fn fuzz_one;
    u64 iter_limit;
};

void fuzz(FuzzParams params);

} // namespace zx
