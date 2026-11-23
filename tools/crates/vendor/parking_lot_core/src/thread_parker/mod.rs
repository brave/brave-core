use std::time::Instant;

/// Trait for the platform thread parker implementation.
///
/// The unsafe methods form a protocol and implementations may rely on the
/// parker remaining at a stable address once it has been used. In particular,
/// the Unix implementation contains initialized pthread mutexes and condition
/// variables which must not be moved. Calling an unsafe method after moving the
/// parker from the address at which a previous unsafe method was called is
/// undefined behavior.
///
/// The protocol also transfers ownership of the surrounding `ThreadData`
/// between the parked thread and the parking lot. Writes performed by an
/// unparker before `unpark_lock` must be visible after `park` returns or after
/// `park_until` returns true.
pub trait ThreadParkerT {
    type UnparkHandle: UnparkHandleT;

    const IS_CHEAP_TO_CONSTRUCT: bool;

    fn new() -> Self;

    /// Prepares the parker. This must be called while the local thread still
    /// owns the `ThreadData`, before adding it to the queue.
    unsafe fn prepare_park(&self);

    /// Checks if the park timed out. This should be called while holding the
    /// queue lock after `park_until` has returned false.
    unsafe fn timed_out(&self) -> bool;

    /// Parks the thread until it is unparked. This must be called after the
    /// `ThreadData` has been added to the queue and the queue has been unlocked.
    /// Returning transfers ownership of the `ThreadData` back to the local
    /// thread and acquires the writes that preceded `unpark_lock`.
    unsafe fn park(&self);

    /// Parks the thread until it is unparked or the timeout is reached. This
    /// must be called after the `ThreadData` has been added to the queue and the
    /// queue has been unlocked. Returning true transfers ownership of the
    /// `ThreadData` back to the local thread and acquires the writes that
    /// preceded `unpark_lock`. Returning false does not transfer ownership: the
    /// caller must resolve the timeout while holding the queue lock.
    ///
    /// The timeout is the earliest point at which this method may return false,
    /// but scheduling and platform-specific behavior may delay the return.
    unsafe fn park_until(&self, timeout: Instant) -> bool;

    /// Prepares to unpark a thread after removing its `ThreadData` from its
    /// queue and writing its result fields.
    ///
    /// The caller must exclusively own the target `ThreadData`, which must
    /// remain valid and parked until this method is called.
    ///
    /// The returned handle must be consumed by a call to
    /// [`UnparkHandleT::unpark`].
    ///
    /// After this returns, the caller must not access the target `ThreadData`:
    /// the target may immediately observe the unpark and destroy it, even while
    /// the queue remains locked. The returned handle must nevertheless remain
    /// safe to pass to `unpark`. Implementations may either keep the parker
    /// alive or make such a late `unpark` harmless.
    unsafe fn unpark_lock(&self) -> Self::UnparkHandle;
}

/// Handle for a thread whose unpark has been committed. Every handle returned
/// by [`ThreadParkerT::unpark_lock`] must be consumed by a call to
/// [`UnparkHandleT::unpark`]. Any potentially expensive wake operation is
/// delayed until then.
pub trait UnparkHandleT {
    /// Wakes up the parked thread. This should be called after the queue lock is
    /// released to avoid blocking the queue for too long.
    ///
    /// This method is unsafe for the same reason as the unsafe methods in
    /// `ThreadParkerT`.
    unsafe fn unpark(self);
}

cfg_select! {
    any(target_os = "linux", target_os = "android") => {
        #[path = "linux.rs"]
        mod imp;
    }
    unix => {
        #[path = "unix.rs"]
        mod imp;
    }
    windows => {
        #[path = "windows/mod.rs"]
        mod imp;
    }
    target_os = "redox" => {
        #[path = "redox.rs"]
        mod imp;
    }
    all(target_env = "sgx", target_vendor = "fortanix") => {
        #[path = "sgx.rs"]
        mod imp;
    }
    all(
        feature = "nightly",
        target_family = "wasm",
        target_feature = "atomics"
    ) => {
        #[path = "wasm_atomic.rs"]
        mod imp;
    }
    target_os = "wasi" => {
        // WASI provides pthreads with or without the atomics extension. This
        // also works on stable, where target_feature = "atomics" is not set.
        // Keep the intrinsic backend above preferred when it is available.
        #[path = "unix.rs"]
        mod imp;
    }
    target_family = "wasm" => {
        #[path = "wasm.rs"]
        mod imp;
    }
    _ => {
        #[path = "generic.rs"]
        mod imp;
    }
}

pub use self::imp::{ThreadParker, thread_yield};
