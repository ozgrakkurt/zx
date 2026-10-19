import zx;

using namespace zx;

struct Ctx {
    Alloc *alloc;
    Backtrace *err_bt;
};

using key_type = u16;
using val_type = i256;

struct KeyVal {
    val_type val;
    key_type key;
};

struct RefMap {
    SliceMut<KeyVal> m_entries;
    u32 m_offset;

    static RefMap init(Backtrace* err_bt, Alloc *alloc, u32 capacity) {
        SliceMut<KeyVal> entries;
        testing::assert_with_trace(AllocStatus::Ok == alloc->alloc<KeyVal>(err_bt, capacity, &entries), err_bt);
        return RefMap{
            .m_entries = entries,
            .m_offset = 0,
        };
    }

    void reset() {
        this->m_entries.memset(0xaa);
        this->m_offset = 0;
    }

    u32 len() const { return this->m_offset; }

    u32 capacity() const { return u64_to_u32_unsafe(this->m_entries.len()); }

    u32 capacity_left() const {
        return sub_unsafe_u32(this->capacity(), this->m_offset);
    }

    KeyVal select_random(Xoshiro256 *rand) const {
        assert(this->len() > 0);
        u32 const idx = mod_unsafe_u32((u32)rand->next(), this->len());
        return this->m_entries.read_unsafe(idx);
    }

    bool insert(KeyVal kv) {
        for (u32 i = 0; i < this->m_offset; i++) {
            KeyVal *entry = this->m_entries.offset_mut_unsafe(i);
            if (entry->key == kv.key) {
                entry->val = kv.val;
                return true;
            }
        }
        if (this->capacity_left() == 0) {
            return false;
        }
        this->m_entries.write_unsafe(this->m_offset, kv);
        this->m_offset = add_unsafe_u32(this->m_offset, 1);
        return true;
    }

    bool get(key_type key, KeyVal const **out) const {
        assert(out != nullptr);
        for (u32 i = 0; i < this->len(); i++) {
            KeyVal const *entry = this->m_entries.offset_unsafe(i);
            if (entry->key == key) {
                *out = entry;
                return true;
            }
        }
        *out = nullptr;
        return false;
    }

    bool get_mut(key_type key, KeyVal **out) const {
        assert(out != nullptr);
        for (u32 i = 0; i < this->len(); i++) {
            KeyVal *entry = this->m_entries.offset_mut_unsafe(i);
            if (entry->key == key) {
                *out = entry;
                return true;
            }
        }
        *out = nullptr;
        return false;
    }
};

static bool keys_eql(void const *ctx, KeyVal const *l, KeyVal const *r) {
    (void)ctx;
    return l->key == r->key;
}

static u64 make_hash(key_type key) {
    return rapidhash_v3::hash(Slice<u8>::init_from_ptr_unsafe(&key), 169);
}

