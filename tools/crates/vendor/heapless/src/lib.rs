//! `static` friendly data structures that don't require dynamic memory allocation
//!
//! The core principle behind `heapless` is that its data structures are backed by a *static* memory
//! allocation. For example, you can think of `heapless::Vec` as an alternative version of
//! `std::Vec` with fixed capacity and that can't be re-allocated on the fly (e.g. via `push`).
//!
//! All `heapless` data structures store their memory allocation *inline* and specify their capacity
//! via their type parameter `N`. This means that you can instantiate a `heapless` data structure on
//! the stack, in a `static` variable, or even in the heap.
//!
//! ```
//! use heapless::Vec; // fixed capacity `std::Vec`
//!
//! // on the stack
//! let mut xs: Vec<u8, 8> = Vec::new(); // can hold up to 8 elements
//! xs.push(42).unwrap();
//! assert_eq!(xs.pop(), Some(42));
//!
//! // in a `static` variable
//! static mut XS: Vec<u8, 8> = Vec::new();
//!
//! let xs = unsafe { &mut XS };
//!
//! xs.push(42);
//! assert_eq!(xs.pop(), Some(42));
//!
//! // in the heap (though kind of pointless because no reallocation)
//! let mut ys: Box<Vec<u8, 8>> = Box::new(Vec::new());
//! ys.push(42).unwrap();
//! assert_eq!(ys.pop(), Some(42));
//! ```
//!
//! Because they have fixed capacity `heapless` data structures don't implicitly reallocate. This
//! means that operations like `heapless::Vec.push` are *truly* constant time rather than amortized
//! constant time with potentially unbounded (depends on the allocator) worst case execution time
//! (which is bad / unacceptable for hard real time applications).
//!
//! `heapless` data structures don't use a memory allocator which means no risk of an uncatchable
//! Out Of Memory (OOM) condition while performing operations on them. It's certainly possible to
//! run out of capacity while growing `heapless` data structures, but the API lets you handle this
//! possibility by returning a `Result` on operations that may exhaust the capacity of the data
//! structure.
//!
//! List of currently implemented data structures:
//!
#![cfg_attr(
    any(arm_llsc, target_arch = "x86"),
    doc = "- [`Arc`](pool::arc::Arc) -- like `std::sync::Arc` but backed by a lock-free memory pool rather than `#[global_allocator]`"
)]
#![cfg_attr(
    any(arm_llsc, target_arch = "x86"),
    doc = "- [`Box`](pool::boxed::Box) -- like `std::boxed::Box` but backed by a lock-free memory pool rather than `#[global_allocator]`"
)]
//! - [`BinaryHeap`] -- priority queue
//! - [`IndexMap`] -- hash table
//! - [`IndexSet`] -- hash set
//! - [`LinearMap`]
#![cfg_attr(
    any(arm_llsc, target_arch = "x86"),
    doc = "- [`Object`](pool::object::Object) -- objects managed by an object pool"
)]
//! - [`String`]
//! - [`Vec`]
//! - [`mpmc::Q*`](mpmc) -- multiple producer multiple consumer lock-free queue
//! - [`spsc::Queue`] -- single producer single consumer lock-free queue
//!
//! # Optional Features
//!
//! The `heapless` crate provides the following optional Cargo features:
//!
//! - `ufmt`: Implement [`ufmt_write::uWrite`] for `String<N>` and `Vec<u8, N>`
//!
//! [`ufmt_write::uWrite`]: https://docs.rs/ufmt-write/
//!
//! # Minimum Supported Rust Version (MSRV)
//!
//! This crate does *not* have a Minimum Supported Rust Version (MSRV) and may make use of language
//! features and API in the standard library available in the latest stable Rust version.
//!
//! In other words, changes in the Rust version requirement of this crate are not considered semver
//! breaking change and may occur in patch version releases.
#![cfg_attr(docsrs, feature(doc_cfg), feature(doc_auto_cfg))]
#![cfg_attr(not(test), no_std)]
#![deny(missing_docs)]
#![deny(warnings)]

pub use binary_heap::BinaryHeap;
pub use deque::Deque;
pub use histbuf::{HistoryBuffer, OldestOrdered};
pub use indexmap::{
    Bucket, Entry, FnvIndexMap, IndexMap, Iter as IndexMapIter, IterMut as IndexMapIterMut,
    Keys as IndexMapKeys, OccupiedEntry, Pos, VacantEntry, Values as IndexMapValues,
    ValuesMut as IndexMapValuesMut,
};
pub use indexset::{FnvIndexSet, IndexSet, Iter as IndexSetIter};
pub use linear_map::LinearMap;
pub use string::String;
pub use vec::Vec;

#[macro_use]
#[cfg(test)]
mod test_helpers;

mod deque;
mod histbuf;
mod indexmap;
mod indexset;
mod linear_map;
mod string;
mod vec;

#[cfg(feature = "serde")]
mod de;
#[cfg(feature = "serde")]
mod ser;

pub mod binary_heap;
#[cfg(feature = "defmt-03")]
mod defmt;
#[cfg(any(
    // assume we have all atomics available if we're using portable-atomic
    feature = "portable-atomic",
    // target has native atomic CAS (mpmc_large requires usize, otherwise just u8)
    all(feature = "mpmc_large", target_has_atomic = "ptr"),
    all(not(feature = "mpmc_large"), target_has_atomic = "8")
))]
pub mod mpmc;
#[cfg(any(arm_llsc, target_arch = "x86"))]
pub mod pool;
pub mod sorted_linked_list;
#[cfg(any(
    // assume we have all atomics available if we're using portable-atomic
    feature = "portable-atomic",
    // target has native atomic CAS. Note this is too restrictive, spsc requires load/store only, not CAS.
    // This should be `cfg(target_has_atomic_load_store)`, but that's not stable yet.
    target_has_atomic = "ptr",
    // or the current target is in a list in build.rs of targets known to have load/store but no CAS.
    has_atomic_load_store
))]
pub mod spsc;

#[cfg(feature = "ufmt")]
mod ufmt;

mod sealed;
