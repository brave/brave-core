use core::{
    ptr,
    sync::atomic::{AtomicI32, Ordering},
};
use std::thread;
use std::time::Instant;
use syscall::{
    call::futex,
    data::TimeSpec,
    error::{EAGAIN, EFAULT, EINTR, ETIMEDOUT, Error},
    flag::{FUTEX_WAIT, FUTEX_WAKE},
};

const UNPARKED: i32 = 0;
const PARKED: i32 = 1;

// Helper type for putting a thread to sleep until some other thread wakes it up
pub struct ThreadParker {
    futex: AtomicI32,
}

impl super::ThreadParkerT for ThreadParker {
    type UnparkHandle = UnparkHandle;

    const IS_CHEAP_TO_CONSTRUCT: bool = true;

    #[inline]
    fn new() -> ThreadParker {
        ThreadParker {
            futex: AtomicI32::new(UNPARKED),
        }
    }

    #[inline]
    unsafe fn prepare_park(&self) {
        self.futex.store(PARKED, Ordering::Relaxed);
    }

    #[inline]
    unsafe fn timed_out(&self) -> bool {
        self.futex.load(Ordering::Relaxed) != UNPARKED
    }

    #[inline]
    unsafe fn park(&self) {
        while self.futex.load(Ordering::Acquire) != UNPARKED {
            self.futex_wait(None);
        }
    }

    #[inline]
    unsafe fn park_until(&self, timeout: Instant) -> bool {
        while self.futex.load(Ordering::Acquire) != UNPARKED {
            let now = Instant::now();
            if timeout <= now {
                return false;
            }
            let diff = timeout - now;
            if diff.as_secs() > i64::MAX as u64 {
                // Timeout overflowed, just sleep indefinitely
                unsafe { self.park() };
                return true;
            }
            let ts = TimeSpec {
                tv_sec: diff.as_secs() as i64,
                tv_nsec: diff.subsec_nanos() as i32,
            };
            self.futex_wait(Some(ts));
        }
        true
    }

    #[inline]
    unsafe fn unpark_lock(&self) -> UnparkHandle {
        // The target may destroy the parker as soon as the store below is
        // observed, so construct the handle first.
        let handle = UnparkHandle { futex: self.ptr() };

        // We don't need to lock anything, just clear the state
        self.futex.store(UNPARKED, Ordering::Release);
        handle
    }
}

impl ThreadParker {
    #[inline]
    fn futex_wait(&self, ts: Option<TimeSpec>) {
        let ts_ptr: *const TimeSpec = ts
            .as_ref()
            .map(|ts_ref| ts_ref as *const _)
            .unwrap_or(ptr::null());
        let r = unsafe {
            futex(
                self.ptr(),
                FUTEX_WAIT,
                PARKED,
                // The syscall ABI carries this pointer as an integer.
                ts_ptr.expose_provenance(),
                ptr::null_mut(),
            )
        };
        match r {
            Ok(r) => assert_eq!(r, 0),
            Err(Error { errno }) => {
                assert!(errno == EINTR || errno == EAGAIN || errno == ETIMEDOUT);
            }
        }
    }

    #[inline]
    fn ptr(&self) -> *mut i32 {
        &self.futex as *const AtomicI32 as *mut i32
    }
}

pub struct UnparkHandle {
    futex: *mut i32,
}

impl super::UnparkHandleT for UnparkHandle {
    #[inline]
    unsafe fn unpark(self) {
        // The thread data may have been freed at this point, but it doesn't
        // matter since the syscall will just return EFAULT in that case.
        let r = unsafe { futex(self.futex, FUTEX_WAKE, PARKED, 0, ptr::null_mut()) };
        match r {
            Ok(num_woken) => assert!(num_woken == 0 || num_woken == 1),
            Err(Error { errno }) => assert_eq!(errno, EFAULT),
        }
    }
}

#[inline]
pub fn thread_yield() {
    thread::yield_now();
}
