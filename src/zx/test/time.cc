import zx;

using namespace zx;

int main(void) {
    Backtrace err_bt = Backtrace::init();

    TimeInfo info;
    TimeInfo::InitStatus const status = TimeInfo::init(&err_bt, &info);
    testing::assert_with_trace(status == TimeInfo::InitStatus::Ok, &err_bt);

    Timer timer = Timer::init(info);

    u64 const l1 = timer.lap();
    u64 const l2 = timer.lap();

    (void)l1;
    (void)l2;

    return 0;
}
