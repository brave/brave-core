use crate::raw_rwlock::RawRwLock;

/// A reader-writer lock.
///
/// This type of lock allows a number of readers or at most one writer at any
/// point in time. The write portion of this lock typically allows modification
/// of the underlying data (exclusive access) and the read portion of this lock
/// typically allows for read-only access (shared access).
///
/// This lock uses a task-fair locking policy which generally gives waiting
/// writers priority over new readers. This means that readers trying to acquire
/// the lock will block even if the lock is unlocked when there are writers
/// waiting to acquire the lock. Because of this, attempts to recursively acquire
/// a read lock within a single thread may result in a deadlock.
///
/// Use [`RecursiveRwLock`](crate::RecursiveRwLock) instead if recursive read
/// locking is required.
///
/// The type parameter `T` represents the data that this lock protects. It is
/// required that `T` satisfies `Send` to be shared across threads and `Sync` to
/// allow concurrent access through readers. The RAII guards returned from the
/// locking methods implement `Deref` (and `DerefMut` for the `write` methods)
/// to allow access to the contents of the lock.
///
/// # Fairness
///
/// This rwlock uses unfair unlocking by default, which allows newly arriving
/// threads to acquire the lock before a waiting thread and generally improves
/// throughput. This can starve waiting threads.
///
/// Fair unlocking can be requested explicitly by calling
/// `RwLockReadGuard::unlock_fair` or `RwLockWriteGuard::unlock_fair` instead of
/// simply dropping the guard. A fair unlock gives waiting threads priority over
/// newly arriving threads.
///
/// # Differences from the standard library `RwLock`
///
/// - Task-fair locking policy instead of an unspecified platform default.
/// - No poisoning, the lock is released normally on panic.
/// - Only requires one word of lock state.
/// - Supports upgradable read locks and atomic upgrades to write locks.
/// - Supports locking with a timeout.
/// - Allows raw locking & unlocking without a guard.
/// - Supports explicit fair unlocking by calling
///   `RwLockReadGuard::unlock_fair` and `RwLockWriteGuard::unlock_fair`.
///
/// # Examples
///
/// ```
/// use parking_lot::RwLock;
///
/// let lock = RwLock::new(5);
///
/// // Multiple readers can access the data concurrently.
/// std::thread::scope(|scope| {
///     for _ in 0..3 {
///         scope.spawn(|| {
///             let reader = lock.read();
///             assert_eq!(*reader, 5);
///         });
///     }
/// });
///
/// // All reader threads have finished. Only one write lock may be held.
/// {
///     let mut writer = lock.write();
///     *writer += 1;
///     assert_eq!(*writer, 6);
/// } // Write lock is released.
/// ```
pub type RwLock<T> = lock_api::RwLock<RawRwLock, T>;

/// An RAII guard which releases shared read access when dropped.
pub type RwLockReadGuard<'a, T> = lock_api::RwLockReadGuard<'a, RawRwLock, T>;

/// An RAII guard which releases exclusive write access when dropped.
pub type RwLockWriteGuard<'a, T> = lock_api::RwLockWriteGuard<'a, RawRwLock, T>;

/// An RAII read lock guard returned by [`RwLockReadGuard::map`], which can point to a
/// subfield of the protected data.
///
/// The main difference between `MappedRwLockReadGuard` and `RwLockReadGuard` is that the
/// former doesn't support temporarily unlocking and re-locking, since that
/// could introduce soundness issues if the locked object is modified by another
/// thread.
pub type MappedRwLockReadGuard<'a, T> = lock_api::MappedRwLockReadGuard<'a, RawRwLock, T>;

/// An RAII write lock guard returned by [`RwLockWriteGuard::map`], which can point to a
/// subfield of the protected data.
///
/// The main difference between `MappedRwLockWriteGuard` and `RwLockWriteGuard` is that the
/// former doesn't support temporarily unlocking and re-locking, since that
/// could introduce soundness issues if the locked object is modified by another
/// thread.
pub type MappedRwLockWriteGuard<'a, T> = lock_api::MappedRwLockWriteGuard<'a, RawRwLock, T>;

/// An RAII guard which releases upgradable read access when dropped.
pub type RwLockUpgradableReadGuard<'a, T> = lock_api::RwLockUpgradableReadGuard<'a, RawRwLock, T>;

