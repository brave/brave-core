use lockfree_object_pool::NoneObjectPool;

#[test]
fn test_none() {
    let pool = NoneObjectPool::<u32>::new(Default::default);
    for _ in 0..2 {
        let mut _v = pool.pull();
        assert!(*_v == 0);
        *_v += 1;
    }
}
