//! This library provides type-safe and fully-featured [`Mutex`], [`RwLock`],
//! and [`Condvar`] types which wrap simple raw synchronization primitives. This
//! has several
//! benefits: not only does it eliminate a large portion of the work in
//! implementing custom lock types, it also allows users to write code which is
//! generic with regard to different lock implementations.
//!
//! Basic usage of this crate is very straightforward:
//!
//! 1. Create a raw lock type. This should only contain the lock state, not any
//!    data protected by the lock.
//! 2. Implement the `RawMutex` trait for your custom lock type.
//! 3. Export your mutex as a type alias for `lock_api::Mutex`, and
//!    your mutex guard as a type alias for `lock_api::MutexGuard`.
//!    See the [example](#example) below for details.
//!
//! This process is similar for [`RwLock`]s, except that two guards need to be
//! exported instead of one (or three guards if your type supports upgradable
//! read locks; see [extension traits](#extension-traits) below for details).
//! A [`Condvar`] additionally requires a [`RawCondvar`] implementation tied to
//! the raw mutex type with which it can be used.
//!
//! # Example
//!
//! ```
//! use lock_api::{RawMutex, Mutex, GuardSend};
//! use std::sync::atomic::{AtomicBool, Ordering};
//!
//! // 1. Define our raw lock type
//! pub struct RawSpinlock(AtomicBool);
//!
//! // 2. Implement RawMutex for this type
//! unsafe impl RawMutex for RawSpinlock {
//!     const INIT: RawSpinlock = RawSpinlock(AtomicBool::new(false));
//!
//!     // A spinlock guard can be sent to another thread and unlocked there
//!     type Guard = GuardSend;
//!
//!     fn lock(&self) -> Self::Guard {
//!         // Note: This isn't the best way of implementing a spinlock, but it
//!         // suffices for the sake of this example.
//!         loop {
//!             if let Some(guard) = self.try_lock() {
//!                 return guard;
//!             }
//!         }
//!     }
//!
//!     fn try_lock(&self) -> Option<Self::Guard> {
//!         self.0
//!             .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
//!             .ok().map(|_| GuardSend::new())
//!     }
//!
//!     unsafe fn unlock(&self, _guard: Self::Guard) {
//!         self.0.store(false, Ordering::Release);
//!     }
//!
//!     fn is_locked(&self) -> bool {
//!         self.0.load(Ordering::Relaxed)
//!     }
//! }
//!
//! // 3. Export the wrappers. These are the types that users will actually use.
//! pub type Spinlock<T> = lock_api::Mutex<RawSpinlock, T>;
//! pub type SpinlockGuard<'a, T> = lock_api::MutexGuard<'a, RawSpinlock, T>;
//! ```
//!
//! # Extension traits
//!
//! In addition to basic locking & unlocking functionality, you have the option
//! of exposing additional functionality in your lock types by implementing
//! additional traits for it. Examples of extension features include:
//!
//! - Fair unlocking ([`RawMutexFair`], [`RawRwLockFair`])
//! - Lock timeouts ([`RawMutexTimed`], [`RawRwLockTimed`])
//! - Condition-variable timeouts ([`RawCondvarTimed`])
//! - Downgradable write locks ([`RawRwLockDowngrade`])
//! - Upgradable read locks ([`RawRwLockUpgrade`])
//!
//! The `Mutex` and `RwLock` wrappers will automatically expose this additional
//! functionality if the raw lock type implements these extension traits.
//!
//! # Cargo features
//!
//! This crate provides the following Cargo features:
//!
//! - `atomic_usize`: Enables reentrant mutex types, which require pointer-sized
//!   atomic operations. This feature is enabled by default.
//! - `arc_lock`: Enables locking from an `Arc` and types such as
//!   `ArcMutexGuard`. This requires the `alloc` crate.
//! - `owning_ref`: Allows lock guards to be used with the `owning_ref` crate.
//! - `serde`: Enables serialization and deserialization of lock types.

#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![warn(missing_docs)]
#![warn(rust_2018_idioms)]

#[cfg(feature = "arc_lock")]
extern crate alloc;

/// A stateless guard which is [`Send`].
#[derive(Default)]
pub struct GuardSend(());

impl GuardSend {
    /// Creates a stateless guard which can be sent to another thread.
    #[inline]
    pub const fn new() -> Self {
        Self(())
    }
}

/// A stateless guard which is not [`Send`].
#[derive(Default)]
pub struct GuardNoSend(core::marker::PhantomData<*mut ()>);

impl GuardNoSend {
    /// Creates a stateless guard which cannot be sent to another thread.
    #[inline]
    pub const fn new() -> Self {
        Self(core::marker::PhantomData)
    }
}

unsafe impl Sync for GuardNoSend {}

// scopeguard's defer! defines its closure in Rust 2015, so it captures entire
// variables instead of individual fields. Define it locally to use our
// edition's precise captures, allowing cleanup to borrow one field while
// another is used.
macro_rules! defer {
    ($($t:tt)*) => {
        let _guard = scopeguard::guard((), |()| { $($t)* });
    };
}

mod guard;
mod mutex;
pub use crate::mutex::*;

#[cfg(feature = "atomic_usize")]
mod remutex;
#[cfg(feature = "atomic_usize")]
pub use crate::remutex::*;

mod rwlock;
pub use crate::rwlock::*;

mod condvar;
pub use crate::condvar::*;