#[cfg(test)]
mod tests {
    use crate::{RwLock, RwLockUpgradableReadGuard, RwLockWriteGuard};
    use rand::RngExt;
    use std::sync::Arc;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::thread;

    #[cfg(feature = "serde")]
    use postcard::{from_bytes, to_stdvec};

    #[derive(Eq, PartialEq, Debug)]
    struct NonCopy(i32);

    #[test]
    fn smoke() {
        let l = RwLock::new(());
        drop(l.read());
        drop(l.write());
        drop(l.upgradable_read());
        drop((l.read(), l.read()));
        drop((l.read(), l.upgradable_read()));
        drop(l.write());
    }

    #[test]
    fn frob() {
        const N: u32 = 10;
        const M: u32 = if cfg!(miri) { 100 } else { 1000 };

        let r = Arc::new(RwLock::new(()));

        let mut threads = Vec::new();
        for _ in 0..N {
            let r = r.clone();
            threads.push(thread::spawn(move || {
                let mut rng = rand::rng();
                for _ in 0..M {
                    if rng.random_bool(1.0 / N as f64) {
                        drop(r.write());
                    } else {
                        drop(r.read());
                    }
                }
            }));
        }
        for thread in threads {
            thread.join().unwrap();
        }
    }

    #[test]
    fn test_rw_arc_no_poison_wr() {
        let arc = Arc::new(RwLock::new(1));
        let arc2 = arc.clone();
        let _: Result<(), _> = thread::spawn(move || {
            let _lock = arc2.write();
            panic!();
        })
        .join();
        let lock = arc.read();
        assert_eq!(*lock, 1);
    }

    #[test]
    fn test_rw_arc_no_poison_ww() {
        let arc = Arc::new(RwLock::new(1));
        let arc2 = arc.clone();
        let _: Result<(), _> = thread::spawn(move || {
            let _lock = arc2.write();
            panic!();
        })
        .join();
        let lock = arc.write();
        assert_eq!(*lock, 1);
    }

    #[test]
    fn test_rw_arc_no_poison_rr() {
        let arc = Arc::new(RwLock::new(1));
        let arc2 = arc.clone();
        let _: Result<(), _> = thread::spawn(move || {
            let _lock = arc2.read();
            panic!();
        })
        .join();
        let lock = arc.read();
        assert_eq!(*lock, 1);
    }

    #[test]
    fn test_rw_arc_no_poison_rw() {
        let arc = Arc::new(RwLock::new(1));
        let arc2 = arc.clone();
        let _: Result<(), _> = thread::spawn(move || {
            let _lock = arc2.read();
            panic!()
        })
        .join();
        let lock = arc.write();
        assert_eq!(*lock, 1);
    }

    #[test]
    fn test_ruw_arc() {
        let arc = Arc::new(RwLock::new(0));
        let arc2 = arc.clone();

        let writer = thread::spawn(move || {
            for _ in 0..10 {
                let mut lock = arc2.write();
                let tmp = *lock;
                *lock = -1;
                thread::yield_now();
                *lock = tmp + 1;
            }
        });

        let mut children = Vec::new();

        // Upgradable readers try to catch the writer in the act and also
        // try to touch the value
        for _ in 0..5 {
            let arc3 = arc.clone();
            children.push(thread::spawn(move || {
                let lock = arc3.upgradable_read();
                let tmp = *lock;
                assert!(tmp >= 0);
                thread::yield_now();
                let mut lock = RwLockUpgradableReadGuard::upgrade(lock);
                assert_eq!(tmp, *lock);
                *lock = -1;
                thread::yield_now();
                *lock = tmp + 1;
            }));
        }

        // Readers try to catch the writers in the act
        for _ in 0..5 {
            let arc4 = arc.clone();
            children.push(thread::spawn(move || {
                let lock = arc4.read();
                assert!(*lock >= 0);
            }));
        }

        // Wait for children to pass their asserts
        for r in children {
            assert!(r.join().is_ok());
        }

        // Wait for writer to finish
        writer.join().unwrap();
        let lock = arc.read();
        assert_eq!(*lock, 15);
    }

