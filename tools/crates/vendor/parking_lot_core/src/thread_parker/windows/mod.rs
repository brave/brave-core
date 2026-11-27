use core::sync::atomic::AtomicUsize;
use std::sync::LazyLock;
use std::time::Instant;

mod bindings;
mod keyed_event;
mod waitaddress;

enum Backend {
    KeyedEvent(keyed_event::KeyedEvent),
    WaitAddress(waitaddress::WaitAddress),
}

static BACKEND: LazyLock<Backend> = LazyLock::new(|| {
    if let Some(waitaddress) = waitaddress::WaitAddress::create() {
        Backend::WaitAddress(waitaddress)
    } else if let Some(keyed_event) = keyed_event::KeyedEvent::create() {
        Backend::KeyedEvent(keyed_event)
    } else {
        panic!(
            "parking_lot requires either NT Keyed Events (WinXP+) or \
             WaitOnAddress/WakeByAddress (Win8+)"
        );
    }
});

impl Backend {
    #[inline]
    fn get() -> &'static Backend {
        &BACKEND
    }
}

// Helper type for putting a thread to sleep until some other thread wakes it up
pub struct ThreadParker {
    key: AtomicUsize,
    backend: &'static Backend,
}

impl super::ThreadParkerT for ThreadParker {
    type UnparkHandle = UnparkHandle;

    const IS_CHEAP_TO_CONSTRUCT: bool = true;

    #[inline]
    fn new() -> ThreadParker {
        // Initialize the backend here to ensure we don't get any panics
        // later on, which could leave synchronization primitives in a broken
        // state.
        ThreadParker {
            key: AtomicUsize::new(0),
            backend: Backend::get(),
        }
    }

    // Prepares the parker. This should be called before adding it to the queue.
    #[inline]
    unsafe fn prepare_park(&self) {
        match *self.backend {
            Backend::KeyedEvent(ref x) => x.prepare_park(&self.key),
            Backend::WaitAddress(ref x) => x.prepare_park(&self.key),
        }
    }

    // Checks if the park timed out. This should be called while holding the
    // queue lock after park_until has returned false.
    #[inline]
    unsafe fn timed_out(&self) -> bool {
        match *self.backend {
            Backend::KeyedEvent(ref x) => x.timed_out(&self.key),
            Backend::WaitAddress(ref x) => x.timed_out(&self.key),
        }
    }

    // Parks the thread until it is unparked. This should be called after it has
    // been added to the queue, after unlocking the queue.
    #[inline]
    unsafe fn park(&self) {
        match *self.backend {
            Backend::KeyedEvent(ref x) => unsafe { x.park(&self.key) },
            Backend::WaitAddress(ref x) => x.park(&self.key),
        }
    }

    // Parks the thread until it is unparked or the timeout is reached. This
    // should be called after it has been added to the queue, after unlocking
    // the queue. Returns true if we were unparked and false if we timed out.
    #[inline]
    unsafe fn park_until(&self, timeout: Instant) -> bool {
        match *self.backend {
            Backend::KeyedEvent(ref x) => unsafe { x.park_until(&self.key, timeout) },
            Backend::WaitAddress(ref x) => x.park_until(&self.key, timeout),
        }
    }

    // Marks the thread as unparked while holding the queue lock and returns a
    // backend-specific handle for the delayed wake.
    #[inline]
    unsafe fn unpark_lock(&self) -> UnparkHandle {
        match *self.backend {
            Backend::KeyedEvent(ref x) => {
                UnparkHandle::KeyedEvent(unsafe { x.unpark_lock(&self.key) })
            }
            Backend::WaitAddress(ref x) => UnparkHandle::WaitAddress(x.unpark_lock(&self.key)),
        }
    }
}

// Handle for a thread that is about to be unparked. We need to mark the thread
// as unparked while holding the queue lock, but we delay the actual unparking
// until after the queue lock is released.
pub enum UnparkHandle {
    KeyedEvent(keyed_event::UnparkHandle),
    WaitAddress(waitaddress::UnparkHandle),
}

impl super::UnparkHandleT for UnparkHandle {
    // Wakes up the parked thread. This should be called after the queue lock is
    // released to avoid blocking the queue for too long.
    #[inline]
    unsafe fn unpark(self) {
        match self {
            UnparkHandle::KeyedEvent(x) => unsafe { x.unpark() },
            UnparkHandle::WaitAddress(x) => x.unpark(),
        }
    }
}

// Yields the rest of the current timeslice to the OS
#[inline]
pub fn thread_yield() {
    unsafe {
        // We don't use SwitchToThread here because it doesn't consider all
        // threads in the system and the thread we are waiting for may not get
        // selected.
        bindings::Sleep(0);
    }
}
