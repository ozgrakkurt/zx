import zx;

using namespace zx;

// Regression test for ELF parsing at startup: a large zero-initialized global
// lives in a SHT_NOBITS section (.bss) and has no bytes in the executable
// file. A debug build must not try to slice the file buffer using the .bss
// section's offset/size.
//
// `volatile` keeps the compiler from optimizing the array and its accesses
// away, so it reliably ends up in .bss.
static volatile u8 big_bss[8 * 1024 * 1024];

int main() {
    big_bss[0] = 1;
    big_bss[sizeof(big_bss) - 1] = 2;

    return (big_bss[0] == 1 && big_bss[sizeof(big_bss) - 1] == 2) ? 0 : 1;
}
