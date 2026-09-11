import zx;

using namespace zx;

int main() {
    sys::exit_group(0);
    // exit_group should "exit" so we don't reach this point and we return "69"
    // which means the test will fail with non-zero exit code.
    return 69;
}
