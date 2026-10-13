import zx;

using namespace zx;

using key_type = u256;

static bool keys_eql(void const *ctx, key_type const *a, key_type const *b) {
    (void)ctx;
    return *a == *b;
}

static u64 hash_key(key_type key) {
    return rapidhash_v3::hash(Slice<u8>::init_from_ptr_unsafe(&key), 0);
}

using HTable = Hashtable<key_type, void const *, keys_eql>;

static void smoke_test_inner(HTable *table, u32 len) {
    assert(table->capacity_left() == len);

    for (u32 i = 0; i < div_unsafe_u32(len, 2); i++) {
        key_type const key = i;
        u64 const hash = hash_key(key);
        assert(HashtableInsertStatus::CreatedNewEntry ==
               table->insert(hash, &key, nullptr));
        assert(HashtableInsertStatus::ModifiedExistingEntry ==
               table->insert(hash, &key, nullptr));
    }

    for (u32 i = 0; i < div_unsafe_u32(len, 2); i++) {
        key_type const key = i;
        u64 const hash = hash_key(key);

        key_type const *entry = nullptr;
        assert(HashtableGetStatus::Found ==
               table->get(hash, &key, nullptr, &entry));
        assert(entry != nullptr);
        assert(*entry == key);
    }

    for (u32 i = div_unsafe_u32(len, 2); i < len; i++) {
        key_type const key = i;
        u64 const hash = hash_key(key);

        key_type const *entry = nullptr;
        assert(HashtableGetStatus::NotFound ==
               table->get(hash, &key, nullptr, &entry));
        assert(entry == nullptr);
    }

    for (u32 i = 0; i < div_unsafe_u32(len, 2); i++) {
        key_type const key = i;
        u64 const hash = hash_key(key);

        key_type const *entry = nullptr;
        assert(HashtableGetOrInsertStatus::FoundExistingEntry ==
               table->get_or_insert(hash, &key, nullptr, &entry));
        assert(table->capacity_left() ==
               sub_unsafe_u32(len, div_unsafe_u32(len, 2)));
        assert(entry != nullptr);
        assert(*entry == key);
    }

    for (u32 i = div_unsafe_u32(len, 2); i < len; i++) {
        key_type const key = i;
        u64 const hash = hash_key(key);

        key_type const *entry = nullptr;
        assert(HashtableGetOrInsertStatus::CreatedNewEntry ==
               table->get_or_insert(hash, &key, nullptr, &entry));
        assert(table->capacity_left() ==
               sub_unsafe_u32(len, add_unsafe_u32(i, 1)));
        assert(entry != nullptr);
        assert(*entry == key);
    }
}

static void smoke_test(Alloc *alloc, u32 len) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    HTable table;
    testing::assert_with_trace(HashtableInitStatus::Ok ==
                                   HTable::init(&err_ctx, alloc, len, &table),
                               &err_ctx);

    smoke_test_inner(&table, len);
    table.reset();
    smoke_test_inner(&table, len);
}

static void nocap_test(Alloc *alloc) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    HTable table;
    testing::assert_with_trace(HashtableInitStatus::Ok ==
                                   HTable::init(&err_ctx, alloc, 0, &table),
                               &err_ctx);
    assert(table.capacity_left() == 0);

    key_type const key = 100;
    u64 const hash = hash_key(key);

    key_type const *entry_p = nullptr;

    assert(HashtableGetStatus::NotFound ==
           table.get(hash, &key, nullptr, &entry_p));
    assert(entry_p == nullptr);
    assert(HashtableInsertStatus::OutOfCapacity ==
           table.insert(hash, &key, nullptr));

    assert(HashtableGetOrInsertStatus::OutOfCapacity ==
           table.get_or_insert(hash, &key, nullptr, &entry_p));
    assert(entry_p == nullptr);

    assert(HashtableGetStatus::NotFound ==
           table.get(hash, &key, nullptr, &entry_p));
    assert(entry_p == nullptr);
}

static void noalloc_test() {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    Alloc noalloc = Alloc::init(SliceMut<u8>::init_dangling());

    HTable table;
    assert(HashtableInitStatus::OutOfMemory ==
           HTable::init(&err_ctx, &noalloc, 0, &table));
    assert(HashtableInitStatus::OutOfMemory ==
           HTable::init(&err_ctx, &noalloc, 1, &table));
    assert(HashtableInitStatus::OutOfMemory ==
           HTable::init(&err_ctx, &noalloc, 10, &table));
    assert(HashtableInitStatus::OutOfMemory ==
           HTable::init(&err_ctx, &noalloc, 1000000, &table));
    assert(HashtableInitStatus::IntegerOverflow ==
           HTable::init(&err_ctx, &noalloc, U32_MAX, &table));
}

int main(void) {
    constexpr u64 MEMORY_SIZE = 1 << 18;

    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    u8 *mem;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_ctx, MEMORY_SIZE, &mem),
        &err_ctx);

    noalloc_test();

    SliceMut<u8> mem_slice = SliceMut<u8>::init_unsafe(mem, MEMORY_SIZE);

    Alloc alloc = Alloc::init(mem_slice);

    nocap_test(&alloc);
    alloc.reset();

    constexpr u32 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                               65, 63, 64,   128,  127, 129,  3169};
    constexpr u32 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

    for (u32 i = 0; i < n_lengths; i++) {
        u32 const len = lengths[i];
        alloc.reset();
        smoke_test(&alloc, len);
    }

    testing::assert_with_trace(page_alloc::FreeStatus::Ok ==
                                   page_alloc::free(&err_ctx, mem, MEMORY_SIZE),
                               &err_ctx);

    return 0;
}
