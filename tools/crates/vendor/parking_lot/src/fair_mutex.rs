use crate::raw_fair_mutex::RawFairMutex;

/// A mutual exclusion primitive that uses fair unlocking, useful for protecting shared data.
///
/// This mutex will block threads waiting for the lock to become available. The
/// mutex can be statically initialized or created by the `new`
/// constructor. Each mutex has a type parameter which represents the data that
/// it is protecting. The data can only be accessed through the RAII guards
/// returned from `lock` and `try_lock`, which guarantees that the data is only
/// ever accessed when the mutex is locked.
///
/// The regular mutex provided by `parking_lot` uses unfair unlocking by
/// default, whereas this mutex always uses a fair unlock. When there are parked
/// waiters, a fair unlock hands the mutex directly to one of them instead of
/// making the mutex available for the unlocking thread to immediately
/// re-acquire. Fair mutexes are generally slower, but can be useful when
/// predictable handoff is more important than throughput.
///
/// Fair unlocking does not imply strict first-in first-out ordering: threads
/// may acquire the mutex while spinning or through `try_lock` without joining
/// the queue of parked waiters.
///
/// # Differences from the standard library `Mutex`
///
/// - No poisoning, the lock is released normally on panic.
/// - Only requires 1 byte of lock state.
/// - Always uses fair unlocking.
/// - Supports locking with a timeout.
/// - Allows raw locking & unlocking without a guard.
///
/// # Examples
///
/// ```
/// use parking_lot::FairMutex;
/// use std::sync::{Arc, mpsc::channel};
/// use std::thread;
///
/// const N: usize = 10;
///
/// // Spawn a few threads to increment a shared variable (non-atomically), and
/// // let the main thread know once all increments are done.
/// //
/// // Here we're using an Arc to share memory among threads, and the data inside
/// // the Arc is protected with a mutex.
/// let data = Arc::new(FairMutex::new(0));
///
/// let (tx, rx) = channel();
/// for _ in 0..N {
///     let (data, tx) = (Arc::clone(&data), tx.clone());
///     thread::spawn(move || {
///         // The shared state can only be accessed once the lock is held.
///         // Our non-atomic increment is safe because we're the only thread
///         // which can access the shared state when the lock is held.
///         let mut data = data.lock();
///         *data += 1;
///         if *data == N {
///             tx.send(()).unwrap();
///         }
///         // the lock is unlocked here when `data` goes out of scope.
///     });
/// }
///
/// rx.recv().unwrap();
/// ```
pub type FairMutex<T> = lock_api::Mutex<RawFairMutex, T>;

/// An RAII guard which unlocks the mutex when dropped.
///
/// The data protected by the mutex can be accessed through this guard via its
/// [`Deref`](core::ops::Deref) and [`DerefMut`](core::ops::DerefMut)
/// implementations.
pub type FairMutexGuard<'a, T> = lock_api::MutexGuard<'a, RawFairMutex, T>;

/// An RAII mutex guard returned by [`FairMutexGuard::map`], which can point to a
/// subfield of the protected data.
///
/// The main difference between `MappedFairMutexGuard` and `FairMutexGuard` is that the
/// former doesn't support temporarily unlocking and re-locking, since that
/// could introduce soundness issues if the locked object is modified by another
/// thread.
pub type MappedFairMutexGuard<'a, T> = lock_api::MappedMutexGuard<'a, RawFairMutex, T>;

#[cfg(test)]
mod tests {
    use crate::FairMutex;
    use std::sync::Arc;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::mpsc::channel;
    use std::thread;

    #[cfg(feature = "serde")]
    use postcard::{from_bytes, to_stdvec};

    #[derive(Eq, PartialEq, Debug)]
    struct NonCopy(i32);

    #[test]
    fn smoke() {
        let m = FairMutex::new(());
        drop(m.lock());
        drop(m.lock());
    }

