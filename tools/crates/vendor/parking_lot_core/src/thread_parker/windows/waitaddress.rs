use core::{
    mem,
    ptr::NonNull,
    sync::atomic::{AtomicUsize, Ordering},
};
use std::{ffi, time::Instant};

use super::bindings::*;

#[allow(non_snake_case)]
pub struct WaitAddress {
    WaitOnAddress: WaitOnAddress,
    WakeByAddressSingle: WakeByAddressSingle,
}

impl WaitAddress {
    #[allow(non_snake_case)]
    pub fn create() -> Option<WaitAddress> {
        let synch_dll = unsafe { GetModuleHandleA(b"api-ms-win-core-synch-l1-2-0.dll\0".as_ptr()) };
        if synch_dll.is_null() {
            return None;
        }

        let WaitOnAddress = unsafe { GetProcAddress(synch_dll, b"WaitOnAddress\0".as_ptr())? };
        let WakeByAddressSingle =
            unsafe { GetProcAddress(synch_dll, b"WakeByAddressSingle\0".as_ptr())? };

        Some(WaitAddress {
            WaitOnAddress: unsafe { mem::transmute(WaitOnAddress) },
            WakeByAddressSingle: unsafe { mem::transmute(WakeByAddressSingle) },
        })
    }

    #[inline]
    pub fn prepare_park(&'static self, key: &AtomicUsize) {
        key.store(1, Ordering::Relaxed);
    }

    #[inline]
    pub fn timed_out(&'static self, key: &AtomicUsize) -> bool {
        key.load(Ordering::Relaxed) != 0
    }

    #[inline]
    pub fn park(&'static self, key: &AtomicUsize) {
        while key.load(Ordering::Acquire) != 0 {
            let r = self.wait_on_address(key, INFINITE);
            assert_ne!(r, false.into());
        }
    }

    #[inline]
    pub fn park_until(&'static self, key: &AtomicUsize, timeout: Instant) -> bool {
        while key.load(Ordering::Acquire) != 0 {
            let now = Instant::now();
            if timeout <= now {
                return false;
            }
            let diff = timeout - now;
            let timeout = diff
                .as_nanos()
                .div_ceil(1_000_000)
                .min((INFINITE - 1) as u128) as u32;
            if self.wait_on_address(key, timeout) == false.into() {
                assert_eq!(unsafe { GetLastError() }, ERROR_TIMEOUT);
            }
        }
        true
    }

    #[inline]
    pub fn unpark_lock(&'static self, key: &AtomicUsize) -> UnparkHandle {
        // The target may destroy the parker as soon as the store below is
        // observed, so construct the handle first.
        let handle = UnparkHandle {
            key: NonNull::from(key),
            waitaddress: self,
        };

        // We don't need to lock anything, just clear the state
        key.store(0, Ordering::Release);
        handle
    }

    #[inline]
    fn wait_on_address(&'static self, key: &AtomicUsize, timeout: u32) -> BOOL {
        let cmp = 1usize;
        unsafe {
            (self.WaitOnAddress)(
                key as *const _ as *mut ffi::c_void,
                &cmp as *const _ as *mut ffi::c_void,
                mem::size_of::<usize>(),
                timeout,
            )
        }
    }
}

// Handle for a thread that is about to be unparked. We need to mark the thread
// as unparked while holding the queue lock, but we delay the actual unparking
// until after the queue lock is released.
pub struct UnparkHandle {
    key: NonNull<AtomicUsize>,
    waitaddress: &'static WaitAddress,
}

impl UnparkHandle {
    // Wakes up the parked thread. This should be called after the queue lock is
    // released to avoid blocking the queue for too long.
    #[inline]
    pub fn unpark(self) {
        unsafe { (self.waitaddress.WakeByAddressSingle)(self.key.as_ptr().cast::<ffi::c_void>()) };
    }
}
