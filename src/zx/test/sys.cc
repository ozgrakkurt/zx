import zx;

using namespace zx;

static void test_mmap() {
    // mmap munmap mprotect
}

static void test_file() {
    // open close read write statx
}

static void test_perf_event_open() {}

static void test_io_uring() {
    // setup enter register
}

int main() {
    test_mmap();
    test_file();
    test_perf_event_open();
    test_io_uring();
}
