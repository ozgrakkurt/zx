export module zx:backtrace;
import :def;

export namespace zx {
namespace backtrace {

enum class [[nodiscard]] GetContextStatus : u8_c {
    Ok = 0,
    Nullptr,
};

extern "C" struct Context {
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 rbp;
    u64 rbx;
    u64 rsp;
    u64 rip;
};

#ifdef ZX_DEBUG_INFO

extern "C" void zx_backtrace_getcontext_(Context *out);

GetContextStatus getcontext(Context *out) {
    if (out == nullptr) {
        return GetContextStatus::Nullptr;
    }

    zx_backtrace_getcontext_(out);
    return GetContextStatus::Ok;
}

#else // ifdef ZX_DEBUG_INFO

GetContextStatus getcontext(Context *out) {
    if (out == nullptr) {
        return GetContextStatus::Nullptr;
    }

    __builtin_memset(out, 0, sizeof(Context));
    return GetContextStatus::Ok;
}

#endif // ifdef ZX_DEBUG_INFO

} // namespace backtrace
} // namespace zx
