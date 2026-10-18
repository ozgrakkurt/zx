import zx;

using namespace zx;

i32_c main(i32_c argc, i8_c **argv, i8_c **envp) {
    (void)envp;

    bool overflow = false;

    u32 const expected_n_args = i32_to_u32(argc, &overflow);
    assert(!overflow);
    i8_c **arg = argv;
    u32 n_args = 0;
    while (*arg != nullptr) {
        n_args = add_u32(n_args, 1, &overflow);
        assert(!overflow);
        i8_c **next_arg = arg + 1;
        assert((u64)next_arg > (u64)arg);
        arg = next_arg;
    }

    assert(expected_n_args == n_args);

    Backtrace err_bt = Backtrace::init();

    constexpr u64 MEM_SIZE = u64{1} << 15;

    u8 *mem = nullptr;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_bt, MEM_SIZE, &mem),
        &err_bt);
    // sanity check
    assert(mem != nullptr);

    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(mem, MEM_SIZE));

    bool fmt_overflow = false;

    fmt.push_str(CString::init_unsafe("### ARGS: \n").as_slice(),
                 &fmt_overflow);
    assert(!fmt_overflow);

    for (u64 i = 0; i < n_args; i++) {
        fmt.push_char('-', &fmt_overflow);
        Slice<u8> arg_str = CString::init_unsafe(argv[i]).as_slice();
        fmt.push_str(arg_str, &fmt_overflow);
        fmt.push_char('\n', &fmt_overflow);
        assert(!fmt_overflow);
    }

    fmt.push_str(CString::init_unsafe("### ENV: \n").as_slice(), &fmt_overflow);
    assert(!fmt_overflow);

    i8_c **env = envp;
    while (*env != nullptr) {
        fmt.push_char('-', &fmt_overflow);
        Slice<u8> env_str = CString::init_unsafe(*env).as_slice();
        fmt.push_str(env_str, &fmt_overflow);
        fmt.push_char('\n', &fmt_overflow);
        assert(!fmt_overflow);
        env++;
    }

    u64 n_written = 0;
    testing::assert_with_trace(sys::WriteStatus::Ok ==
                                   sys::write(&err_bt, sys::STDERR,
                                              fmt.written().ptr(),
                                              fmt.written().len(), &n_written),
                               &err_bt);
    assert(n_written == fmt.n_written());

    // uncomment to see output with test failure
    // panic();

    testing::assert_with_trace(page_alloc::FreeStatus::Ok ==
                                   page_alloc::free(&err_bt, mem, MEM_SIZE),
                               &err_bt);

    return 0;
}