    #[test]
    fn test_rw_arc() {
        let arc = Arc::new(RwLock::new(0));
        let arc2 = arc.clone();

        let writer = thread::spawn(move || {
            let mut lock = arc2.write();
            for _ in 0..10 {
                let tmp = *lock;
                *lock = -1;
                thread::yield_now();
                *lock = tmp + 1;
            }
        });

        // Readers try to catch the writer in the act
        let mut children = Vec::new();
        for _ in 0..5 {
            let arc3 = arc.clone();
            children.push(thread::spawn(move || {
                let lock = arc3.read();
                assert!(*lock >= 0);
            }));
        }

        // Wait for children to pass their asserts
        for r in children {
            assert!(r.join().is_ok());
        }

        // Wait for writer to finish
        writer.join().unwrap();
        let lock = arc.read();
        assert_eq!(*lock, 10);
    }

    #[test]
    fn test_rw_arc_access_in_unwind() {
        let arc = Arc::new(RwLock::new(1));
        let arc2 = arc.clone();
        let _ = thread::spawn(move || {
            struct Unwinder {
                i: Arc<RwLock<isize>>,
            }
            impl Drop for Unwinder {
                fn drop(&mut self) {
                    let mut lock = self.i.write();
                    *lock += 1;
                }
            }
            let _u = Unwinder { i: arc2 };
            panic!();
        })
        .join();
        let lock = arc.read();
        assert_eq!(*lock, 2);
    }

    #[test]
    fn test_rwlock_unsized() {
        let rw: &RwLock<[i32]> = &RwLock::new([1, 2, 3]);
        {
            let b = &mut *rw.write();
            b[0] = 4;
            b[2] = 5;
        }
        let comp: &[i32] = &[4, 2, 5];
        assert_eq!(&*rw.read(), comp);
    }

    #[test]
    fn test_read_with_waiting_writer() {
        let lock = Arc::new(RwLock::new(()));
        let reader = lock.read();

        let lock2 = Arc::clone(&lock);
        let writer = thread::spawn(move || drop(lock2.write()));
        while !unsafe { lock.raw() }.has_parked_threads() {
            thread::yield_now();
        }

        // A waiting writer blocks new readers even though no writer holds the lock.
        assert!(lock.try_read().is_none());

        drop(reader);
        writer.join().unwrap();
    }

    #[test]
    fn test_rwlock_try_read() {
        let lock = RwLock::new(0isize);
        {
            let read_guard = lock.read();

            let read_result = lock.try_read();
            assert!(
                read_result.is_some(),
                "try_read should succeed while read_guard is in scope"
            );

            drop(read_guard);
        }
        {
            let upgrade_guard = lock.upgradable_read();

            let read_result = lock.try_read();
            assert!(
                read_result.is_some(),
                "try_read should succeed while upgrade_guard is in scope"
            );

            drop(upgrade_guard);
        }
        {
            let write_guard = lock.write();

            let read_result = lock.try_read();
            assert!(
                read_result.is_none(),
                "try_read should fail while write_guard is in scope"
            );

            drop(write_guard);
        }
    }

    #[test]
    fn test_rwlock_try_write() {
        let lock = RwLock::new(0isize);
        {
            let read_guard = lock.read();

            let write_result = lock.try_write();
            assert!(
                write_result.is_none(),
                "try_write should fail while read_guard is in scope"
            );
            assert!(lock.is_locked());
            assert!(!lock.is_locked_exclusive());

            drop(read_guard);
        }
        {
            let upgrade_guard = lock.upgradable_read();

            let write_result = lock.try_write();
            assert!(
                write_result.is_none(),
                "try_write should fail while upgrade_guard is in scope"
            );
            assert!(lock.is_locked());
            assert!(!lock.is_locked_exclusive());

            drop(upgrade_guard);
        }
        {
            let write_guard = lock.write();

            let write_result = lock.try_write();
            assert!(
                write_result.is_none(),
                "try_write should fail while write_guard is in scope"
            );
            assert!(lock.is_locked());
            assert!(lock.is_locked_exclusive());

            drop(write_guard);
        }
    }

    #[test]
    fn test_rwlock_try_upgrade() {
        let lock = RwLock::new(0isize);
        {
            let read_guard = lock.read();

            let upgrade_result = lock.try_upgradable_read();
            assert!(
                upgrade_result.is_some(),
                "try_upgradable_read should succeed while read_guard is in scope"
            );

            drop(read_guard);
        }
        {
            let upgrade_guard = lock.upgradable_read();

            let upgrade_result = lock.try_upgradable_read();
            assert!(
                upgrade_result.is_none(),
                "try_upgradable_read should fail while upgrade_guard is in scope"
            );

            drop(upgrade_guard);
        }
        {
            let write_guard = lock.write();

            let upgrade_result = lock.try_upgradable_read();
            assert!(
                upgrade_result.is_none(),
                "try_upgradable should fail while write_guard is in scope"
            );

            drop(write_guard);
        }
    }

