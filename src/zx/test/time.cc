import zx;

using namespace zx;

int main(void) {
    TimeInfo info;
    TimeInfo::InitStatus const status = TimeInfo::init(&info);
    assert(status == TimeInfo::InitStatus::Ok);

    Timer timer = Timer::init(info);

    u64 const l1 = timer.lap();
    u64 const l2 = timer.lap();

    (void)l1;
    (void)l2;

    return 0;
}