using HTable = Hashtable<KeyVal, void const *, keys_eql>;

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    Ctx ctx = *(Ctx*)usr_ctx;

    Alloc *alloc = ctx.alloc;
    alloc->reset();

    Backtrace* err_bt = ctx.err_bt;

    constexpr u32 max_capacity = 128;
    u32 const capacity = u64_to_u32_unsafe(
        mod_unsafe_u64(fuzz_input->next(), max_capacity));

    constexpr u32 max_n_ops = max_capacity * 4;
    u32 const n_ops =
        u64_to_u32_unsafe(mod_unsafe_u64(fuzz_input->next(), max_n_ops));

    HTable table;
    testing::assert_with_trace(HashtableInitStatus::Ok == HTable::init(err_bt, alloc, capacity, &table), err_bt);

    RefMap map = RefMap::init(err_bt, alloc, capacity);

    Xoshiro256 val_rand = Xoshiro256::init(fuzz_input->next());

    for (u32 i = 0; i < n_ops; i++) {
        assert(table.capacity_left() == map.capacity_left());
        assert(table.capacity() == map.capacity());
        assert(table.n_occupied() == map.len());

        u8 const op =
            u64_to_u8_unsafe(mod_unsafe_u64(fuzz_input->next(), 4));
        u64 const seed = fuzz_input->next();
        Xoshiro256 key_rand = Xoshiro256::init(seed);
        switch (op) {
        // insert
        case 0: {
            KeyVal const kv = KeyVal{
                .key = key_rand.random<key_type>(),
                .val = val_rand.random<val_type>(),
            };
            u64 const hash = make_hash(kv.key);

            HashtableInsertStatus const hstatus =
                table.insert(hash, &kv, nullptr);
            if (HashtableInsertStatus::CreatedNewEntry == hstatus) {
                KeyVal const *kvp = nullptr;
                assert(!map.get(kv.key, &kvp));
                assert(map.insert(kv));
            } else if (HashtableInsertStatus::ModifiedExistingEntry ==
                       hstatus) {
                KeyVal const *kvp = nullptr;
                assert(map.get(kv.key, &kvp));
                assert(kvp != nullptr);
                assert(kvp->key == kv.key);
                assert(map.insert(kv));
            } else if (HashtableInsertStatus::OutOfCapacity == hstatus) {
                assert(!map.insert(kv));
            } else {
                panic();
            }

            if (hstatus != HashtableInsertStatus::OutOfCapacity) {
                KeyVal const *kvp = nullptr;
                assert(HashtableGetStatus::Found ==
                       table.get(hash, &kv, nullptr, &kvp));
                assert(kvp != nullptr);
                assert(kvp->key == kv.key && kvp->val == kv.val);
            } else {
                KeyVal const *kvp = nullptr;
                assert(HashtableGetStatus::NotFound ==
                       table.get(hash, &kv, nullptr, &kvp));
            }

            break;
        }
        // get_or_insert
        case 1: {
            KeyVal const kv = KeyVal{
                .key = key_rand.random<key_type>(),
                .val = val_rand.random<val_type>(),
            };
            u64 const hash = make_hash(kv.key);

            KeyVal const *kvp = nullptr;
            HashtableGetOrInsertStatus const hstatus =
                table.get_or_insert(hash, &kv, nullptr, &kvp);
            if (HashtableGetOrInsertStatus::CreatedNewEntry == hstatus) {
                KeyVal const *kvp2 = nullptr;
                assert(!map.get(kv.key, &kvp2));
                assert(map.insert(kv));
            } else if (HashtableGetOrInsertStatus::FoundExistingEntry ==
                       hstatus) {
                KeyVal const *kvp2 = nullptr;
                assert(map.get(kv.key, &kvp2));
                assert(kvp2 != nullptr);
                assert(kvp2->key == kv.key);
            } else if (HashtableGetOrInsertStatus::OutOfCapacity == hstatus) {
                assert(!map.insert(kv));
            } else {
                panic();
            }

            if (hstatus != HashtableGetOrInsertStatus::OutOfCapacity) {
                KeyVal const *kvp2 = nullptr;
                assert(HashtableGetStatus::Found ==
                       table.get(hash, &kv, nullptr, &kvp2));
                assert(kvp2 != nullptr);
                assert(kvp->key == kvp2->key && kvp->val == kvp2->val);
            } else {
                KeyVal const *kvp2 = nullptr;
                assert(HashtableGetStatus::NotFound ==
                       table.get(hash, &kv, nullptr, &kvp2));
            }

            break;
        }
        // rand get
        case 2: {
            KeyVal const kv = KeyVal{
                .key = key_rand.random<key_type>(),
                .val = val_rand.random<val_type>(),
            };
            u64 const hash = make_hash(kv.key);
            KeyVal const *kvp = nullptr;
            HashtableGetStatus const hstatus =
                table.get(hash, &kv, nullptr, &kvp);
            if (hstatus == HashtableGetStatus::Found) {
                assert(kvp != nullptr);
                KeyVal const *expected_kvp = nullptr;
                assert(map.get(kv.key, &expected_kvp));
                assert(expected_kvp != nullptr);
                assert(kvp->key == expected_kvp->key);
                assert(kvp->val == expected_kvp->val);
            } else if (hstatus == HashtableGetStatus::NotFound) {
                KeyVal const *expected_kvp = nullptr;
                assert(!map.get(kv.key, &expected_kvp));
            } else {
                panic();
            }
            break;
        }
        // rand select
        case 3: {
            if (map.len() == 0) {
                continue;
            }

            KeyVal const kv = map.select_random(&key_rand);
            u64 const hash = make_hash(kv.key);
            KeyVal const *kvp_im = nullptr;
            assert(HashtableGetStatus::Found ==
                   table.get(hash, &kv, nullptr, &kvp_im));

            assert(kvp_im != nullptr);

            assert(kvp_im->key == kv.key && kvp_im->val == kv.val);
            break;
        }
        default: {
            panic();
            break;
        }
        }
    }
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    constexpr u64 MEM_CAP = u64{1} << 28;
    u8* mem = nullptr;
    testing::assert_with_trace(page_alloc::AllocStatus::Ok == page_alloc::alloc_4kb(&err_bt, MEM_CAP, &mem), &err_bt);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, MEM_CAP));

    constexpr u64 FUZZ_MEM_CAP = u64{1} << 27;
    static_assert(FUZZ_MEM_CAP < MEM_CAP);

    SliceMut<u8> fuzz_mem;
    testing::assert_with_trace(AllocStatus::Ok == alloc.alloc<u8>(&err_bt, FUZZ_MEM_CAP, &fuzz_mem), &err_bt);

    Alloc fuzz_alloc = Alloc::init(fuzz_mem);

    u64 const op_mem_cap = sub_unsafe_u64(MEM_CAP, FUZZ_MEM_CAP);
    SliceMut<u8> op_mem;
    testing::assert_with_trace(AllocStatus::Ok == alloc.alloc<u8>(&err_bt, op_mem_cap, &op_mem), &err_bt);

    Alloc op_alloc = Alloc::init(op_mem);
    Ctx ctx = Ctx{
        .alloc = &op_alloc,
        .err_bt = &err_bt,
    };
    constexpr u64 n_iter = u64{1} << 12;
    fuzz(FuzzParams{.alloc = &fuzz_alloc,
                    .usr_ctx = (void *)&ctx,
                    .fuzz_one = fuzz_one,
                    .iter_limit = n_iter});
    return 0;
}
