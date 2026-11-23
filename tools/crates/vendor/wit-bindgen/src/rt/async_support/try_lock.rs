use core::cell::UnsafeCell;
use core::ops::{Deref, DerefMut};
use core::sync::atomic::{
    AtomicBool,
    Ordering::{Acquire, Release},
};

/// Small helper type to wrap `T` in a lock-like primitive which only supports
/// the `try_lock` operation.
///
/// This is useful on wasm right now where threads aren't actually a thing so
/// there shouldn't ever be contention, but the Rust type system still requires
/// Send/Sync.
#[derive(Default)]
pub struct TryLock<T> {
    locked: AtomicBool,
    data: UnsafeCell<T>,
}

unsafe impl<T: Send> Send for TryLock<T> {}
unsafe impl<T: Send> Sync for TryLock<T> {}

impl<T> TryLock<T> {
    pub fn new(data: T) -> Self {
        TryLock {
            locked: AtomicBool::new(false),
            data: UnsafeCell::new(data),
        }
    }

    pub fn try_lock(&self) -> Option<TryLockGuard<'_, T>> {
        if self.locked.swap(true, Acquire) {
            None
        } else {
            Some(TryLockGuard { lock: self })
        }
    }
}

pub struct TryLockGuard<'a, T> {
    lock: &'a TryLock<T>,
}

impl<T> Deref for TryLockGuard<'_, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        unsafe { &*self.lock.data.get() }
    }
}

impl<T> DerefMut for TryLockGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { &mut *self.lock.data.get() }
    }
}

impl<T> Drop for TryLockGuard<'_, T> {
    fn drop(&mut self) {
        self.lock.locked.store(false, Release);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn smoke() {
        let lock = TryLock::new(1);
        assert!(lock.try_lock().is_some());
        assert!(lock.try_lock().is_some());

        let mut guard = lock.try_lock().unwrap();
        assert_eq!(*guard, 1);
        *guard = 2;
        assert_eq!(*guard, 2);
        assert!(lock.try_lock().is_none());
        drop(guard);
        assert!(lock.try_lock().is_some());

        let guard = lock.try_lock().unwrap();
        assert_eq!(*guard, 2);
    }
}
