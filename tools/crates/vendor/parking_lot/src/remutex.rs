use crate::raw_mutex::RawMutex;
use core::num::NonZeroUsize;
use lock_api::{self, GetThreadId};

/// Implementation of the `GetThreadId` trait for `lock_api::ReentrantMutex`.
pub struct RawThreadId;

unsafe impl GetThreadId for RawThreadId {
    const INIT: RawThreadId = RawThreadId;

    fn nonzero_thread_id(&self) -> NonZeroUsize {
        // The address of a thread-local variable is guaranteed to be unique to the
        // current thread, and is also guaranteed to be non-zero. The variable has to have a
        // non-zero size to guarantee it has a unique address for each thread.
        thread_local!(static KEY: u8 = const { 0 });
        KEY.with(|x| {
            NonZeroUsize::new(core::ptr::from_ref(x).addr())
                .expect("thread-local variable address is null")
        })
    }
}

/// A reentrant mutual exclusion lock.
///
/// This lock blocks other threads waiting for it to become available. A thread
/// which already holds the lock can acquire it additional times without
/// blocking.
///
/// Unlike [`Mutex`](crate::Mutex), [`ReentrantMutexGuard`] does not provide
/// mutable references to the locked data, because multiple guards can coexist
/// on the same thread. Use interior mutability, such as [`RefCell`](core::cell::RefCell),
/// to mutate the guarded data.
///
/// See [`Mutex`](crate::Mutex) for more details about the underlying mutex
/// primitive.
///
/// # Examples
///
/// ```
/// use parking_lot::ReentrantMutex;
/// use std::cell::RefCell;
///
/// let lock = ReentrantMutex::new(RefCell::new(0));
/// let first = lock.lock();
/// let second = lock.lock();
/// *first.borrow_mut() += 1;
/// *second.borrow_mut() += 1;
/// assert_eq!(*lock.lock().borrow(), 2);
/// ```
pub type ReentrantMutex<T> = lock_api::ReentrantMutex<RawMutex, RawThreadId, T>;

/// An RAII guard which releases one level of recursive locking when dropped.
///
/// The data protected by the mutex can be accessed through this guard via its
/// [`Deref`](core::ops::Deref) implementation.
pub type ReentrantMutexGuard<'a, T> = lock_api::ReentrantMutexGuard<'a, RawMutex, RawThreadId, T>;

/// An RAII mutex guard returned by [`ReentrantMutexGuard::map`], which can point to a
/// subfield of the protected data.
///
/// The main difference between `MappedReentrantMutexGuard` and `ReentrantMutexGuard` is that the
/// former doesn't support temporarily unlocking and re-locking, since that
/// could introduce soundness issues if the locked object is modified by another
/// thread.
pub type MappedReentrantMutexGuard<'a, T> =
    lock_api::MappedReentrantMutexGuard<'a, RawMutex, RawThreadId, T>;

#[cfg(test)]
mod tests {
    use crate::ReentrantMutex;
    use crate::ReentrantMutexGuard;
    use std::cell::RefCell;
    use std::sync::Arc;
    use std::sync::mpsc::channel;
    use std::thread;

    #[cfg(feature = "serde")]
    use postcard::{from_bytes, to_stdvec};

    #[test]
    fn smoke() {
        let m = ReentrantMutex::new(2);
        {
            let a = m.lock();
            {
                let b = m.lock();
                {
                    let c = m.lock();
                    assert_eq!(*c, 2);
                }
                assert_eq!(*b, 2);
            }
            assert_eq!(*a, 2);
        }
    }

    #[test]
    fn is_mutex() {
        let m = Arc::new(ReentrantMutex::new(RefCell::new(0)));
        let m2 = m.clone();
        let lock = m.lock();
        let child = thread::spawn(move || {
            let lock = m2.lock();
            assert_eq!(*lock.borrow(), 4950);
        });
        for i in 0..100 {
            let lock = m.lock();
            *lock.borrow_mut() += i;
        }
        drop(lock);
        child.join().unwrap();
    }

    #[test]
    fn trylock_works() {
        let m = Arc::new(ReentrantMutex::new(()));
        let m2 = m.clone();
        let _lock = m.try_lock().unwrap();
        let _lock2 = m.try_lock().unwrap();
        thread::spawn(move || {
            let lock = m2.try_lock();
            assert!(lock.is_none());
        })
        .join()
        .unwrap();
        let _lock3 = m.try_lock().unwrap();
    }

    #[test]
    fn test_reentrant_mutex_debug() {
        let mutex = ReentrantMutex::new(vec![0u8, 10]);

        assert_eq!(format!("{:?}", mutex), "ReentrantMutex { data: [0, 10] }");
    }

    #[test]
    fn test_reentrant_mutex_bump() {
        let mutex = Arc::new(ReentrantMutex::new(()));
        let mutex2 = mutex.clone();

        let mut guard = mutex.lock();

        let (tx, rx) = channel();

        thread::spawn(move || {
            let _guard = mutex2.lock();
            tx.send(()).unwrap();
        });

        // `bump()` repeatedly until the thread starts up and requests the lock
        while rx.try_recv().is_err() {
            ReentrantMutexGuard::bump(&mut guard);
        }
    }

    #[cfg(feature = "serde")]
    #[test]
    fn test_serde() {
        let contents: Vec<u8> = vec![0, 1, 2];
        let mutex = ReentrantMutex::new(contents.clone());

        let serialized = to_stdvec(&mutex).unwrap();
        let deserialized: ReentrantMutex<Vec<u8>> = from_bytes(&serialized).unwrap();

        assert_eq!(*(mutex.lock()), *(deserialized.lock()));
        assert_eq!(contents, *(deserialized.lock()));
    }
}
