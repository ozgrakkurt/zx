import zx;

using namespace zx;

int main(void) {
    Backtrace err_bt = Backtrace::init();

    Recorder recorder;
    testing::assert_with_trace(Recorder::InitStatus::Ok ==
                                   Recorder::init(&err_bt, &recorder),
                               &err_bt);

    recorder.start();

    u64 const arena_len = 4 * 1024 * 1024;
    u8 *arena = nullptr;
    testing::assert_with_trace(
        sys::MMapStatus::Ok == sys::mmap(&err_bt, nullptr, arena_len,
                                         sys::PROT_READ | sys::PROT_WRITE,
                                         sys::MAP_PRIVATE | sys::MAP_ANONYMOUS,
                                         -1, 0, &arena),
        &err_bt);

    u64 sum = 0;
    u8 volatile *touch = arena;
    for (u64 i = 0; i < arena_len; i += 64) {
        touch[i] = (u8)i;
        sum += touch[i];
    }
    for (u64 i = 0; i < 200'000; ++i) {
        sum += i;
    }
    assert(sum != 0);

    recorder.stop();

    assert(recorder.m_cycles > 0);
    assert(recorder.m_instructions > 0);
    assert(recorder.m_branch_instructions > 0);
    assert(recorder.m_instructions >= recorder.m_branch_instructions);
    assert(recorder.m_branch_misses <= recorder.m_branch_instructions);
    assert(recorder.m_cache_misses > 0);
    assert(!recorder.m_hw_multiplexed);

    assert(recorder.m_page_faults > 0);
    assert(recorder.m_page_faults_minor > 0);
    assert(recorder.m_page_faults >= recorder.m_page_faults_minor);
    assert(recorder.m_page_faults >= recorder.m_page_faults_major);

    testing::assert_with_trace(
        sys::MMapStatus::Ok == sys::munmap(&err_bt, arena, arena_len), &err_bt);

    recorder.deinit(&err_bt);

    return 0;
}
