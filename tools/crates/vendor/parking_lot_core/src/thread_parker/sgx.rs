use core::sync::atomic::{AtomicBool, Ordering};
use std::io::ErrorKind;
use std::time::Instant;
use std::{
    io,
    os::fortanix_sgx::{
        thread::current as current_tcs,
        usercalls::{
            self,
            raw::{EV_UNPARK, Tcs, WAIT_INDEFINITE},
        },
    },
    thread,
};

// Helper type for putting a thread to sleep until some other thread wakes it up
pub struct ThreadParker {
    parked: AtomicBool,
    tcs: Tcs,
}

impl super::ThreadParkerT for ThreadParker {
    type UnparkHandle = UnparkHandle;

    const IS_CHEAP_TO_CONSTRUCT: bool = true;

    #[inline]
    fn new() -> ThreadParker {
        ThreadParker {
            parked: AtomicBool::new(false),
            tcs: current_tcs(),
        }
    }

    #[inline]
    unsafe fn prepare_park(&self) {
        self.parked.store(true, Ordering::Relaxed);
    }

    #[inline]
    unsafe fn timed_out(&self) -> bool {
        self.parked.load(Ordering::Relaxed)
    }

    #[inline]
    unsafe fn park(&self) {
        while self.parked.load(Ordering::Acquire) {
            let events = usercalls::wait(EV_UNPARK, WAIT_INDEFINITE)
                .expect("wait returned an unexpected error");
            assert_eq!(events & EV_UNPARK, EV_UNPARK);
        }
    }

    #[inline]
    unsafe fn park_until(&self, timeout: Instant) -> bool {
        while self.parked.load(Ordering::Acquire) {
            let now = Instant::now();
            if timeout <= now {
                return false;
            }
            let remaining = timeout - now;
            let remaining_nanos =
                u128::min(remaining.as_nanos(), WAIT_INDEFINITE as u128 - 1) as u64;

            match usercalls::wait(EV_UNPARK, remaining_nanos) {
                Ok(_) => {}
                Err(error)
                    if error.kind() == ErrorKind::TimedOut
                        || error.kind() == ErrorKind::WouldBlock => {}
                Err(error) => panic!("wait returned an unexpected error: {error}"),
            }
        }
        true
    }

    #[inline]
    unsafe fn unpark_lock(&self) -> UnparkHandle {
        // The target may destroy the parker as soon as the store below is
        // observed, so construct the handle first.
        let handle = UnparkHandle(self.tcs);

        // We don't need to lock anything, just clear the state
        self.parked.store(false, Ordering::Release);
        handle
    }
}

pub struct UnparkHandle(Tcs);

impl super::UnparkHandleT for UnparkHandle {
    #[inline]
    unsafe fn unpark(self) {
        let result = usercalls::send(EV_UNPARK, Some(self.0));
        if let Err(error) = result
            // `InvalidInput` may be returned if the thread we send to has
            // already been unparked and exited.
            && error.kind() != io::ErrorKind::InvalidInput
        {
            panic!("send returned an unexpected error: {error:?}");
        }
    }
}

#[inline]
pub fn thread_yield() {
    thread::yield_now();
}