    #[test]
    fn lots_and_lots() {
        const J: u32 = if cfg!(miri) { 200 } else { 1000 };
        const K: u32 = 3;

        let m = Arc::new(FairMutex::new(0));

        fn inc(m: &FairMutex<u32>) {
            for _ in 0..J {
                *m.lock() += 1;
            }
        }

        let (tx, rx) = channel();
        for _ in 0..K {
            let tx2 = tx.clone();
            let m2 = m.clone();
            thread::spawn(move || {
                inc(&m2);
                tx2.send(()).unwrap();
            });
            let tx2 = tx.clone();
            let m2 = m.clone();
            thread::spawn(move || {
                inc(&m2);
                tx2.send(()).unwrap();
            });
        }

        drop(tx);
        for _ in 0..2 * K {
            rx.recv().unwrap();
        }
        assert_eq!(*m.lock(), J * K * 2);
    }

    #[test]
    fn try_lock() {
        let m = FairMutex::new(());
        *m.try_lock().unwrap() = ();
    }

    #[test]
    fn test_into_inner() {
        let m = FairMutex::new(NonCopy(10));
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
        let m = FairMutex::new(Foo(num_drops.clone()));
        assert_eq!(num_drops.load(Ordering::SeqCst), 0);
        {
            let _inner = m.into_inner();
            assert_eq!(num_drops.load(Ordering::SeqCst), 0);
        }
        assert_eq!(num_drops.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn test_get_mut() {
        let mut m = FairMutex::new(NonCopy(10));
        *m.get_mut() = NonCopy(20);
        assert_eq!(m.into_inner(), NonCopy(20));
    }

    #[test]
    fn test_mutex_arc_nested() {
        // Tests nested mutexes and access
        // to underlying data.
        let arc = Arc::new(FairMutex::new(1));
        let arc2 = Arc::new(FairMutex::new(arc));
        let (tx, rx) = channel();
        let _t = thread::spawn(move || {
            let lock = arc2.lock();
            let lock2 = lock.lock();
            assert_eq!(*lock2, 1);
            tx.send(()).unwrap();
        });
        rx.recv().unwrap();
    }

    #[test]
    fn test_mutex_arc_access_in_unwind() {
        let arc = Arc::new(FairMutex::new(1));
        let arc2 = arc.clone();
        let _ = thread::spawn(move || {
            struct Unwinder {
                i: Arc<FairMutex<i32>>,
            }
            impl Drop for Unwinder {
                fn drop(&mut self) {
                    *self.i.lock() += 1;
                }
            }
            let _u = Unwinder { i: arc2 };
            panic!();
        })
        .join();
        let lock = arc.lock();
        assert_eq!(*lock, 2);
    }

    #[test]
    fn test_mutex_unsized() {
        let mutex: &FairMutex<[i32]> = &FairMutex::new([1, 2, 3]);
        {
            let b = &mut *mutex.lock();
            b[0] = 4;
            b[2] = 5;
        }
        let comp: &[i32] = &[4, 2, 5];
        assert_eq!(&*mutex.lock(), comp);
    }

    #[test]
    fn test_mutexguard_sync() {
        fn sync<T: Sync>(_: T) {}

        let mutex = FairMutex::new(());
        sync(mutex.lock());
    }

    #[test]
    fn test_mutex_debug() {
        let mutex = FairMutex::new(vec![0u8, 10]);

        assert_eq!(format!("{:?}", mutex), "Mutex { data: [0, 10] }");
        let _lock = mutex.lock();
        assert_eq!(format!("{:?}", mutex), "Mutex { data: <locked> }");
    }

    #[cfg(feature = "serde")]
    #[test]
    fn test_serde() {
        let contents: Vec<u8> = vec![0, 1, 2];
        let mutex = FairMutex::new(contents.clone());

        let serialized = to_stdvec(&mutex).unwrap();
        let deserialized: FairMutex<Vec<u8>> = from_bytes(&serialized).unwrap();

        assert_eq!(*(mutex.lock()), *(deserialized.lock()));
        assert_eq!(contents, *(deserialized.lock()));
    }
}
