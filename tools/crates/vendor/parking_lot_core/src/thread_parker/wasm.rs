//! The wasm platform can't park when atomic support is not available.
//! So this ThreadParker just panics on any attempt to park.

use std::thread;
use std::time::Instant;

pub struct ThreadParker(());

impl super::ThreadParkerT for ThreadParker {
    type UnparkHandle = UnparkHandle;

    const IS_CHEAP_TO_CONSTRUCT: bool = true;

    fn new() -> ThreadParker {
        ThreadParker(())
    }

    unsafe fn prepare_park(&self) {
        panic!("Parking not supported on this platform");
    }

    unsafe fn timed_out(&self) -> bool {
        panic!("Parking not supported on this platform");
    }

    unsafe fn park(&self) {
        panic!("Parking not supported on this platform");
    }

    unsafe fn park_until(&self, _timeout: Instant) -> bool {
        panic!("Parking not supported on this platform");
    }

    unsafe fn unpark_lock(&self) -> UnparkHandle {
        panic!("Parking not supported on this platform");
    }
}

pub struct UnparkHandle(());

impl super::UnparkHandleT for UnparkHandle {
    unsafe fn unpark(self) {}
}

pub fn thread_yield() {
    thread::yield_now();
}
