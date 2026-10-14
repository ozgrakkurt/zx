import zx;

using namespace zx;

#ifdef ZX_DEBUG_INFO

void cmmb(Backtrace *err_bt) {
    Backtrace::collect(err_bt);
    elf::print_stack_trace_panic(err_bt);
}

void callme_maybe(Backtrace *err_bt, u64 iter) {
    if (iter > 5) {
        Backtrace::collect(err_bt);
        elf::print_stack_trace_panic(err_bt);
    } else {
        callme_maybe(err_bt, iter + 1);
    }
}

int main() {
    Backtrace err_bt = Backtrace::init();

    callme_maybe(&err_bt, 0);
    cmmb(&err_bt);

    // uncomment to actually see the print outputs in the terminal.
    // sys::exit_group(sys::EXIT_FAILURE);

    return 0;
}

#else // ifdef ZX_DEBUG_INFO

int main() { return 0; }

#endif // ifdef ZX_DEBUG_INFO
