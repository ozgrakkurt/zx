export module zx:assert;
import :panic;

export namespace zx {

inline void assert(bool condition) {
    if (!condition) [[unlikely]] {
        panic();
    }
}

#ifdef ZX_DEV
inline void assert_dev(bool condition) {
    if (!condition) [[unlikely]] {
        panic();
    }
}
#else
inline void assert_dev(bool condition) { (void)condition; }
#endif

} // namespace zx
