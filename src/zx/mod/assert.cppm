export module zx:assert;
import :panic;

export namespace zx {

void assert(bool condition) {
    if (!condition) [[unlikely]] {
        panic();
    }
}

#ifdef ZX_DEV
void assert_dev(bool condition) {
    if (!condition) [[unlikely]] {
        panic();
    }
}
#else
void assert_dev(bool condition) { (void)condition; }
#endif

} // namespace zx
