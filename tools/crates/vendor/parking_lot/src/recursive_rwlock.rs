use crate::RawRwLockRecursive;

/// A reader-writer lock which permits recursive read locking.
///
/// This type of lock allows a number of readers or at most one writer at any
/// point in time. Unlike [`RwLock`](crate::RwLock), a thread can hold multiple
/// read locks without deadlocking, at the cost of allowing writers to starve.
///
/// # Examples
///
/// ```
/// use parking_lot::RecursiveRwLock;
///
/// let lock = RecursiveRwLock::new(5);
/// let first = lock.read();
/// let second = lock.read();
/// assert_eq!(*first, *second);
/// ```
pub type RecursiveRwLock<T> = lock_api::RwLock<RawRwLockRecursive, T>;

/// An RAII guard which releases shared read access when dropped.
pub type RecursiveRwLockReadGuard<'a, T> = lock_api::RwLockReadGuard<'a, RawRwLockRecursive, T>;

/// An RAII guard which releases exclusive write access when dropped.
pub type RecursiveRwLockWriteGuard<'a, T> = lock_api::RwLockWriteGuard<'a, RawRwLockRecursive, T>;

/// An RAII guard which releases upgradable read access when dropped.
pub type RecursiveRwLockUpgradableReadGuard<'a, T> =
    lock_api::RwLockUpgradableReadGuard<'a, RawRwLockRecursive, T>;

/// A mapped RAII guard which releases shared access when dropped.
pub type MappedRecursiveRwLockReadGuard<'a, T> =
    lock_api::MappedRwLockReadGuard<'a, RawRwLockRecursive, T>;

/// A mapped RAII guard which releases exclusive access when dropped.
pub type MappedRecursiveRwLockWriteGuard<'a, T> =
    lock_api::MappedRwLockWriteGuard<'a, RawRwLockRecursive, T>;

#[cfg(test)]
mod tests {
    use crate::{RecursiveRwLock, RecursiveRwLockReadGuard, RecursiveRwLockUpgradableReadGuard};
    use rand::RngExt;
    use std::sync::Arc;
    use std::thread;

    #[test]
    fn frob() {
        const N: u32 = 10;
        const M: u32 = if cfg!(miri) { 100 } else { 1000 };

        let lock = Arc::new(RecursiveRwLock::new(()));
        let mut threads = Vec::new();
        for _ in 0..N {
            let lock = Arc::clone(&lock);
            threads.push(thread::spawn(move || {
                let mut rng = rand::rng();
                for _ in 0..M {
                    if rng.random_bool(1.0 / N as f64) {
                        drop(lock.write());
                    } else {
                        drop(lock.read());
                    }
                }
            }));
        }
        for thread in threads {
            thread.join().unwrap();
        }
    }

    #[test]
    fn recursive_read_with_waiting_writer() {
        let lock = Arc::new(RecursiveRwLock::new(()));
        let first = lock.read();
        let second = lock.read();

        let lock2 = Arc::clone(&lock);
        let writer = thread::spawn(move || drop(lock2.write()));
        while !unsafe { lock.raw() }.has_parked_threads() {
            thread::yield_now();
        }

        assert!(lock.try_read().is_some());
        let mut second = second;
        RecursiveRwLockReadGuard::bump(&mut second);

        drop(first);
        drop(second);
        writer.join().unwrap();
    }

    #[test]
    fn upgrade() {
        let lock = RecursiveRwLock::new(1);
        let upgradable = lock.upgradable_read();
        let read = lock.read();
        drop(read);
        let mut write = RecursiveRwLockUpgradableReadGuard::upgrade(upgradable);
        *write = 2;
        drop(write);
        assert_eq!(*lock.read(), 2);
    }
}
