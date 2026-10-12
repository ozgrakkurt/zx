import zx;

using namespace zx;

int main(void) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    TimeInfo info;
    TimeInfo::InitStatus const status = TimeInfo::init(&err_ctx, &info);
    testing::assert_with_trace(status == TimeInfo::InitStatus::Ok, &err_ctx);

    Timer timer = Timer::init(info);

    u64 const l1 = timer.lap();
    u64 const l2 = timer.lap();

    (void)l1;
    (void)l2;

    return 0;
}
