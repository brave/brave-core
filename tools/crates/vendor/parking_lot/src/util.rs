use std::time::{Duration, Instant};

/// Runs `f`, aborting the process if it unwinds.
///
/// This is used after publishing a raw lock ownership transition which cannot
/// be rolled back safely.
#[inline]
pub(crate) fn abort_on_panic<T>(f: impl FnOnce() -> T) -> T {
    struct AbortOnDrop;

    impl Drop for AbortOnDrop {
        fn drop(&mut self) {
            panic!("aborting due to panic while changing lock ownership");
        }
    }

    let guard = AbortOnDrop;
    let result = f();
    core::mem::forget(guard);
    result
}

#[inline]
pub fn to_deadline(timeout: Duration) -> Option<Instant> {
    Instant::now().checked_add(timeout)
}
