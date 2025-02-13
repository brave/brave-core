use lockfree_object_pool::SpinLockObjectPool;

#[macro_use]
mod test_generic;

fn make_pool() -> SpinLockObjectPool<u32> {
    SpinLockObjectPool::<u32>::new(Default::default, |v| {
        *v = 0;
    })
}

fn make_recycle_pool() -> SpinLockObjectPool<u32> {
    SpinLockObjectPool::<u32>::new(Default::default, |_v| {})
}

test_generic_01!(test_spin_lock_01, make_pool());
test_generic_02!(test_spin_lock_02, make_pool());
test_recycle_generic_01!(test_spin_lock_recycle_01, make_recycle_pool());
