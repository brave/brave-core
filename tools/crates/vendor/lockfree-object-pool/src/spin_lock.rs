use std::cell::UnsafeCell;
use std::ops::{Deref, DerefMut};
use std::sync::atomic::{AtomicBool, Ordering};
use std::thread;

pub struct SpinLock<T> {
    data: UnsafeCell<T>,
    lock: AtomicBool,
}

impl<T> SpinLock<T> {
    #[inline]
    pub fn new(data: T) -> Self {
        Self {
            data: UnsafeCell::new(data),
            lock: AtomicBool::new(false),
        }
    }

    #[inline]
    pub fn lock(&self) -> SpinLockGuard<T> {
        self.acquire();
        SpinLockGuard { lock: self }
    }

    #[inline]
    fn acquire(&self) {
        self.exchange(false, true);
    }

    #[inline]
    fn release(&self) {
        self.exchange(true, false);
    }

    #[inline]
    fn exchange(&self, from: bool, to: bool) {
        loop {
            match self
                .lock
                .compare_exchange_weak(from, to, Ordering::SeqCst, Ordering::Relaxed)
            {
                Ok(_) => break,
                Err(_) => {
                    thread::yield_now();
                }
            }
        }
    }
}

unsafe impl<T: Send> Send for SpinLock<T> {} // SAFETY: sending the data is allowed if it's Send
unsafe impl<T: Send> Sync for SpinLock<T> {} // SAFETY: the Mutex manages synchronization so only Send is required

pub struct SpinLockGuard<'a, T> {
    lock: &'a SpinLock<T>,
}

impl<'a, T> DerefMut for SpinLockGuard<'a, T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe {
            // SAFETY: this is the only active guard
            &mut *self.lock.data.get()
        }
    }
}

impl<'a, T> Deref for SpinLockGuard<'a, T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &Self::Target {
        unsafe {
            // SAFETY: this is the only active guard
            &*self.lock.data.get()
        }
    }
}

impl<'a, T> Drop for SpinLockGuard<'a, T> {
    #[inline]
    fn drop(&mut self) {
        self.lock.release();
    }
}

unsafe impl<T: Send> Send for SpinLockGuard<'_, T> {} // SAFETY: normal rules apply
unsafe impl<T: Sync> Sync for SpinLockGuard<'_, T> {} // SAFETY: normal rules apply
