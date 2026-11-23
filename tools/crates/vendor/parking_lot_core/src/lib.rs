//! This library exposes a low-level API for creating your own efficient
//! synchronization primitives.
//!
//! # The parking lot
//!
//! To keep synchronization primitives small, all thread queuing and suspending
//! functionality is offloaded to the *parking lot*. The idea behind this is based
//! on the WebKit [`WTF::ParkingLot`](https://webkit.org/blog/6161/locking-in-webkit/)
//! class, which essentially consists of a hash table mapping of lock addresses
//! to queues of parked (sleeping) threads. The WebKit parking lot was itself
//! inspired by Linux [futexes](https://man7.org/linux/man-pages/man2/futex.2.html),
//! but it is more powerful since it allows invoking callbacks while holding a
//! queue lock.
//!
//! There are two main operations that can be performed on the parking lot:
//!
//!  - *Parking* refers to suspending the thread while simultaneously enqueuing it
//!    on a queue keyed by some address.
//!  - *Unparking* refers to dequeuing a thread from a queue keyed by some address
//!    and resuming it.
//!
//! See the documentation of the individual functions for more details.
//!
//! Parking-lot operations never unwind. If an operation panics, the process
//! aborts.
//!
//! # Use in global allocators
//!
//! Synchronization primitives built on this parking lot must not be used
//! internally by a global allocator. Parking-lot operations may allocate
//! memory, including while internal locks are held. If the allocator then
//! blocks on one of these primitives, it may recursively invoke itself or
//! deadlock.
//!
//! # Building custom synchronization primitives
//!
//! Building custom synchronization primitives is very simple since the parking
//! lot takes care of all the hard parts for you. A simple example for a
//! custom primitive would be to integrate a `Mutex` inside another data type.
//! Since a mutex only requires 2 bits, it can share space with other data.
//! For example, one could create an `ArcMutex` type that combines the atomic
//! reference count and the two mutex bits in the same atomic word.

#![warn(missing_docs)]
#![warn(rust_2018_idioms)]
#![cfg_attr(
    all(target_env = "sgx", target_vendor = "fortanix"),
    feature(sgx_platform)
)]
#![cfg_attr(
    all(
        feature = "nightly",
        target_family = "wasm",
        target_feature = "atomics"
    ),
    feature(stdarch_wasm_atomic_wait)
)]

mod parking_lot;
mod spinwait;
mod thread_parker;
mod word_lock;

pub use self::parking_lot::deadlock;
pub use self::parking_lot::{DEFAULT_PARK_TOKEN, DEFAULT_UNPARK_TOKEN};
pub use self::parking_lot::{
    FilterOp, ParkResult, ParkToken, RequeueOp, UnparkResult, UnparkToken,
};
pub use self::parking_lot::{park, unpark_all, unpark_filter, unpark_one, unpark_requeue};
pub use self::spinwait::SpinWait;
