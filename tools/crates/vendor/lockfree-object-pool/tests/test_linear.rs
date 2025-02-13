use lockfree_object_pool::LinearObjectPool;

#[macro_use]
mod test_generic;

fn make_pool() -> LinearObjectPool<u32> {
    LinearObjectPool::<u32>::new(Default::default, |v| {
        *v = 0;
    })
}

fn make_recycle_pool() -> LinearObjectPool<u32> {
    LinearObjectPool::<u32>::new(Default::default, |_v| {})
}

test_generic_01!(test_linear_01, make_pool());
test_generic_02!(test_linear_02, make_pool());
test_recycle_generic_01!(test_linear_recycle_01, make_recycle_pool());

#[test]
fn test_linear_03() {
    let pool = make_pool();

    let mut addrs = Vec::new();

    for _ in 0..10 {
        let mut v = pool.pull();
        assert_eq!(*v, 0);
        *v += 1;
        assert_eq!(*v, 1);
        let o = &mut *v;
        *o += 1;
        assert_eq!(*o, 2);
        let addr = format!("{:?}", o as *const u32);
        if !addrs.contains(&addr) {
            addrs.push(addr);
        }
        assert_eq!(*v, 2);
    }

    assert_eq!(addrs.len(), 1);
    for _ in 0..2 {
        let mut v = pool.pull();
        assert_eq!(*v, 0);
        *v += 1;
    }
}

#[test]
fn test_linear_04() {
    let pool = make_pool();

    let mut addrs = Vec::new();

    for _ in 0..10 {
        let mut v1 = pool.pull();
        let mut v2 = pool.pull();
        let addr1 = format!("{:?}", &mut *v1 as *const u32);
        let addr2 = format!("{:?}", &mut *v2 as *const u32);

        if !addrs.contains(&addr1) {
            addrs.push(addr1);
        }

        if !addrs.contains(&addr2) {
            addrs.push(addr2);
        }
    }

    assert_eq!(addrs.len(), 2);
}
