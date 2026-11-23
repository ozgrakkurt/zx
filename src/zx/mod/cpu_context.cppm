export module zx:cpu_context;
import :def;

export namespace zx {

extern "C" struct CPUContext {
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 rbp;
    u64 rbx;
    u64 rsp;
    u64 return_address;
};

#ifdef ZX_DEBUG_INFO

extern "C" void zx_getcpucontext_(CPUContext *out);

#else // ifdef ZX_DEBUG_INFO

extern "C" inline void zx_getcpucontext_(CPUContext *out) {
#ifdef ZX_DEV
    __builtin_memset(out, 0xaa, sizeof(CPUContext));
#endif
    (void)out;
}

#endif // ifdef ZX_DEBUG_INFO

} // namespace zx
