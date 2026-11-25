import zx;

using namespace zx;

static_assert(sizeof(io_uring::IoUringSQE) == 64);
static_assert(sizeof(io_uring::IoUringCQE) == 16);
static_assert(sizeof(io_uring::IoUringParams) == 120);

namespace {

constexpr u32 RING_ENTRIES = 8;
constexpr u32 PROBE_OPS = (u32)io_uring::IoUringOp::WriteFixed + 1;
constexpr u64 USER_DATA_BASE = 0x1055;

struct IOVec {
    u8 *base;
    u64 len;
};

struct Ring {
    i32 fd;
    u8 *ring_mem;
    u64 ring_mem_sz;
    u8 *cq_mem;
    u64 cq_mem_sz;
    u8 *sqes_mem;
    u64 sqes_mem_sz;
    u32 *sq_tail;
    u32 *sq_mask;
    u32 *sq_array;
    u32 *cq_head;
    u32 *cq_tail;
    u32 *cq_mask;
    io_uring::IoUringCQE *cqes;
    io_uring::IoUringSQE *sqes;
};

static u32 load_u32(u32 const *ptr) { return *(u32 volatile const *)ptr; }

static void store_u32(u32 *ptr, u32 val) { *(u32 volatile *)ptr = val; }

static Ring ring_init(Backtrace *err_bt) {
    io_uring::IoUringParams params;
    __builtin_memset(&params, 0, sizeof(params));

    i32 fd = -1;
    testing::assert_with_trace(
        sys::IoUringSetupStatus::Ok ==
            sys::io_uring_setup(err_bt, RING_ENTRIES, &params, &fd),
        err_bt);
    assert(fd >= 0);
    assert(params.sq_entries >= 1);
    assert(params.cq_entries >= 1);

    u64 const sq_ring_sz =
        params.sq_off.array + params.sq_entries * sizeof(u32);
    u64 const cq_ring_sz =
        params.cq_off.cqes + params.cq_entries * sizeof(io_uring::IoUringCQE);

    bool const single_mmap =
        (params.features & io_uring::IORING_FEAT_SINGLE_MMAP) != 0;

    u64 ring_sz = sq_ring_sz;
    if (single_mmap && cq_ring_sz > ring_sz) {
        ring_sz = cq_ring_sz;
    }

    u8 *ring_mem = nullptr;
    testing::assert_with_trace(
        sys::MMapStatus::Ok == sys::mmap(err_bt, nullptr, ring_sz,
                                         sys::PROT_READ | sys::PROT_WRITE,
                                         sys::MAP_SHARED | sys::MAP_POPULATE,
                                         fd, (i64)io_uring::IORING_OFF_SQ_RING,
                                         &ring_mem),
        err_bt);
    assert(ring_mem != nullptr);

    u8 *cq_mem = ring_mem;
    u64 cq_mem_sz = 0;
    if (!single_mmap) {
        cq_mem_sz = cq_ring_sz;
        testing::assert_with_trace(
            sys::MMapStatus::Ok ==
                sys::mmap(err_bt, nullptr, cq_ring_sz,
                          sys::PROT_READ | sys::PROT_WRITE,
                          sys::MAP_SHARED | sys::MAP_POPULATE, fd,
                          (i64)io_uring::IORING_OFF_CQ_RING, &cq_mem),
            err_bt);
        assert(cq_mem != nullptr);
    }

    u64 const sqes_sz = params.sq_entries * sizeof(io_uring::IoUringSQE);
    u8 *sqes_mem = nullptr;
    testing::assert_with_trace(
        sys::MMapStatus::Ok == sys::mmap(err_bt, nullptr, sqes_sz,
                                         sys::PROT_READ | sys::PROT_WRITE,
                                         sys::MAP_SHARED | sys::MAP_POPULATE,
                                         fd, (i64)io_uring::IORING_OFF_SQES,
                                         &sqes_mem),
        err_bt);
    assert(sqes_mem != nullptr);

    Ring ring;
    __builtin_memset(&ring, 0, sizeof(ring));
    ring.fd = fd;
    ring.ring_mem = ring_mem;
    ring.ring_mem_sz = ring_sz;
    ring.cq_mem = cq_mem;
    ring.cq_mem_sz = cq_mem_sz;
    ring.sqes_mem = sqes_mem;
    ring.sqes_mem_sz = sqes_sz;
    ring.sq_tail = (u32 *)(ring_mem + params.sq_off.tail);
    ring.sq_mask = (u32 *)(ring_mem + params.sq_off.ring_mask);
    ring.sq_array = (u32 *)(ring_mem + params.sq_off.array);
    ring.cq_head = (u32 *)(cq_mem + params.cq_off.head);
    ring.cq_tail = (u32 *)(cq_mem + params.cq_off.tail);
    ring.cq_mask = (u32 *)(cq_mem + params.cq_off.ring_mask);
    ring.cqes = (io_uring::IoUringCQE *)(cq_mem + params.cq_off.cqes);
    ring.sqes = (io_uring::IoUringSQE *)sqes_mem;

    assert(*ring.sq_mask == params.sq_entries - 1);
    assert(*ring.cq_mask == params.cq_entries - 1);
    assert(load_u32(ring.sq_tail) == 0);
    assert(load_u32(ring.cq_head) == 0);
    assert(load_u32(ring.cq_tail) == 0);

    return ring;
}

static void ring_deinit(Backtrace *err_bt, Ring *ring) {
    if (ring->cq_mem != ring->ring_mem) {
        testing::assert_with_trace(
            sys::MMapStatus::Ok ==
                sys::munmap(err_bt, ring->cq_mem, ring->cq_mem_sz),
            err_bt);
    }

    testing::assert_with_trace(
        sys::MMapStatus::Ok ==
            sys::munmap(err_bt, ring->sqes_mem, ring->sqes_mem_sz),
        err_bt);
    testing::assert_with_trace(
        sys::MMapStatus::Ok ==
            sys::munmap(err_bt, ring->ring_mem, ring->ring_mem_sz),
        err_bt);
    testing::assert_with_trace(
        sys::CloseStatus::Ok == sys::close(err_bt, ring->fd), err_bt);
}

static io_uring::IoUringSQE *ring_get_sqe(Ring *ring) {
    u32 const tail = load_u32(ring->sq_tail);
    u32 const index = tail & *ring->sq_mask;
    io_uring::IoUringSQE *sqe = &ring->sqes[index];
    __builtin_memset(sqe, 0, sizeof(*sqe));
    ring->sq_array[index] = index;
    store_u32(ring->sq_tail, tail + 1);
    return sqe;
}

static void ring_submit(Backtrace *err_bt, Ring *ring, u32 n) {
    u32 n_entered = 0;
    testing::assert_with_trace(sys::IoUringEnterStatus::Ok ==
                                   sys::io_uring_enter(err_bt, ring->fd, n, 0,
                                                       0, nullptr, 0,
                                                       &n_entered),
                               err_bt);
    assert(n_entered == n);
}

static void ring_wait(Backtrace *err_bt, Ring *ring, u32 n) {
    u32 n_entered = 0;
    testing::assert_with_trace(
        sys::IoUringEnterStatus::Ok ==
            sys::io_uring_enter(err_bt, ring->fd, 0, n,
                                io_uring::IORING_ENTER_GETEVENTS, nullptr, 0,
                                &n_entered),
        err_bt);
    assert(n_entered <= n);
}

static void ring_reap(Ring *ring, u32 n, u64 user_data_base, i32 expected_res) {
    u32 head = load_u32(ring->cq_head);
    u32 const tail = load_u32(ring->cq_tail);
    assert(tail - head == n);

    u32 seen = 0;
    for (u32 i = 0; i < n; i++) {
        io_uring::IoUringCQE const *cqe = &ring->cqes[head & *ring->cq_mask];
        assert(cqe->user_data >= user_data_base);
        assert(cqe->user_data < user_data_base + n);
        assert(cqe->res == expected_res);

        u32 const bit = (u32)1 << (u32)(cqe->user_data - user_data_base);
        assert((seen & bit) == 0);
        seen |= bit;
        head++;
    }

    assert(seen == ((u32)1 << n) - 1);
    store_u32(ring->cq_head, head);
}

static void test_nop(Backtrace *err_bt, Ring *ring) {
    constexpr u32 n_ops = 4;

    for (u32 i = 0; i < n_ops; i++) {
        io_uring::IoUringSQE *sqe = ring_get_sqe(ring);
        sqe->opcode = (u8)io_uring::IoUringOp::Nop;
        sqe->user_data = USER_DATA_BASE + i;
    }

    ring_submit(err_bt, ring, n_ops);
    ring_wait(err_bt, ring, n_ops);
    ring_reap(ring, n_ops, USER_DATA_BASE, 0);
}

static void test_probe_and_fixed_io(Backtrace *err_bt, Ring *ring) {
    constexpr u64 probe_sz = sizeof(io_uring::IoUringProbe) +
                             PROBE_OPS * sizeof(io_uring::IoUringProbeOp);
    alignas(8) u8 probe_mem[probe_sz];
    __builtin_memset(probe_mem, 0, sizeof(probe_mem));

    testing::assert_with_trace(
        sys::IoUringRegisterStatus::Ok ==
            sys::io_uring_register(
                err_bt, ring->fd,
                (u32)io_uring::IoUringRegisterOp::RegisterProbe, probe_mem,
                PROBE_OPS),
        err_bt);

    io_uring::IoUringProbe const *probe =
        (io_uring::IoUringProbe const *)probe_mem;
    assert(probe->ops_len == PROBE_OPS);
    assert((probe->ops[(u32)io_uring::IoUringOp::Nop].flags &
            io_uring::IO_URING_OP_SUPPORTED) != 0);
    assert((probe->ops[(u32)io_uring::IoUringOp::WriteFixed].flags &
            io_uring::IO_URING_OP_SUPPORTED) != 0);

    constexpr u64 data_len = 64;
    alignas(64) u8 write_data[data_len];
    alignas(64) u8 read_data[data_len];
    for (u32 i = 0; i < data_len; i++) {
        write_data[i] = (u8)(i * 7 + 3);
    }
    __builtin_memset(read_data, 0, sizeof(read_data));

    IOVec iovecs[2];
    iovecs[0].base = write_data;
    iovecs[0].len = data_len;
    iovecs[1].base = read_data;
    iovecs[1].len = data_len;

    testing::assert_with_trace(
        sys::IoUringRegisterStatus::Ok ==
            sys::io_uring_register(
                err_bt, ring->fd,
                (u32)io_uring::IoUringRegisterOp::RegisterBuffers, iovecs, 2),
        err_bt);

    i32 file_fd = -1;
    testing::assert_with_trace(
        tmp_file::OpenStatus::Ok == tmp_file::open(err_bt, &file_fd), err_bt);
    assert(file_fd >= 0);

    i32 files[1];
    files[0] = file_fd;
    testing::assert_with_trace(
        sys::IoUringRegisterStatus::Ok ==
            sys::io_uring_register(
                err_bt, ring->fd,
                (u32)io_uring::IoUringRegisterOp::RegisterFiles, files, 1),
        err_bt);

    // Write into the temp file and read it back, using a registered file and
    // registered buffers. Linking the pair makes the read wait for the write.
    io_uring::IoUringSQE *write_sqe = ring_get_sqe(ring);
    write_sqe->opcode = (u8)io_uring::IoUringOp::WriteFixed;
    write_sqe->flags = io_uring::IOSQE_FIXED_FILE | io_uring::IOSQE_IO_LINK;
    write_sqe->fd = 0;
    write_sqe->off = 0;
    write_sqe->addr = (u64)write_data;
    write_sqe->len = data_len;
    write_sqe->buf_index = 0;
    write_sqe->user_data = USER_DATA_BASE + 10;

    io_uring::IoUringSQE *read_sqe = ring_get_sqe(ring);
    read_sqe->opcode = (u8)io_uring::IoUringOp::ReadFixed;
    read_sqe->flags = io_uring::IOSQE_FIXED_FILE;
    read_sqe->fd = 0;
    read_sqe->off = 0;
    read_sqe->addr = (u64)read_data;
    read_sqe->len = data_len;
    read_sqe->buf_index = 1;
    read_sqe->user_data = USER_DATA_BASE + 11;

    ring_submit(err_bt, ring, 2);
    ring_wait(err_bt, ring, 2);
    ring_reap(ring, 2, USER_DATA_BASE + 10, (i32)data_len);

    assert(__builtin_memcmp(write_data, read_data, data_len) == 0);

    testing::assert_with_trace(
        sys::IoUringRegisterStatus::Ok ==
            sys::io_uring_register(
                err_bt, ring->fd,
                (u32)io_uring::IoUringRegisterOp::UnregisterFiles, nullptr, 0),
        err_bt);
    testing::assert_with_trace(
        sys::IoUringRegisterStatus::Ok ==
            sys::io_uring_register(
                err_bt, ring->fd,
                (u32)io_uring::IoUringRegisterOp::UnregisterBuffers, nullptr,
                0),
        err_bt);
    testing::assert_with_trace(
        sys::CloseStatus::Ok == sys::close(err_bt, file_fd), err_bt);
}

} // namespace

int main() {
    Backtrace err_bt = Backtrace::init();

    Ring ring = ring_init(&err_bt);
    test_nop(&err_bt, &ring);
    test_probe_and_fixed_io(&err_bt, &ring);
    ring_deinit(&err_bt, &ring);

    return 0;
}
