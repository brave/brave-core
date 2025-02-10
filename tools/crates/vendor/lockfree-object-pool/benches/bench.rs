#[macro_use]
extern crate criterion;

use criterion::Criterion;

mod bench_tools;

#[macro_use]
mod bench_generic;

struct Vec4096 {
    _data: Vec<u8>,
}

impl Default for Vec4096 {
    fn default() -> Self {
        Self {
            _data: Vec::with_capacity(16 * 1024),
        }
    }
}

impl sharded_slab::Clear for Vec4096 {
    fn clear(&mut self) {}
}

fn bench_alloc(c: &mut Criterion) {
    let mut group = c.benchmark_group("allocation");
    bench_alloc_impl_!(
        group,
        "none object poll",
        lockfree_object_pool::NoneObjectPool::new(|| Vec::<u8>::with_capacity(16 * 1024)),
        1
    );
    bench_alloc_impl_!(
        group,
        "mutex object poll",
        lockfree_object_pool::MutexObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_alloc_impl_!(
        group,
        "spin_lock object poll",
        lockfree_object_pool::SpinLockObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_alloc_impl_!(
        group,
        "linear object poll",
        lockfree_object_pool::LinearObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_alloc_impl_!(
        group,
        "crate 'object-pool'",
        object_pool::Pool::<Vec<u8>>::new(32, || Vec::with_capacity(4096)),
        2
    );
    bench_alloc_impl_!(
        group,
        "crate 'sharded-slab'",
        sharded_slab::Pool::<Vec4096>::new(),
        3
    );
    group.finish();
}

fn bench_free(c: &mut Criterion) {
    let mut group = c.benchmark_group("free");
    bench_free_impl_!(
        group,
        "none object poll",
        lockfree_object_pool::NoneObjectPool::new(|| Vec::<u8>::with_capacity(16 * 1024)),
        1
    );
    bench_free_impl_!(
        group,
        "mutex object poll",
        lockfree_object_pool::MutexObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_free_impl_!(
        group,
        "spin_lock object poll",
        lockfree_object_pool::SpinLockObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_free_impl_!(
        group,
        "linear object poll",
        lockfree_object_pool::LinearObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_free_impl_!(
        group,
        "crate 'object-pool'",
        object_pool::Pool::<Vec<u8>>::new(32, || Vec::with_capacity(4096)),
        2
    );
    bench_free_impl_!(
        group,
        "crate 'sharded-slab'",
        sharded_slab::Pool::<Vec4096>::new(),
        3
    );
    group.finish();
}

fn bench_reuse(c: &mut Criterion) {
    const VEC_SIZE: usize = 16384;
    const BATCH_SIZE: usize = 8192;

    let mut group = c.benchmark_group("reuse");
    group.bench_function("none object poll", |b| {
        b.iter_batched(
            || {
                (
                    lockfree_object_pool::NoneObjectPool::new(|| {
                        Vec::<u8>::with_capacity(16 * 1024)
                    }),
                    Vec::with_capacity(VEC_SIZE),
                )
            },
            |(pool, mut vec)| {
                for index in 0..BATCH_SIZE {
                    vec.insert(index, criterion::black_box(pool.pull()));
                }
            },
            criterion::BatchSize::SmallInput,
        );
    });

    group.bench_function("mutex object poll", |b| {
        b.iter_batched(
            || {
                let pool = lockfree_object_pool::MutexObjectPool::<Vec<u8>>::new(
                    || Vec::with_capacity(16 * 1024),
                    |_v| {},
                );
                let v: Vec<_> = (0..VEC_SIZE).map(|_| pool.pull()).collect();
                drop(v);
                (pool, Vec::with_capacity(VEC_SIZE))
            },
            |(pool, mut vec)| {
                for index in 0..BATCH_SIZE {
                    vec.insert(index, criterion::black_box(pool.pull()));
                }
            },
            criterion::BatchSize::SmallInput,
        );
    });

    group.bench_function("spin_lock object poll", |b| {
        b.iter_batched(
            || {
                let pool = lockfree_object_pool::SpinLockObjectPool::<Vec<u8>>::new(
                    || Vec::with_capacity(16 * 1024),
                    |_v| {},
                );
                let v: Vec<_> = (0..VEC_SIZE).map(|_| pool.pull()).collect();
                drop(v);
                (pool, Vec::with_capacity(VEC_SIZE))
            },
            |(pool, mut vec)| {
                for index in 0..BATCH_SIZE {
                    vec.insert(index, criterion::black_box(pool.pull()));
                }
            },
            criterion::BatchSize::SmallInput,
        );
    });

    group.bench_function("linear object poll", |b| {
        b.iter_batched(
            || {
                let pool = lockfree_object_pool::LinearObjectPool::new(
                    || Vec::<u8>::with_capacity(16 * 1024),
                    |_v| {},
                );
                let v: Vec<_> = (0..VEC_SIZE).map(|_| pool.pull()).collect();
                drop(v);
                (pool, Vec::with_capacity(VEC_SIZE))
            },
            |(pool, mut vec)| {
                for index in 0..BATCH_SIZE {
                    vec.insert(index, criterion::black_box(pool.pull()));
                }
            },
            criterion::BatchSize::SmallInput,
        );
    });

    group.bench_function("crate 'object-pool'", |b| {
        b.iter_batched(
            || {
                let pool = object_pool::Pool::new(VEC_SIZE, || Vec::<u8>::with_capacity(16 * 1024));
                let v: Vec<_> = (0..VEC_SIZE).map(|_| pool.try_pull().unwrap()).collect();
                drop(v);
                (pool, Vec::with_capacity(VEC_SIZE))
            },
            |(pool, mut vec)| {
                for index in 0..BATCH_SIZE {
                    vec.insert(index, criterion::black_box(pool.try_pull().unwrap()));
                }
            },
            criterion::BatchSize::SmallInput,
        );
    });
}

fn bench_alloc_mt(c: &mut Criterion) {
    let mut group = c.benchmark_group("multi thread allocation");
    bench_alloc_mt_impl_!(
        group,
        "none object poll",
        lockfree_object_pool::NoneObjectPool::new(|| Vec::<u8>::with_capacity(16 * 1024)),
        1
    );
    bench_alloc_mt_impl_!(
        group,
        "mutex object poll",
        lockfree_object_pool::MutexObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_alloc_mt_impl_!(
        group,
        "spin_lock object poll",
        lockfree_object_pool::SpinLockObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_alloc_mt_impl_!(
        group,
        "linear object poll",
        lockfree_object_pool::LinearObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_alloc_mt_impl_!(
        group,
        "crate 'object-pool'",
        object_pool::Pool::<Vec<u8>>::new(32, || Vec::with_capacity(4096)),
        2
    );
    bench_alloc_mt_impl_!(
        group,
        "crate 'sharded-slab'",
        sharded_slab::Pool::<Vec4096>::new(),
        3
    );
    group.finish();
}

fn bench_free_mt(c: &mut Criterion) {
    let mut group = c.benchmark_group("multi thread free");
    bench_free_mt_impl_!(
        group,
        "none object poll",
        lockfree_object_pool::NoneObjectPool::new(|| Vec::<u8>::with_capacity(16 * 1024)),
        1
    );
    bench_free_mt_impl_!(
        group,
        "mutex object poll",
        lockfree_object_pool::MutexObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_free_mt_impl_!(
        group,
        "spin_lock object poll",
        lockfree_object_pool::SpinLockObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_free_mt_impl_!(
        group,
        "linear object poll",
        lockfree_object_pool::LinearObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        1
    );
    bench_free_mt_impl_!(
        group,
        "crate 'object-pool'",
        object_pool::Pool::<Vec<u8>>::new(32, || Vec::with_capacity(4096)),
        2
    );
    bench_free_mt_impl_!(
        group,
        "crate 'sharded-slab'",
        sharded_slab::Pool::<Vec4096>::new(),
        3
    );
    group.finish();
}

fn bench_forward_multi_thread(c: &mut Criterion, nb_writter: usize, nb_readder: usize) {
    let mut group = c.benchmark_group(format!(
        "forward msg from pull (nb_writter:{} nb_readder:{})",
        nb_writter, nb_readder
    ));
    bench_forward_impl_!(
        group,
        "none object poll",
        lockfree_object_pool::NoneObjectPool::new(|| Vec::<u8>::with_capacity(16 * 1024)),
        nb_readder,
        nb_writter,
        1
    );
    bench_forward_impl_!(
        group,
        "mutex object poll",
        lockfree_object_pool::MutexObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        nb_readder,
        nb_writter,
        1
    );
    bench_forward_impl_!(
        group,
        "spin_lock object poll",
        lockfree_object_pool::SpinLockObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        nb_readder,
        nb_writter,
        1
    );
    bench_forward_impl_!(
        group,
        "linear object poll",
        lockfree_object_pool::LinearObjectPool::<Vec<u8>>::new(
            || Vec::with_capacity(16 * 1024),
            |_v| {}
        ),
        nb_readder,
        nb_writter,
        1
    );
    bench_forward_impl_!(
        group,
        "crate 'sharded-slab'",
        sharded_slab::Pool::<Vec4096>::new(),
        nb_readder,
        nb_writter,
        3
    );
    group.finish();
}

fn bench_forward_multi_thread55(c: &mut Criterion) {
    bench_forward_multi_thread(c, 5, 5);
}

fn bench_forward_multi_thread15(c: &mut Criterion) {
    bench_forward_multi_thread(c, 1, 5);
}

fn bench_forward_multi_thread51(c: &mut Criterion) {
    bench_forward_multi_thread(c, 5, 1);
}

fn bench_forward_multi_thread11(c: &mut Criterion) {
    bench_forward_multi_thread(c, 1, 1);
}

criterion_group!(
    forward_multi_thread,
    bench_forward_multi_thread55,
    bench_forward_multi_thread15,
    bench_forward_multi_thread51,
    bench_forward_multi_thread11
);
criterion_group!(multi_thread, bench_alloc_mt, bench_free_mt);
criterion_group!(mono_thread, bench_alloc, bench_free, bench_reuse);
criterion_main!(mono_thread, multi_thread, forward_multi_thread);
