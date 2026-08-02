export module zx:sys;
import :syscall;
import :def;

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

export namespace zx {
} // namespace zx