    #[test]
    fn test_into_inner() {
        let m = RwLock::new(NonCopy(10));
        assert_eq!(m.into_inner(), NonCopy(10));
    }

    #[test]
    fn test_into_inner_drop() {
        struct Foo(Arc<AtomicUsize>);
        impl Drop for Foo {
            fn drop(&mut self) {
                self.0.fetch_add(1, Ordering::SeqCst);
            }
        }
        let num_drops = Arc::new(AtomicUsize::new(0));
        let m = RwLock::new(Foo(num_drops.clone()));
        assert_eq!(num_drops.load(Ordering::SeqCst), 0);
        {
            let _inner = m.into_inner();
            assert_eq!(num_drops.load(Ordering::SeqCst), 0);
        }
        assert_eq!(num_drops.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn test_get_mut() {
        let mut m = RwLock::new(NonCopy(10));
        *m.get_mut() = NonCopy(20);
        assert_eq!(m.into_inner(), NonCopy(20));
    }

    #[test]
    fn test_rwlockguard_sync() {
        fn sync<T: Sync>(_: T) {}

        let rwlock = RwLock::new(());
        sync(rwlock.read());
        sync(rwlock.write());
    }

    #[test]
    fn test_rwlock_downgrade() {
        let x = Arc::new(RwLock::new(0));
        let mut handles = Vec::new();
        for _ in 0..8 {
            let x = x.clone();
            handles.push(thread::spawn(move || {
                for _ in 0..100 {
                    let mut writer = x.write();
                    *writer += 1;
                    let cur_val = *writer;
                    let reader = RwLockWriteGuard::downgrade(writer);
                    assert_eq!(cur_val, *reader);
                }
            }));
        }
        for handle in handles {
            handle.join().unwrap()
        }
        assert_eq!(*x.read(), 800);
    }

    #[test]
    fn test_rwlock_debug() {
        let x = RwLock::new(vec![0u8, 10]);

        assert_eq!(format!("{:?}", x), "RwLock { data: [0, 10] }");
        let _lock = x.write();
        assert_eq!(format!("{:?}", x), "RwLock { data: <locked> }");
    }

    #[test]
    fn test_clone_through_guard() {
        let rwlock = RwLock::new(Arc::new(1));
        let a = rwlock.read();
        // This must clone the protected Arc, not the read guard.
        let b = a.clone();
        assert_eq!(Arc::strong_count(&b), 2);
    }

    #[cfg(feature = "serde")]
    #[test]
    fn test_serde() {
        let contents: Vec<u8> = vec![0, 1, 2];
        let mutex = RwLock::new(contents.clone());

        let serialized = to_stdvec(&mutex).unwrap();
        let deserialized: RwLock<Vec<u8>> = from_bytes(&serialized).unwrap();

        assert_eq!(*(mutex.read()), *(deserialized.read()));
        assert_eq!(contents, *(deserialized.read()));
    }

    #[test]
    fn test_issue_203() {
        struct Bar(RwLock<()>);

        impl Drop for Bar {
            fn drop(&mut self) {
                let _n = self.0.write();
            }
        }

        thread_local! {
            static B: Bar = const { Bar(RwLock::new(())) };
        }

        thread::spawn(|| {
            B.with(|_| ());

            let a = RwLock::new(());
            let _a = a.read();
        })
        .join()
        .unwrap();
    }

    #[test]
    fn test_rw_write_is_locked() {
        let lock = RwLock::new(0isize);
        {
            let _read_guard = lock.read();

            assert!(lock.is_locked());
            assert!(!lock.is_locked_exclusive());
        }

        {
            let _write_guard = lock.write();

            assert!(lock.is_locked());
            assert!(lock.is_locked_exclusive());
        }
    }

    #[test]
    #[cfg(feature = "arc_lock")]
    fn test_issue_430() {
        let lock = std::sync::Arc::new(RwLock::new(0));

        let mut rl = lock.upgradable_read_arc();

        rl.with_upgraded(|_| {
            println!("lock upgrade");
        });

        rl.with_upgraded(|_| {
            println!("lock upgrade");
        });

        drop(lock);
    }
}
