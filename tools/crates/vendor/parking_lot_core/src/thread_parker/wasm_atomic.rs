use core::{
    arch::wasm32,
    ptr::NonNull,
    sync::atomic::{AtomicI32, Ordering},
};
use std::thread;
use std::time::Instant;

// Helper type for putting a thread to sleep until some other thread wakes it up
pub struct ThreadParker {
    parked: AtomicI32,
}

const UNPARKED: i32 = 0;
const PARKED: i32 = 1;

impl super::ThreadParkerT for ThreadParker {
    type UnparkHandle = UnparkHandle;

    const IS_CHEAP_TO_CONSTRUCT: bool = true;

    #[inline]
    fn new() -> ThreadParker {
        ThreadParker {
            parked: AtomicI32::new(UNPARKED),
        }
    }

    #[inline]
    unsafe fn prepare_park(&self) {
        self.parked.store(PARKED, Ordering::Relaxed);
    }

    #[inline]
    unsafe fn timed_out(&self) -> bool {
        self.parked.load(Ordering::Relaxed) == PARKED
    }

    #[inline]
    unsafe fn park(&self) {
        while self.parked.load(Ordering::Acquire) == PARKED {
            let r = unsafe { wasm32::memory_atomic_wait32(self.ptr().as_ptr(), PARKED, -1) };
            // we should have either woken up (0) or got a not-equal due to a
            // race (1). We should never time out (2)
            assert!(r == 0 || r == 1);
        }
    }

    #[inline]
    unsafe fn park_until(&self, timeout: Instant) -> bool {
        while self.parked.load(Ordering::Acquire) == PARKED {
            if let Some(left) = timeout.checked_duration_since(Instant::now()) {
                let nanos_left = i64::try_from(left.as_nanos()).unwrap_or(i64::MAX);
                let r = unsafe {
                    wasm32::memory_atomic_wait32(self.ptr().as_ptr(), PARKED, nanos_left)
                };
                assert!(r == 0 || r == 1 || r == 2);
            } else {
                return false;
            }
        }
        true
    }

    #[inline]
    unsafe fn unpark_lock(&self) -> UnparkHandle {
        // The target may destroy the parker as soon as the store below is
        // observed, so construct the handle first.
        let handle = UnparkHandle(self.ptr());

        // We don't need to lock anything, just clear the state
        self.parked.store(UNPARKED, Ordering::Release);
        handle
    }
}

impl ThreadParker {
    #[inline]
    fn ptr(&self) -> NonNull<i32> {
        NonNull::from(&self.parked).cast()
    }
}

pub struct UnparkHandle(NonNull<i32>);

impl super::UnparkHandleT for UnparkHandle {
    #[inline]
    unsafe fn unpark(self) {
        let num_notified = unsafe { wasm32::memory_atomic_notify(self.0.as_ptr(), 1) };
        assert!(num_notified == 0 || num_notified == 1);
    }
}

#[inline]
pub fn thread_yield() {
    thread::yield_now();
}
