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
    u64 rip;
};

extern "C" void zx_getcpucontext_(CPUContext *out);

} // namespace zx
