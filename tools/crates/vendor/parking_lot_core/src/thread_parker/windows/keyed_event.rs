use core::{
    ffi,
    mem::{self, MaybeUninit},
    ptr::{self, NonNull},
};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Instant;

const STATE_UNPARKED: usize = 0;
const STATE_PARKED: usize = 1;
const STATE_TIMED_OUT: usize = 2;

use super::bindings::*;

#[allow(non_snake_case)]
pub struct KeyedEvent {
    handle: HANDLE,
    NtReleaseKeyedEvent: NtReleaseKeyedEvent,
    NtWaitForKeyedEvent: NtWaitForKeyedEvent,
}

// SAFETY: Keyed-event handles may be used from any thread, including concurrently.
unsafe impl Send for KeyedEvent {}
unsafe impl Sync for KeyedEvent {}

impl KeyedEvent {
    #[inline]
    unsafe fn wait_for(&self, key: *mut ffi::c_void, timeout: *mut i64) -> NTSTATUS {
        // SAFETY: The caller guarantees that `key` and `timeout` are valid for
        // this wait, and `self.handle` is a live keyed-event handle.
        unsafe { (self.NtWaitForKeyedEvent)(self.handle, key, false.into(), timeout) }
    }

    #[inline]
    unsafe fn release(&self, key: *mut ffi::c_void) -> NTSTATUS {
        // SAFETY: The caller guarantees that `key` identifies a corresponding
        // wait, and `self.handle` is a live keyed-event handle.
        unsafe { (self.NtReleaseKeyedEvent)(self.handle, key, false.into(), ptr::null_mut()) }
    }

    #[allow(non_snake_case)]
    pub fn create() -> Option<KeyedEvent> {
        let ntdll = unsafe { GetModuleHandleA(b"ntdll.dll\0".as_ptr()) };
        if ntdll.is_null() {
            return None;
        }

        let NtCreateKeyedEvent =
            unsafe { GetProcAddress(ntdll, b"NtCreateKeyedEvent\0".as_ptr())? };
        let NtReleaseKeyedEvent =
            unsafe { GetProcAddress(ntdll, b"NtReleaseKeyedEvent\0".as_ptr())? };
        let NtWaitForKeyedEvent =
            unsafe { GetProcAddress(ntdll, b"NtWaitForKeyedEvent\0".as_ptr())? };

        let NtCreateKeyedEvent: NtCreateKeyedEvent = unsafe { mem::transmute(NtCreateKeyedEvent) };
        let mut handle = MaybeUninit::uninit();
        // SAFETY: The function was resolved from ntdll with the matching name,
        // the output pointer is valid, and null object attributes are allowed.
        let status = unsafe {
            NtCreateKeyedEvent(
                handle.as_mut_ptr(),
                GENERIC_READ | GENERIC_WRITE,
                ptr::null_mut(),
                0,
            )
        };
        if status != STATUS_SUCCESS {
            return None;
        }

        Some(KeyedEvent {
            handle: unsafe { handle.assume_init() },
            NtReleaseKeyedEvent: unsafe { mem::transmute(NtReleaseKeyedEvent) },
            NtWaitForKeyedEvent: unsafe { mem::transmute(NtWaitForKeyedEvent) },
        })
    }

    #[inline]
    pub fn prepare_park(&'static self, key: &AtomicUsize) {
        key.store(STATE_PARKED, Ordering::Relaxed);
    }

    #[inline]
    pub fn timed_out(&'static self, key: &AtomicUsize) -> bool {
        key.load(Ordering::Relaxed) == STATE_TIMED_OUT
    }

    #[inline]
    pub unsafe fn park(&'static self, key: &AtomicUsize) {
        // The rendezvous with NtReleaseKeyedEvent provides the synchronization
        // required by ThreadParkerT for the surrounding ThreadData.
        let status = unsafe { self.wait_for(key as *const _ as *mut ffi::c_void, ptr::null_mut()) };
        assert_eq!(status, STATUS_SUCCESS);
    }

    #[inline]
    pub unsafe fn park_until(&'static self, key: &AtomicUsize, timeout: Instant) -> bool {
        loop {
            let now = Instant::now();
            if timeout <= now {
                // If another thread unparked us, we need to call
                // NtWaitForKeyedEvent otherwise that thread will stay stuck at
                // NtReleaseKeyedEvent.
                if key.swap(STATE_TIMED_OUT, Ordering::Relaxed) == STATE_UNPARKED {
                    unsafe { self.park(key) };
                    return true;
                }
                return false;
            }

            // NT uses a timeout in units of 100ns. We use a negative value to
            // indicate a relative timeout based on a monotonic clock.
            let diff = timeout - now;
            let ticks = diff.as_nanos().div_ceil(100).min(i64::MAX as u128);
            let mut nt_timeout = -(ticks as i64);

            // A successful wait synchronizes with NtReleaseKeyedEvent just as
            // in `park` above.
            let status =
                unsafe { self.wait_for(key as *const _ as *mut ffi::c_void, &mut nt_timeout) };
            if status == STATUS_SUCCESS {
                return true;
            }
            assert_eq!(status, STATUS_TIMEOUT);
        }
    }

    #[inline]
    pub unsafe fn unpark_lock(&'static self, key: &AtomicUsize) -> UnparkHandle {
        // If the state was STATE_PARKED then we need to wake up the thread
        if key.swap(STATE_UNPARKED, Ordering::Relaxed) == STATE_PARKED {
            UnparkHandle {
                key: Some(NonNull::from(key)),
                keyed_event: self,
            }
        } else {
            UnparkHandle {
                key: None,
                keyed_event: self,
            }
        }
    }
}

impl Drop for KeyedEvent {
    #[inline]
    fn drop(&mut self) {
        unsafe {
            let ok = CloseHandle(self.handle);
            assert_ne!(ok, false.into());
        }
    }
}

// Handle for a thread that is about to be unparked. We need to mark the thread
// as unparked while holding the queue lock, but we delay the actual unparking
// until after the queue lock is released.
pub struct UnparkHandle {
    key: Option<NonNull<AtomicUsize>>,
    keyed_event: &'static KeyedEvent,
}

impl UnparkHandle {
    // Wakes up the parked thread. This should be called after the queue lock is
    // released to avoid blocking the queue for too long.
    #[inline]
    pub unsafe fn unpark(self) {
        if let Some(key) = self.key {
            // This rendezvous synchronizes with the target's
            // NtWaitForKeyedEvent call.
            let status = unsafe { self.keyed_event.release(key.as_ptr().cast::<ffi::c_void>()) };
            assert_eq!(status, STATUS_SUCCESS);
        }
    }
}
