# zx

## Developing

### CLI

This is a C++ codebase that uses the [zxbs](https://github.com/ozgrakkurt/zxbs) build system.

Just build with:
```
zxbs build
```

Run tests with:
```
zxbs test
```

Run all fuzz tests with:
```
zxbs --fuzz --opt-level=2 test
```

Run specific fuzz test with:
```
zxbs --fuzz --opt-level=2 test <entry_name>:<fuzz_test_name>
```

Default fuzzing has a time limit, you might want to run a fuzz target forever. Run specific fuzz test forever with:
```
zxbs --fuzz --fuzz-unlimited --opt-level=2 test <entry_name>:<fuzz_test_name>
```

Run a specific binary target with:
```
zxbs run <entry_name>:<binary_target_name> [arguments]
```

Run a specific binary target in release mode with:
```
zxbs --no-dev-mode --no-debug-info --opt-level=2 run <entry_name:binary_target_name> [arguments]
```

Clean build cache with:
```
zxbs clean
```

Check "dev" time with:
```
zxbs clean
time zxbs test
```

If you find that the build timing is getting larger and you don't know why. Run this:
```
zxbs clean
zxbs --time-trace test
```
It will output a trace (.json) file in `build/out` folder. You can open it in `chrome://tracing` page of your chromium browser of choice.
This trace view will show what is slowing the build down.

### Directory structure

Source code is put in folders under the src directory like so:

- `src`
  - <entry_name>
    - mod -> Folder of module files (.cppm) for implementation/interface. These files are expected to contain the majority of the implementation code and contain all of the interface code.
    - lib -> Folder of implementation files for the static library (.a). Each file (.cc) in this directory will be compiled to an object file (.o) and archived together into the library (.a) file for this entry.
            Each binary output that depends on this entry will have this library archive (.a) linked in.
    - test -> Folder of (.cc) files Each file in this directory will be compiled to an object file (.o) and linked into an executable by combining all of the other library (.a) files it depends on. The executable will be run for testing.
    - bin -> Folder of (.cc) files. Each file in this directory will be compiled to an object file (.o) and linked into an executable by combining all of the other library (.a) files it depends on.
    - deps -> This is a file of newline seperated list of entry names that list the dependencies of this entry.

## Platform requirements

We only target linux on x86_64 AMD CPUs `znver4` and later. So we expect the CPU to have recent AVX512 features.

We also expect recent linux kernels (6.13+).

We always use memmap with LOCKED option so the memory locking limit on the OS should be enough for that.

The storage device should be an NVMe SSD and polling feature should be enabled in the kernel parameters with `nvme.poll_queues` set to a number larger than 0.

## Code Style

The code consists exclusively of `.cppm` and `.cc` files.

We don't link-in any standard library code or standard headers. There is no header usage in the codebase, no includes or `.h` files.

It is not allowed to use basically any C++ feature except methods and templates.

Struct field names should be prefixed with m_ in structs with methods.

Anything with a name ending with _ is private to the module that implements it.

All tests and fuzz tests have to be deterministic. This is very important because we use caching to avoid re-running tests that already passed.

### Error handling

_unsafe postfix is used to indicate functions that will `panic` at runtime if there is something wrong but won't do any checking when dev mode is disabled (`--no-dev-mode`).

Some functions like arithmetic and formatting functions indicate failure by a boolean flag (bool*) to make aggregating errors up to some point easier.

All other functions return error enums to indicate failure.

Some functions take a `Backtrace*` argument to fill the backtrace in case of an error. We later use this backtrace to print a stack trace of an error. This argument can be passed
as `nullptr` if the caller doesn't have access to a `Backtrace` or they just don't want a backtrace collected.

Every error return point should do `Backtrace::collect()` at the point of error return so we can clearly see where the error happened in the backtrace. This only applies if the error
was decided by the current function. If the error is just being propagated from a called function then there is no need to collect a backtrace as the called function already collected one.

### Memory Allocation

We allocate memory from the OS using mmap at program start and then don't do any system calls for memory management after that. The memory inside the program is managed using only arena allocation.

### SIMD

Since we don't include standard headers, we use clang builtins for doing SIMD.

Vector types are defined in `zx:vector`.

Tipically you can replace an immintrin function with an operator (+, -, ..) or a __builtin_ia32* intrinsic.

To find how to immitate an immintrin function, go to https://github.com/llvm/llvm-project/blob/main/clang/lib/Headers/immintrin.h and find the header file that corresponds to the feature set of
the immintrin you want to use. For example `avxintrin.h` contains intrinsics that implement the `AVX` features. Then you can check the definition of the immintrin function there to find how it is implemented.
You will want to do w/e that function is doing directly in this codebase.

We generally don't want to implement the immintrin functions in this codebase since there is a huge number of them and they are not particularly useful to have in terms of abstraction.

