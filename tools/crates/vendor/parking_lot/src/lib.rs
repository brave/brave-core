//! This library provides compact and efficient implementations of [`Mutex`],
//! [`FairMutex`], [`ReentrantMutex`], [`RwLock`], [`RecursiveRwLock`],
//! [`Condvar`] and [`Once`].
//!
//! # Use in global allocators
//!
//! The synchronization primitives in this crate must not be used internally by
//! a global allocator. Their contended paths may allocate memory, including
//! while internal parking-lot locks are held. If the allocator then blocks on
//! one of these primitives, it may recursively invoke itself or deadlock.

#![warn(missing_docs)]
#![warn(rust_2018_idioms)]

mod condvar;
mod fair_mutex;
mod mutex;
mod once;
mod raw_condvar;
mod raw_fair_mutex;
mod raw_mutex;
mod raw_rwlock;
mod recursive_rwlock;
mod remutex;
mod rwlock;
mod util;

#[cfg(feature = "deadlock_detection")]
pub mod deadlock;
#[cfg(not(feature = "deadlock_detection"))]
mod deadlock;

// Deadlock detection records lock ownership per thread, so guards cannot be
// sent to another thread while it is enabled.
#[cfg(all(feature = "send_guard", feature = "deadlock_detection"))]
compile_error!("the `send_guard` and `deadlock_detection` features cannot be used together");
#[cfg(feature = "send_guard")]
type GuardMarker = lock_api::GuardSend;
#[cfg(not(feature = "send_guard"))]
type GuardMarker = lock_api::GuardNoSend;

pub use self::condvar::{Condvar, WaitTimeoutResult};
pub use self::fair_mutex::{FairMutex, FairMutexGuard, MappedFairMutexGuard};
pub use self::mutex::{MappedMutexGuard, Mutex, MutexGuard};
pub use self::once::{Once, OnceState};
pub use self::raw_condvar::RawCondvar;
pub use self::raw_fair_mutex::RawFairMutex;
pub use self::raw_mutex::RawMutex;
pub use self::raw_rwlock::{RawRwLock, RawRwLockRecursive};
pub use self::recursive_rwlock::{
    MappedRecursiveRwLockReadGuard, MappedRecursiveRwLockWriteGuard, RecursiveRwLock,
    RecursiveRwLockReadGuard, RecursiveRwLockUpgradableReadGuard, RecursiveRwLockWriteGuard,
};
pub use self::remutex::{
    MappedReentrantMutexGuard, RawThreadId, ReentrantMutex, ReentrantMutexGuard,
};
pub use self::rwlock::{
    MappedRwLockReadGuard, MappedRwLockWriteGuard, RwLock, RwLockReadGuard,
    RwLockUpgradableReadGuard, RwLockWriteGuard,
};
pub use ::lock_api;

#[cfg(feature = "arc_lock")]
pub use self::lock_api::{
    ArcMutexGuard, ArcReentrantMutexGuard, ArcRwLockReadGuard, ArcRwLockUpgradableReadGuard,
    ArcRwLockWriteGuard,
};

#[cfg(all(test, feature = "send_guard"))]
#[test]
fn test_send_guards() {
    fn assert_send<T: Send>() {}

    assert_send::<MutexGuard<'static, ()>>();
    assert_send::<MappedMutexGuard<'static, ()>>();
    assert_send::<FairMutexGuard<'static, ()>>();
    assert_send::<MappedFairMutexGuard<'static, ()>>();
    assert_send::<RwLockReadGuard<'static, ()>>();
    assert_send::<RwLockWriteGuard<'static, ()>>();
    assert_send::<RwLockUpgradableReadGuard<'static, ()>>();
    assert_send::<MappedRwLockReadGuard<'static, ()>>();
    assert_send::<MappedRwLockWriteGuard<'static, ()>>();
    assert_send::<RecursiveRwLockReadGuard<'static, ()>>();
    assert_send::<RecursiveRwLockWriteGuard<'static, ()>>();
    assert_send::<RecursiveRwLockUpgradableReadGuard<'static, ()>>();
    assert_send::<MappedRecursiveRwLockReadGuard<'static, ()>>();
    assert_send::<MappedRecursiveRwLockWriteGuard<'static, ()>>();
}
