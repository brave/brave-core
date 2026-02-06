//! A priority queue implemented with a *d*-ary heap.
//!
//! Insertion and popping the largest element have *O*(log(*n*)) time complexity.
//! Checking the largest element is *O*(1). Converting a vector to a *d*-ary heap
//! can be done in-place, and has *O*(*n*) complexity. A *d*-ary heap can also be
//! converted to a sorted vector in-place, allowing it to be used for an *O*(*n* * log(*n*))
//! in-place heapsort.
//!
//! # Comparison to standard library
//!
//! The standard library contains a 2-ary heap
//! ([`std::collections::BinaryHeap`][std]). The [`BinaryHeap`] of this crate
//! aims to be a drop-in replacement, both in API and in performance. Cargo
//! features are used in place of unstable Rust features. The advantage of this
//! crate over the standard library lies in the possibility of easily changing
//! the arity of the heap, which can increase performance.
//!
//! The standard library binary heap can contain up to [`isize::MAX`] elements;
//! this is the same for the binary heap of this crate, but other heaps in this
//! crate can hold less elements. In the general case, the maximum number of
//! elements is ([`usize::MAX`] - 1) / *d* for an arity of *d*. On 64-bit systems
//! this should generally not be a concern when using reasonable arities. On
//! 32-bit systems this may be a concern when using very large heaps with a
//! relatively high arity.
//!
//! [std]: https://doc.rust-lang.org/std/collections/struct.BinaryHeap.html
//!
//! # Comparison of different arities *d*
//!
//! The arity *d* is defined as the maximum number of children each node can
//! have. A higher number means the heap has less layers, but may require more
//! work per layer because there are more children present. This generally makes
//! methods adding elements to the heap such as [`push`] faster, and methods
//! removing them such as [`pop`] slower. However, due to higher cache locality
//! for higher *d*, the drop in [`pop`] performance is often diminished. If you're
//! unsure what value of *d* to choose, the [`QuaternaryHeap`] with *d* = 4 is
//! usually a good start, but benchmarking is necessary to determine the best
//! value of *d*.
//!
//! [`push`]: struct.DaryHeap.html#method.push
//! [`pop`]: struct.DaryHeap.html#method.pop
//!
//! # Usage
//!
//! Rust type interference cannot infer the desired heap arity (value of *d*)
//! automatically when using [`DaryHeap`] directly. It is therefore more
//! ergonomic to  use one of the type aliases to select the desired arity:
//!
//! | Name               | Arity   |
//! |--------------------|---------|
//! | [`BinaryHeap`]     | *d* = 2 |
//! | [`TernaryHeap`]    | *d* = 3 |
//! | [`QuaternaryHeap`] | *d* = 4 |
//! | [`QuinaryHeap`]    | *d* = 5 |
//! | [`SenaryHeap`]     | *d* = 6 |
//! | [`SeptenaryHeap`]  | *d* = 7 |
//! | [`OctonaryHeap`]   | *d* = 8 |
//!
//! The difference in ergonomics illustrated in the following:
//!
//! ```
//! use dary_heap::{DaryHeap, TernaryHeap};
//!
//! // Type parameter T can be inferred, but arity cannot
//! let mut heap1 = DaryHeap::<_, 3>::new();
//! heap1.push(42);
//!
//! // Type alias removes need for explicit type
//! let mut heap2 = TernaryHeap::new();
//! heap2.push(42);
//! ```
//!
//! If a different arity is desired, you can use the former or a define a type
//! alias yourself. It should be noted that *d* > 8 is rarely beneficial.
//!
//! ## Validity of arities in *d*-ary heaps
//!
//! Only arities of two or greater are useful in *d*-ary heap, and are therefore
//! the only ones implemented by default. Lower arities are only possible if you
//! put in the effort to implement them yourself. An arity of one is possible,
//! but yields a heap where every element has one child. This essentially makes
//! it a sorted vector with poor performance. Regarding an arity of zero: this
//! is not statically prevented, but constructing a [`DaryHeap`] with it and
//! using it may (and probably will) result in a runtime panic.
//!
//! [`DaryHeap`]: struct.DaryHeap.html
//! [`BinaryHeap`]: type.BinaryHeap.html
//! [`TernaryHeap`]: type.TernaryHeap.html
//! [`QuaternaryHeap`]: type.QuaternaryHeap.html
//! [`QuinaryHeap`]: type.QuinaryHeap.html
//! [`SenaryHeap`]: type.SenaryHeap.html
//! [`SeptenaryHeap`]: type.SeptenaryHeap.html
//! [`OctonaryHeap`]: type.OctonaryHeap.html
//!
//! # Examples
//!
//! This is a larger example that implements [Dijkstra's algorithm][dijkstra]
//! to solve the [shortest path problem][sssp] on a [directed graph][dir_graph].
//! It shows how to use [`DaryHeap`] with custom types.
//!
//! [dijkstra]: https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm
//! [sssp]: https://en.wikipedia.org/wiki/Shortest_path_problem
//! [dir_graph]: https://en.wikipedia.org/wiki/Directed_graph
//!
//! ```
//! use std::cmp::Ordering;
//! use dary_heap::TernaryHeap;
//!
//! #[derive(Copy, Clone, Eq, PartialEq)]
//! struct State {
//!     cost: usize,
//!     position: usize,
//! }
//!
//! // The priority queue depends on `Ord`.
//! // Explicitly implement the trait so the queue becomes a min-heap
//! // instead of a max-heap.
//! impl Ord for State {
//!     fn cmp(&self, other: &Self) -> Ordering {
//!         // Notice that we flip the ordering on costs.
//!         // In case of a tie we compare positions - this step is necessary
//!         // to make implementations of `PartialEq` and `Ord` consistent.
//!         other.cost.cmp(&self.cost)
//!             .then_with(|| self.position.cmp(&other.position))
//!     }
//! }
//!
//! // `PartialOrd` needs to be implemented as well.
//! impl PartialOrd for State {
//!     fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
//!         Some(self.cmp(other))
//!     }
//! }
//!
//! // Each node is represented as a `usize`, for a shorter implementation.
//! struct Edge {
//!     node: usize,
//!     cost: usize,
//! }
//!
//! // Dijkstra's shortest path algorithm.
//!
//! // Start at `start` and use `dist` to track the current shortest distance
//! // to each node. This implementation isn't memory-efficient as it may leave duplicate
//! // nodes in the queue. It also uses `usize::MAX` as a sentinel value,
//! // for a simpler implementation.
//! fn shortest_path(adj_list: &Vec<Vec<Edge>>, start: usize, goal: usize) -> Option<usize> {
//!     // dist[node] = current shortest distance from `start` to `node`
//!     let mut dist: Vec<_> = (0..adj_list.len()).map(|_| usize::MAX).collect();
//!
//!     let mut heap = TernaryHeap::new();
//!
//!     // We're at `start`, with a zero cost
//!     dist[start] = 0;
//!     heap.push(State { cost: 0, position: start });
//!
//!     // Examine the frontier with lower cost nodes first (min-heap)
//!     while let Some(State { cost, position }) = heap.pop() {
//!         // Alternatively we could have continued to find all shortest paths
//!         if position == goal { return Some(cost); }
//!
//!         // Important as we may have already found a better way
//!         if cost > dist[position] { continue; }
//!
//!         // For each node we can reach, see if we can find a way with
//!         // a lower cost going through this node
//!         for edge in &adj_list[position] {
//!             let next = State { cost: cost + edge.cost, position: edge.node };
//!
//!             // If so, add it to the frontier and continue
//!             if next.cost < dist[next.position] {
//!                 heap.push(next);
//!                 // Relaxation, we have now found a better way
//!                 dist[next.position] = next.cost;
//!             }
//!         }
//!     }
//!
//!     // Goal not reachable
//!     None
//! }
//!
//! fn main() {
//!     // This is the directed graph we're going to use.
//!     // The node numbers correspond to the different states,
//!     // and the edge weights symbolize the cost of moving
//!     // from one node to another.
//!     // Note that the edges are one-way.
//!     //
//!     //                  7
//!     //          +-----------------+
//!     //          |                 |
//!     //          v   1        2    |  2
//!     //          0 -----> 1 -----> 3 ---> 4
//!     //          |        ^        ^      ^
//!     //          |        | 1      |      |
//!     //          |        |        | 3    | 1
//!     //          +------> 2 -------+      |
//!     //           10      |               |
//!     //                   +---------------+
//!     //
//!     // The graph is represented as an adjacency list where each index,
//!     // corresponding to a node value, has a list of outgoing edges.
//!     // Chosen for its efficiency.
//!     let graph = vec![
//!         // Node 0
//!         vec![Edge { node: 2, cost: 10 },
//!              Edge { node: 1, cost: 1 }],
//!         // Node 1
//!         vec![Edge { node: 3, cost: 2 }],
//!         // Node 2
//!         vec![Edge { node: 1, cost: 1 },
//!              Edge { node: 3, cost: 3 },
//!              Edge { node: 4, cost: 1 }],
//!         // Node 3
//!         vec![Edge { node: 0, cost: 7 },
//!              Edge { node: 4, cost: 2 }],
//!         // Node 4
//!         vec![]];
//!
//!     assert_eq!(shortest_path(&graph, 0, 1), Some(1));
//!     assert_eq!(shortest_path(&graph, 0, 3), Some(3));
//!     assert_eq!(shortest_path(&graph, 3, 0), Some(7));
//!     assert_eq!(shortest_path(&graph, 0, 4), Some(5));
//!     assert_eq!(shortest_path(&graph, 4, 0), None);
//! }
//! ```

#![no_std]
#![cfg_attr(
    feature = "unstable_nightly",
    feature(
        exact_size_is_empty,
        extend_one,
        inplace_iteration,
        min_specialization,
        trusted_fused,
        trusted_len
    )
)]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![allow(
    unknown_lints,
    non_local_definitions,
    unexpected_cfgs,
    clippy::needless_doctest_main
)]

extern crate alloc;

use core::iter::{FromIterator, FusedIterator};
use core::mem::{size_of, swap, ManuallyDrop};
use core::num::NonZeroUsize;
use core::ops::{Deref, DerefMut};
use core::{fmt, ptr, slice};

#[cfg(feature = "extra")]
use alloc::collections::TryReserveError;
use alloc::{vec, vec::Vec};

/// A binary heap (*d* = 2).
pub type BinaryHeap<T> = DaryHeap<T, 2>;

/// A ternary heap (*d* = 3).
pub type TernaryHeap<T> = DaryHeap<T, 3>;

/// A quaternary heap (*d* = 4).
pub type QuaternaryHeap<T> = DaryHeap<T, 4>;

/// A quinary heap (*d* = 5).
pub type QuinaryHeap<T> = DaryHeap<T, 5>;

/// A senary heap (*d* = 6).
pub type SenaryHeap<T> = DaryHeap<T, 6>;

/// A septenary heap (*d* = 7).
pub type SeptenaryHeap<T> = DaryHeap<T, 7>;

/// An octonary heap (*d* = 8).
pub type OctonaryHeap<T> = DaryHeap<T, 8>;

/// A priority queue implemented with a *d*-ary heap.
///
/// This will be a max-heap.
///
/// It is a logic error for an item to be modified in such a way that the
/// item's ordering relative to any other item, as determined by the [`Ord`]
/// trait, changes while it is in the heap. This is normally only possible
/// through interior mutability, global state, I/O, or unsafe code. The
/// behavior resulting from such a logic error is not specified, but will
/// be encapsulated to the `DaryHeap` that observed the logic error and not
/// result in undefined behavior. This could include panics, incorrect results,
/// aborts, memory leaks, and non-termination.
///
/// As long as no elements change their relative order while being in the heap
/// as described above, the API of `DaryHeap` guarantees that the heap
/// invariant remains intact i.e. its methods all behave as documented. For
/// example if a method is documented as iterating in sorted order, that's
/// guaranteed to work as long as elements in the heap have not changed order,
/// even in the presence of closures getting unwinded out of, iterators getting
/// leaked, and similar foolishness.
///
///
/// # Usage
///
/// Rust type interference cannot infer the desired heap arity (value of *d*)
/// automatically. Therefore, it is generally more ergonomic to use one of the
/// [type aliases] instead of `DaryHeap` directly. See the [crate-level
/// documentation][usage] for more information.
///
/// [type aliases]: index.html#types
/// [usage]: index.html#usage
///
/// # Comparison to standard library
///
/// For a comparison with [`std::collections::BinaryHeap`][std], see the [crate-level
/// documentation][comparison].
///
/// [std]: https://doc.rust-lang.org/std/collections/struct.BinaryHeap.html
/// [comparison]: index.html#comparison-to-standard-library
///
/// # Examples
///
/// ```
/// use dary_heap::BinaryHeap;
///
/// // Type inference lets us omit an explicit type signature (which
/// // would be `BinaryHeap<i32>` in this example).
/// let mut heap = BinaryHeap::new();
///
/// // We can use peek to look at the next item in the heap. In this case,
/// // there's no items in there yet so we get None.
/// assert_eq!(heap.peek(), None);
///
/// // Let's add some scores...
/// heap.push(1);
/// heap.push(5);
/// heap.push(2);
///
/// // Now peek shows the most important item in the heap.
/// assert_eq!(heap.peek(), Some(&5));
///
/// // We can check the length of a heap.
/// assert_eq!(heap.len(), 3);
///
/// // We can iterate over the items in the heap, although they are returned in
/// // a random order.
/// for x in &heap {
///     println!("{x}");
/// }
///
/// // If we instead pop these scores, they should come back in order.
/// assert_eq!(heap.pop(), Some(5));
/// assert_eq!(heap.pop(), Some(2));
/// assert_eq!(heap.pop(), Some(1));
/// assert_eq!(heap.pop(), None);
///
/// // We can clear the heap of any remaining items.
/// heap.clear();
///
/// // The heap should now be empty.
/// assert!(heap.is_empty())
/// ```
///
/// A `DaryHeap` with a known list of items can be initialized from an array:
///
/// ```
/// use dary_heap::QuaternaryHeap;
///
/// let heap = QuaternaryHeap::from([1, 5, 2]);
/// ```
///
/// ## Min-heap
///
/// Either [`core::cmp::Reverse`] or a custom [`Ord`] implementation can be used to
/// make `DaryHeap` a min-heap. This makes `heap.pop()` return the smallest
/// value instead of the greatest one.
///
/// ```
/// use dary_heap::TernaryHeap;
/// use std::cmp::Reverse;
///
/// let mut heap = TernaryHeap::new();
///
/// // Wrap values in `Reverse`
/// heap.push(Reverse(1));
/// heap.push(Reverse(5));
/// heap.push(Reverse(2));
///
/// // If we pop these scores now, they should come back in the reverse order.
/// assert_eq!(heap.pop(), Some(Reverse(1)));
/// assert_eq!(heap.pop(), Some(Reverse(2)));
/// assert_eq!(heap.pop(), Some(Reverse(5)));
/// assert_eq!(heap.pop(), None);
/// ```
///
/// # Time complexity
///
/// | [push]  | [pop]         | [peek]/[peek\_mut] |
/// |---------|---------------|--------------------|
/// | *O*(1)~ | *O*(log(*n*)) | *O*(1)             |
///
/// The value for `push` is an expected cost; the method documentation gives a
/// more detailed analysis.
///
/// [`core::cmp::Reverse`]: core::cmp::Reverse
/// [`Cell`]: core::cell::Cell
/// [`RefCell`]: core::cell::RefCell
/// [push]: DaryHeap::push
/// [pop]: DaryHeap::pop
/// [peek]: DaryHeap::peek
/// [peek\_mut]: DaryHeap::peek_mut
pub struct DaryHeap<T, const D: usize> {
    data: Vec<T>,
}

#[cfg(feature = "serde")]
mod serde_impl {
    use super::{DaryHeap, Vec};
    use serde::{Deserialize, Deserializer, Serialize, Serializer};

    impl<T: Serialize, const D: usize> Serialize for DaryHeap<T, D> {
        fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
        {
            self.data.serialize(serializer)
        }
    }

    impl<'de, T: Ord + Deserialize<'de>, const A: usize> Deserialize<'de> for DaryHeap<T, A> {
        fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: Deserializer<'de>,
        {
            Vec::deserialize(deserializer).map(Into::into)
        }

        fn deserialize_in_place<D>(deserializer: D, place: &mut Self) -> Result<(), D::Error>
        where
            D: Deserializer<'de>,
        {
            place.data.clear();
            let result = Vec::deserialize_in_place(deserializer, &mut place.data);
            place.rebuild();
            result
        }
    }
}

/// Structure wrapping a mutable reference to the greatest item on a
/// `DaryHeap`.
///
/// This `struct` is created by the [`peek_mut`] method on [`DaryHeap`]. See
/// its documentation for more.
///
/// [`peek_mut`]: DaryHeap::peek_mut
pub struct PeekMut<'a, T: 'a + Ord, const D: usize> {
    heap: &'a mut DaryHeap<T, D>,
    // If a set_len + sift_down are required, this is Some. If a &mut T has not
    // yet been exposed to peek_mut()'s caller, it's None.
    original_len: Option<NonZeroUsize>,
}

impl<T: Ord + fmt::Debug, const D: usize> fmt::Debug for PeekMut<'_, T, D> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_tuple("PeekMut").field(&self.heap.data[0]).finish()
    }
}

impl<T: Ord, const D: usize> Drop for PeekMut<'_, T, D> {
    fn drop(&mut self) {
        if let Some(original_len) = self.original_len {
            // SAFETY: That's how many elements were in the Vec at the time of
            // the PeekMut::deref_mut call, and therefore also at the time of
            // the BinaryHeap::peek_mut call. Since the PeekMut did not end up
            // getting leaked, we are now undoing the leak amplification that
            // the DerefMut prepared for.
            unsafe { self.heap.data.set_len(original_len.get()) };

            // SAFETY: PeekMut is only instantiated for non-empty heaps.
            unsafe { self.heap.sift_down(0) };
        }
    }
}

impl<T: Ord, const D: usize> Deref for PeekMut<'_, T, D> {
    type Target = T;
    fn deref(&self) -> &T {
        debug_assert!(!self.heap.is_empty());
        // SAFE: PeekMut is only instantiated for non-empty heaps
        unsafe { self.heap.data.get_unchecked(0) }
    }
}

impl<T: Ord, const D: usize> DerefMut for PeekMut<'_, T, D> {
    fn deref_mut(&mut self) -> &mut T {
        debug_assert!(!self.heap.is_empty());

        let len = self.heap.len();
        if len > 1 {
            // Here we preemptively leak all the rest of the underlying vector
            // after the currently max element. If the caller mutates the &mut T
            // we're about to give them, and then leaks the PeekMut, all these
            // elements will remain leaked. If they don't leak the PeekMut, then
            // either Drop or PeekMut::pop will un-leak the vector elements.
            //
            // This is technique is described throughout several other places in
            // the standard library as "leak amplification".
            unsafe {
                // SAFETY: len > 1 so len != 0.
                self.original_len = Some(NonZeroUsize::new_unchecked(len));
                // SAFETY: len > 1 so all this does for now is leak elements,
                // which is safe.
                self.heap.data.set_len(1);
            }
        }

        // SAFE: PeekMut is only instantiated for non-empty heaps
        unsafe { self.heap.data.get_unchecked_mut(0) }
    }
}

impl<'a, T: Ord, const D: usize> PeekMut<'a, T, D> {
    /// Sifts the current element to its new position.
    ///
    /// Afterwards refers to the new element. Returns if the element changed.
    ///
    /// ## Examples
    ///
    /// The condition can be used to upper bound all elements in the heap. When only few elements
    /// are affected, the heap's sort ensures this is faster than a reconstruction from the raw
    /// element list and requires no additional allocation.
    ///
    /// ```
    /// use dary_heap::BinaryHeap;
    ///
    /// let mut heap: BinaryHeap<u32> = (0..128).collect();
    /// let mut peek = heap.peek_mut().unwrap();
    ///
    /// loop {
    ///     *peek = 99;
    ///
    ///     if !peek.refresh() {
    ///         break;
    ///     }
    /// }
    ///
    /// // Post condition, this is now an upper bound.
    /// assert!(*peek < 100);
    /// ```
    ///
    /// When the element remains the maximum after modification, the peek remains unchanged:
    ///
    /// ```
    /// use dary_heap::BinaryHeap;
    ///
    /// let mut heap: BinaryHeap<u32> = [1, 2, 3].into();
    /// let mut peek = heap.peek_mut().unwrap();
    ///
    /// assert_eq!(*peek, 3);
    /// *peek = 42;
    ///
    /// // When we refresh, the peek is updated to the new maximum.
    /// assert!(!peek.refresh(), "42 is even larger than 3");
    /// assert_eq!(*peek, 42);
    /// ```
    #[cfg(feature = "unstable")]
    #[cfg_attr(docsrs, doc(cfg(feature = "unstable")))]
    #[must_use = "is equivalent to dropping and getting a new PeekMut except for return information"]
    pub fn refresh(&mut self) -> bool {
        // The length of the underlying heap is unchanged by sifting down. The value stored for leak
        // amplification thus remains accurate. We erase the leak amplification firstly because the
        // operation is then equivalent to constructing a new PeekMut and secondly this avoids any
        // future complication where original_len being non-empty would be interpreted as the heap
        // having been leak amplified instead of checking the heap itself.
        if let Some(original_len) = self.original_len.take() {
            // SAFETY: This is how many elements were in the Vec at the time of
            // the DaryHeap::peek_mut call.
            unsafe { self.heap.data.set_len(original_len.get()) };

            // The length of the heap did not change by sifting, upholding our own invariants.

            // SAFETY: PeekMut is only instantiated for non-empty heaps.
            (unsafe { self.heap.sift_down(0) }) != 0
        } else {
            // The element was not modified.
            false
        }
    }

    /// Removes the peeked value from the heap and returns it.
    pub fn pop(mut this: PeekMut<'a, T, D>) -> T {
        if let Some(original_len) = this.original_len.take() {
            // SAFETY: This is how many elements were in the Vec at the time of
            // the BinaryHeap::peek_mut call.
            unsafe { this.heap.data.set_len(original_len.get()) };

            // Unlike in Drop, here we don't also need to do a sift_down even if
            // the caller could've mutated the element. It is removed from the
            // heap on the next line and pop() is not sensitive to its value.
        }

        // SAFETY: Have a `PeekMut` element proves that the associated binary heap being non-empty,
        // so the `pop` operation will not fail.
        #[cfg(feature = "extra")]
        unsafe {
            this.heap.pop().unwrap_unchecked()
        }
        // Option::unwrap_unchecked() requires Rust 1.58.0, but the MSRV is
        // currently 1.51.0.
        #[cfg(not(feature = "extra"))]
        this.heap.pop().unwrap()
    }
}

impl<T: Clone, const D: usize> Clone for DaryHeap<T, D> {
    fn clone(&self) -> Self {
        DaryHeap {
            data: self.data.clone(),
        }
    }

    /// Overwrites the contents of `self` with a clone of the contents of `source`.
    ///
    /// This method is preferred over simply assigning `source.clone()` to `self`,
    /// as it avoids reallocation if possible.
    ///
    /// See [`Vec::clone_from()`] for more details.
    fn clone_from(&mut self, source: &Self) {
        self.data.clone_from(&source.data);
    }
}

impl<T: Ord, const D: usize> Default for DaryHeap<T, D> {
    /// Creates an empty `DaryHeap<T, D>`.
    #[inline]
    fn default() -> DaryHeap<T, D> {
        DaryHeap::new()
    }
}

impl<T: fmt::Debug, const D: usize> fmt::Debug for DaryHeap<T, D> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_list().entries(self.iter()).finish()
    }
}

struct RebuildOnDrop<'a, T: Ord, const D: usize> {
    heap: &'a mut DaryHeap<T, D>,
    rebuild_from: usize,
}

impl<'a, T: Ord, const D: usize> Drop for RebuildOnDrop<'a, T, D> {
    fn drop(&mut self) {
        self.heap.rebuild_tail(self.rebuild_from);
    }
}

impl<T: Ord, const D: usize> DaryHeap<T, D> {
    /// Creates an empty `DaryHeap` as a max-heap.
    ///
    /// # Notes
    ///
    /// This function is `const` on crate feature `extra` only.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::QuaternaryHeap;
    /// let mut heap = QuaternaryHeap::new();
    /// heap.push(4);
    /// ```
    #[must_use]
    #[cfg(not(feature = "extra"))]
    pub fn new() -> DaryHeap<T, D> {
        DaryHeap { data: vec![] }
    }

    /// Creates an empty `DaryHeap` as a max-heap.
    ///
    /// # Notes
    ///
    /// This function is `const` on crate feature `extra` only.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::QuaternaryHeap;
    /// let mut heap = QuaternaryHeap::new();
    /// heap.push(4);
    /// ```
    #[must_use]
    #[cfg(feature = "extra")]
    pub const fn new() -> DaryHeap<T, D> {
        DaryHeap { data: vec![] }
    }

    /// Creates an empty `DaryHeap` with at least the specific capacity.
    ///
    /// The *d*-ary heap will be able to hold at least `capacity` elements without
    /// reallocating. This method is allowed to allocate for more elements than
    /// `capacity`. If `capacity` is zero, the *d*-ary heap will not allocate.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::QuaternaryHeap;
    /// let mut heap = QuaternaryHeap::with_capacity(10);
    /// heap.push(4);
    /// ```
    #[must_use]
    pub fn with_capacity(capacity: usize) -> DaryHeap<T, D> {
        DaryHeap {
            data: Vec::with_capacity(capacity),
        }
    }

    /// Returns a mutable reference to the greatest item in the *d*-ary heap, or
    /// `None` if it is empty.
    ///
    /// Note: If the `PeekMut` value is leaked, some heap elements might get
    /// leaked along with it, but the remaining elements will remain a valid
    /// heap.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::TernaryHeap;
    /// let mut heap = TernaryHeap::new();
    /// assert!(heap.peek_mut().is_none());
    ///
    /// heap.push(1);
    /// heap.push(5);
    /// heap.push(2);
    /// {
    ///     let mut val = heap.peek_mut().unwrap();
    ///     *val = 0;
    /// }
    /// assert_eq!(heap.peek(), Some(&2));
    /// ```
    ///
    /// # Time complexity
    ///
    /// If the item is modified then the worst case time complexity is *O*(log(*n*)),
    /// otherwise it's *O*(1).
    pub fn peek_mut(&mut self) -> Option<PeekMut<'_, T, D>> {
        if self.is_empty() {
            None
        } else {
            Some(PeekMut {
                heap: self,
                original_len: None,
            })
        }
    }

    /// Removes the greatest item from the *d*-ary heap and returns it, or `None` if it
    /// is empty.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::BinaryHeap;
    /// let mut heap = BinaryHeap::from([1, 3]);
    ///
    /// assert_eq!(heap.pop(), Some(3));
    /// assert_eq!(heap.pop(), Some(1));
    /// assert_eq!(heap.pop(), None);
    /// ```
    ///
    /// # Time complexity
    ///
    /// The worst case cost of `pop` on a heap containing *n* elements is *O*(log(*n*)).
    pub fn pop(&mut self) -> Option<T> {
        self.data.pop().map(|mut item| {
            if !self.is_empty() {
                swap(&mut item, &mut self.data[0]);
                // SAFETY: !self.is_empty() means that self.len() > 0
                unsafe { self.sift_down_to_bottom(0) };
            }
            item
        })
    }

    /// Pushes an item onto the *d*-ary heap.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::QuaternaryHeap;
    /// let mut heap = QuaternaryHeap::new();
    /// heap.push(3);
    /// heap.push(5);
    /// heap.push(1);
    ///
    /// assert_eq!(heap.len(), 3);
    /// assert_eq!(heap.peek(), Some(&5));
    /// ```
    ///
    /// # Time complexity
    ///
    /// The expected cost of `push`, averaged over every possible ordering of
    /// the elements being pushed, and over a sufficiently large number of
    /// pushes, is *O*(1). This is the most meaningful cost metric when pushing
    /// elements that are *not* already in any sorted pattern.
    ///
    /// The time complexity degrades if elements are pushed in predominantly
    /// ascending order. In the worst case, elements are pushed in ascending
    /// sorted order and the amortized cost per push is *O*(log(*n*)) against a heap
    /// containing *n* elements.
    ///
    /// The worst case cost of a *single* call to `push` is *O*(*n*). The worst case
    /// occurs when capacity is exhausted and needs a resize. The resize cost
    /// has been amortized in the previous figures.
    pub fn push(&mut self, item: T) {
        let old_len = self.len();
        self.data.push(item);
        // SAFETY: Since we pushed a new item it means that
        //  old_len = self.len() - 1 < self.len()
        unsafe { self.sift_up(0, old_len) };
    }

    /// Consumes the `DaryHeap` and returns a vector in sorted
    /// (ascending) order.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::OctonaryHeap;
    ///
    /// let mut heap = OctonaryHeap::from([1, 2, 4, 5, 7]);
    /// heap.push(6);
    /// heap.push(3);
    ///
    /// let vec = heap.into_sorted_vec();
    /// assert_eq!(vec, [1, 2, 3, 4, 5, 6, 7]);
    /// ```
    #[must_use = "`self` will be dropped if the result is not used"]
    pub fn into_sorted_vec(mut self) -> Vec<T> {
        let mut end = self.len();
        while end > 1 {
            end -= 1;
            // SAFETY: `end` goes from `self.len() - 1` to 1 (both included),
            //  so it's always a valid index to access.
            //  It is safe to access index 0 (i.e. `ptr`), because
            //  1 <= end < self.len(), which means self.len() >= 2.
            unsafe {
                let ptr = self.data.as_mut_ptr();
                ptr::swap(ptr, ptr.add(end));
            }
            // SAFETY: `end` goes from `self.len() - 1` to 1 (both included) so:
            //  0 < 1 <= end <= self.len() - 1 < self.len()
            //  Which means 0 < end and end < self.len().
            unsafe { self.sift_down_range(0, end) };
        }
        self.into_vec()
    }

    // The implementations of sift_up and sift_down use unsafe blocks in
    // order to move an element out of the vector (leaving behind a
    // hole), shift along the others and move the removed element back into the
    // vector at the final location of the hole.
    // The `Hole` type is used to represent this, and make sure
    // the hole is filled back at the end of its scope, even on panic.
    // Using a hole reduces the constant factor compared to using swaps,
    // which involves twice as many moves.

    /// # Safety
    ///
    /// The caller must guarantee that `pos < self.len()`.
    ///
    /// Returns the new position of the element.
    unsafe fn sift_up(&mut self, start: usize, pos: usize) -> usize {
        assert_ne!(D, 0, "Arity should be greater than zero");
        // Take out the value at `pos` and create a hole.
        // SAFETY: The caller guarantees that pos < self.len()
        let mut hole = Hole::new(&mut self.data, pos);

        while hole.pos() > start {
            let parent = (hole.pos() - 1) / D;

            // SAFETY: hole.pos() > start >= 0, which means hole.pos() > 0
            //  and so hole.pos() - 1 can't underflow.
            //  This guarantees that parent < hole.pos() so
            //  it's a valid index and also != hole.pos().
            if hole.element() <= hole.get(parent) {
                break;
            }

            // SAFETY: Same as above
            hole.move_to(parent);
        }

        hole.pos()
    }

    /// Take an element at `pos` and move it down the heap,
    /// while its children are larger.
    ///
    /// Returns the new position of the element.
    ///
    /// # Safety
    ///
    /// The caller must guarantee that `pos < end <= self.len()`.
    unsafe fn sift_down_range(&mut self, pos: usize, end: usize) -> usize {
        assert_ne!(D, 0, "Arity should be greater than zero");
        // SAFETY: The caller guarantees that pos < end <= self.len().
        let mut hole = Hole::new(&mut self.data, pos);
        let mut child = D * hole.pos() + 1;

        // Loop invariant: child == d * hole.pos() + 1.
        while child <= end.saturating_sub(D) {
            // compare with the greatest of the d children
            // SAFETY: child < end - d + 1 < self.len() and
            //  child + d - 1 < end <= self.len(), so they're valid indexes.
            //  child + i == d * hole.pos() + 1 + i != hole.pos() for i >= 0
            child = hole.max_sibling::<D>(child);

            // if we are already in order, stop.
            // SAFETY: child is now either the old child or valid sibling
            //  We already proven that all are < self.len() and != hole.pos()
            if hole.element() >= hole.get(child) {
                return hole.pos();
            }

            // SAFETY: same as above.
            hole.move_to(child);
            child = D * hole.pos() + 1;
        }

        child = hole.max_sibling_to::<D>(child, end);
        // SAFETY: && short circuit, which means that in the
        //  second condition it's already true that child < end <= self.len().
        if child < end && hole.element() < hole.get(child) {
            // SAFETY: child is already proven to be a valid index and
            //  child == d * hole.pos() + 1 != hole.pos().
            hole.move_to(child);
        }

        hole.pos()
    }

    /// # Safety
    ///
    /// The caller must guarantee that `pos < self.len()`.
    unsafe fn sift_down(&mut self, pos: usize) -> usize {
        let len = self.len();
        // SAFETY: pos < len is guaranteed by the caller and
        //  obviously len = self.len() <= self.len().
        self.sift_down_range(pos, len)
    }

    /// Take an element at `pos` and move it all the way down the heap,
    /// then sift it up to its position.
    ///
    /// Note: This is faster when the element is known to be large / should
    /// be closer to the bottom.
    ///
    /// # Safety
    ///
    /// The caller must guarantee that `pos < self.len()`.
    unsafe fn sift_down_to_bottom(&mut self, mut pos: usize) {
        assert_ne!(D, 0, "Arity should be greater than zero");
        let end = self.len();
        let start = pos;

        // SAFETY: The caller guarantees that pos < self.len().
        let mut hole = Hole::new(&mut self.data, pos);
        let mut child = D * hole.pos() + 1;

        // Loop invariant: child == d * hole.pos() + 1.
        while child <= end.saturating_sub(D) {
            // SAFETY: child < end - d + 1 < self.len() and
            //  child + d - 1 < end <= self.len(), so they're valid indexes.
            //  child + i == d * hole.pos() + 1 + i != hole.pos() for i >= 0
            child = hole.max_sibling::<D>(child);

            // SAFETY: Same as above
            hole.move_to(child);
            child = D * hole.pos() + 1;
        }

        child = hole.max_sibling_to::<D>(child, end);
        if child < end {
            // SAFETY: child < end <= self.len(), so it's a valid index
            //  and child == d * hole.pos() + i != hole.pos() for i >= 1
            hole.move_to(child);
        }
        pos = hole.pos();
        drop(hole);

        // SAFETY: pos is the position in the hole and was already proven
        //  to be a valid index.
        self.sift_up(start, pos);
    }

    /// Rebuild assuming data[0..start] is still a proper heap.
    fn rebuild_tail(&mut self, start: usize) {
        assert_ne!(D, 0, "Arity should be greater than zero");

        if start == self.len() {
            return;
        }

        let tail_len = self.len() - start;

        // The fix for this lint (usize::BITS) requires Rust 1.53.0, but the
        // MSRV is currently 1.51.0.
        #[allow(clippy::manual_bits)]
        #[inline(always)]
        fn log2_fast(x: usize) -> usize {
            8 * size_of::<usize>() - (x.leading_zeros() as usize) - 1
        }

        // `rebuild` takes O(self.len()) operations
        // and about n * self.len() comparisons in the worst case
        // with n = d / (d - 1)
        // while repeating `sift_up` takes O(tail_len * log(start)) operations
        // and about 1 * tail_len * log(start) comparisons in the worst case,
        // assuming start >= tail_len. For larger heaps, the crossover point
        // no longer follows this reasoning and was determined empirically.
        let better_to_rebuild = if start < tail_len {
            true
        } else if self.len() <= 4096 / D {
            D * self.len() < (D - 1) * tail_len * log2_fast(start)
        } else {
            D * self.len() < (D - 1) * tail_len * 13usize.saturating_sub(D)
        };

        if better_to_rebuild {
            self.rebuild();
        } else {
            for i in start..self.len() {
                // SAFETY: The index `i` is always less than self.len().
                unsafe { self.sift_up(0, i) };
            }
        }
    }

    fn rebuild(&mut self) {
        assert_ne!(D, 0, "Arity should be greater than zero");
        if self.len() < 2 {
            return;
        }
        let mut n = (self.len() - 1) / D + 1;
        while n > 0 {
            n -= 1;
            // SAFETY: n starts from (self.len() - 1) / d + 1 and goes down to 0.
            //  The only case when !(n < self.len()) is if
            //  self.len() == 0, but it's ruled out by the loop condition.
            unsafe { self.sift_down(n) };
        }
    }

    /// Moves all the elements of `other` into `self`, leaving `other` empty.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::OctonaryHeap;
    ///
    /// let mut a = OctonaryHeap::from([-10, 1, 2, 3, 3]);
    /// let mut b = OctonaryHeap::from([-20, 5, 43]);
    ///
    /// a.append(&mut b);
    ///
    /// assert_eq!(a.into_sorted_vec(), [-20, -10, 1, 2, 3, 3, 5, 43]);
    /// assert!(b.is_empty());
    /// ```
    pub fn append(&mut self, other: &mut Self) {
        if self.len() < other.len() {
            swap(self, other);
        }

        let start = self.data.len();

        self.data.append(&mut other.data);

        self.rebuild_tail(start);
    }

    /// Clears the *d*-ary heap, returning an iterator over the removed elements
    /// in heap order. If the iterator is dropped before being fully consumed,
    /// it drops the remaining elements in heap order.
    ///
    /// The returned iterator keeps a mutable borrow on the heap to optimize
    /// its implementation.
    ///
    /// Note:
    /// * `.drain_sorted()` is *O*(*n* \* log(*n*)); much slower than `.drain()`.
    ///   You should use the latter for most cases.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::TernaryHeap;
    ///
    /// let mut heap = TernaryHeap::from([1, 2, 3, 4, 5]);
    /// assert_eq!(heap.len(), 5);
    ///
    /// drop(heap.drain_sorted()); // removes all elements in heap order
    /// assert_eq!(heap.len(), 0);
    /// ```
    #[inline]
    #[cfg(feature = "unstable")]
    #[cfg_attr(docsrs, doc(cfg(feature = "unstable")))]
    pub fn drain_sorted(&mut self) -> DrainSorted<'_, T, D> {
        DrainSorted { inner: self }
    }

    /// Retains only the elements specified by the predicate.
    ///
    /// In other words, remove all elements `e` for which `f(&e)` returns
    /// `false`. The elements are visited in unsorted (and unspecified) order.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::OctonaryHeap;
    ///
    /// let mut heap = OctonaryHeap::from([-10, -5, 1, 2, 4, 13]);
    ///
    /// heap.retain(|x| x % 2 == 0); // only keep even numbers
    ///
    /// assert_eq!(heap.into_sorted_vec(), [-10, 2, 4])
    /// ```
    pub fn retain<F>(&mut self, mut f: F)
    where
        F: FnMut(&T) -> bool,
    {
        // rebuild_start will be updated to the first touched element below, and the rebuild will
        // only be done for the tail.
        let mut guard = RebuildOnDrop {
            rebuild_from: self.len(),
            heap: self,
        };
        // Split the borrow outside of the closure to appease the borrow checker
        let rebuild_from = &mut guard.rebuild_from;
        let mut i = 0;

        guard.heap.data.retain(|e| {
            let keep = f(e);
            if !keep && i < *rebuild_from {
                *rebuild_from = i;
            }
            i += 1;
            keep
        });
    }
}

impl<T, const D: usize> DaryHeap<T, D> {
    /// Returns an iterator visiting all values in the underlying vector, in
    /// arbitrary order.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::TernaryHeap;
    /// let heap = TernaryHeap::from([1, 2, 3, 4]);
    ///
    /// // Print 1, 2, 3, 4 in arbitrary order
    /// for x in heap.iter() {
    ///     println!("{x}");
    /// }
    /// ```
    pub fn iter(&self) -> Iter<'_, T> {
        Iter {
            iter: self.data.iter(),
        }
    }

    /// Returns an iterator which retrieves elements in heap order.
    ///
    /// This method consumes the original heap.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::QuaternaryHeap;
    /// let heap = QuaternaryHeap::from([1, 2, 3, 4, 5]);
    ///
    /// assert_eq!(heap.into_iter_sorted().take(2).collect::<Vec<_>>(), [5, 4]);
    /// ```
    #[cfg(feature = "unstable")]
    #[cfg_attr(docsrs, doc(cfg(feature = "unstable")))]
    pub fn into_iter_sorted(self) -> IntoIterSorted<T, D> {
        IntoIterSorted { inner: self }
    }

    /// Returns the greatest item in the *d*-ary heap, or `None` if it is empty.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::BinaryHeap;
    /// let mut heap = BinaryHeap::new();
    /// assert_eq!(heap.peek(), None);
    ///
    /// heap.push(1);
    /// heap.push(5);
    /// heap.push(2);
    /// assert_eq!(heap.peek(), Some(&5));
    ///
    /// ```
    ///
    /// # Time complexity
    ///
    /// Cost is *O*(1) in the worst case.
    #[must_use]
    pub fn peek(&self) -> Option<&T> {
        // Ignore this lint to keep it identical with upstream
        #[allow(clippy::get_first)]
        self.data.get(0)
    }

    /// Returns the number of elements the *d*-ary heap can hold without reallocating.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::OctonaryHeap;
    /// let mut heap = OctonaryHeap::with_capacity(100);
    /// assert!(heap.capacity() >= 100);
    /// heap.push(4);
    /// ```
    #[must_use]
    pub fn capacity(&self) -> usize {
        self.data.capacity()
    }

    /// Reserves the minimum capacity for at least `additional` elements more than
    /// the current length. Unlike [`reserve`], this will not
    /// deliberately over-allocate to speculatively avoid frequent allocations.
    /// After calling `reserve_exact`, capacity will be greater than or equal to
    /// `self.len() + additional`. Does nothing if the capacity is already
    /// sufficient.
    ///
    /// [`reserve`]: DaryHeap::reserve
    ///
    /// # Panics
    ///
    /// Panics if the new capacity overflows [`usize`].
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::OctonaryHeap;
    /// let mut heap = OctonaryHeap::new();
    /// heap.reserve_exact(100);
    /// assert!(heap.capacity() >= 100);
    /// heap.push(4);
    /// ```
    ///
    /// [`reserve`]: DaryHeap::reserve
    pub fn reserve_exact(&mut self, additional: usize) {
        self.data.reserve_exact(additional);
    }

    /// Reserves capacity for at least `additional` elements more than the
    /// current length. The allocator may reserve more space to speculatively
    /// avoid frequent allocations. After calling `reserve`,
    /// capacity will be greater than or equal to `self.len() + additional`.
    /// Does nothing if capacity is already sufficient.
    ///
    /// # Panics
    ///
    /// Panics if the new capacity overflows [`usize`].
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::BinaryHeap;
    /// let mut heap = BinaryHeap::new();
    /// heap.reserve(100);
    /// assert!(heap.capacity() >= 100);
    /// heap.push(4);
    /// ```
    pub fn reserve(&mut self, additional: usize) {
        self.data.reserve(additional);
    }

    /// Tries to reserve the minimum capacity for at least `additional` elements
    /// more than the current length. Unlike [`try_reserve`], this will not
    /// deliberately over-allocate to speculatively avoid frequent allocations.
    /// After calling `try_reserve_exact`, capacity will be greater than or
    /// equal to `self.len() + additional` if it returns `Ok(())`.
    /// Does nothing if the capacity is already sufficient.
    ///
    /// Note that the allocator may give the collection more space than it
    /// requests. Therefore, capacity can not be relied upon to be precisely
    /// minimal. Prefer [`try_reserve`] if future insertions are expected.
    ///
    /// [`try_reserve`]: DaryHeap::try_reserve
    ///
    /// # Errors
    ///
    /// If the capacity overflows, or the allocator reports a failure, then an error
    /// is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use dary_heap::BinaryHeap;
    /// use std::collections::TryReserveError;
    ///
    /// fn find_max_slow(data: &[u32]) -> Result<Option<u32>, TryReserveError> {
    ///     let mut heap = BinaryHeap::new();
    ///
    ///     // Pre-reserve the memory, exiting if we can't
    ///     heap.try_reserve_exact(data.len())?;
    ///
    ///     // Now we know this can't OOM in the middle of our complex work
    ///     heap.extend(data.iter());
    ///
    ///     Ok(heap.pop())
    /// }
    /// # find_max_slow(&[1, 2, 3]).expect("why is the test harness OOMing on 12 bytes?");
    /// ```
    #[cfg(feature = "extra")]
    #[cfg_attr(docsrs, doc(cfg(feature = "extra")))]
    pub fn try_reserve_exact(&mut self, additional: usize) -> Result<(), TryReserveError> {
        self.data.try_reserve_exact(additional)
    }

    /// Tries to reserve capacity for at least `additional` elements more than the
    /// current length. The allocator may reserve more space to speculatively
    /// avoid frequent allocations. After calling `try_reserve`, capacity will be
    /// greater than or equal to `self.len() + additional` if it returns
    /// `Ok(())`. Does nothing if capacity is already sufficient. This method
    /// preserves the contents even if an error occurs.
    ///
    /// # Errors
    ///
    /// If the capacity overflows, or the allocator reports a failure, then an error
    /// is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use dary_heap::QuaternaryHeap;
    /// use std::collections::TryReserveError;
    ///
    /// fn find_max_slow(data: &[u32]) -> Result<Option<u32>, TryReserveError> {
    ///     let mut heap = QuaternaryHeap::new();
    ///
    ///     // Pre-reserve the memory, exiting if we can't
    ///     heap.try_reserve(data.len())?;
    ///
    ///     // Now we know this can't OOM in the middle of our complex work
    ///     heap.extend(data.iter());
    ///
    ///     Ok(heap.pop())
    /// }
    /// # find_max_slow(&[1, 2, 3]).expect("why is the test harness OOMing on 12 bytes?");
    /// ```
    #[cfg(feature = "extra")]
    #[cfg_attr(docsrs, doc(cfg(feature = "extra")))]
    pub fn try_reserve(&mut self, additional: usize) -> Result<(), TryReserveError> {
        self.data.try_reserve(additional)
    }

    /// Discards as much additional capacity as possible.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::TernaryHeap;
    /// let mut heap: TernaryHeap<i32> = TernaryHeap::with_capacity(100);
    ///
    /// assert!(heap.capacity() >= 100);
    /// heap.shrink_to_fit();
    /// assert!(heap.capacity() == 0);
    /// ```
    pub fn shrink_to_fit(&mut self) {
        self.data.shrink_to_fit();
    }

    /// Discards capacity with a lower bound.
    ///
    /// The capacity will remain at least as large as both the length
    /// and the supplied value.
    ///
    /// If the current capacity is less than the lower limit, this is a no-op.
    ///
    /// # Examples
    ///
    /// ```
    /// use dary_heap::TernaryHeap;
    /// let mut heap: TernaryHeap<i32> = TernaryHeap::with_capacity(100);
    ///
    /// assert!(heap.capacity() >= 100);
    /// heap.shrink_to(10);
    /// assert!(heap.capacity() >= 10);
    /// ```
    #[inline]
    #[cfg(feature = "extra")]
    #[cfg_attr(docsrs, doc(cfg(feature = "extra")))]
    pub fn shrink_to(&mut self, min_capacity: usize) {
        self.data.shrink_to(min_capacity)
    }

    /// Returns a slice of all values in the underlying vector, in arbitrary
    /// order.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::OctonaryHeap;
    /// use std::io::{self, Write};
    ///
    /// let heap = OctonaryHeap::from([1, 2, 3, 4, 5, 6, 7]);
    ///
    /// io::sink().write(heap.as_slice()).unwrap();
    /// ```
    #[must_use]
    pub fn as_slice(&self) -> &[T] {
        self.data.as_slice()
    }

    /// Consumes the `DaryHeap` and returns the underlying vector
    /// in arbitrary order.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::QuaternaryHeap;
    /// let heap = QuaternaryHeap::from([1, 2, 3, 4, 5, 6, 7]);
    /// let vec = heap.into_vec();
    ///
    /// // Will print in some order
    /// for x in vec {
    ///     println!("{x}");
    /// }
    /// ```
    #[must_use = "`self` will be dropped if the result is not used"]
    pub fn into_vec(self) -> Vec<T> {
        self.into()
    }

    /// Returns the length of the *d*-ary heap.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::BinaryHeap;
    /// let heap = BinaryHeap::from([1, 3]);
    ///
    /// assert_eq!(heap.len(), 2);
    /// ```
    #[must_use]
    pub fn len(&self) -> usize {
        self.data.len()
    }

    /// Checks if the *d*-ary heap is empty.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::BinaryHeap;
    /// let mut heap = BinaryHeap::new();
    ///
    /// assert!(heap.is_empty());
    ///
    /// heap.push(3);
    /// heap.push(5);
    /// heap.push(1);
    ///
    /// assert!(!heap.is_empty());
    /// ```
    #[must_use]
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// Clears the *d*-ary heap, returning an iterator over the removed elements
    /// in arbitrary order. If the iterator is dropped before being fully
    /// consumed, it drops the remaining elements in arbitrary order.
    ///
    /// The returned iterator keeps a mutable borrow on the heap to optimize
    /// its implementation.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::QuaternaryHeap;
    /// let mut heap = QuaternaryHeap::from([1, 3]);
    ///
    /// assert!(!heap.is_empty());
    ///
    /// for x in heap.drain() {
    ///     println!("{x}");
    /// }
    ///
    /// assert!(heap.is_empty());
    /// ```
    #[inline]
    pub fn drain(&mut self) -> Drain<'_, T> {
        Drain {
            iter: self.data.drain(..),
        }
    }

    /// Drops all items from the *d*-ary heap.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::TernaryHeap;
    /// let mut heap = TernaryHeap::from([1, 3]);
    ///
    /// assert!(!heap.is_empty());
    ///
    /// heap.clear();
    ///
    /// assert!(heap.is_empty());
    /// ```
    pub fn clear(&mut self) {
        self.drain();
    }
}

/// Hole represents a hole in a slice i.e., an index without valid value
/// (because it was moved from or duplicated).
/// In drop, `Hole` will restore the slice by filling the hole
/// position with the value that was originally removed.
struct Hole<'a, T: 'a> {
    data: &'a mut [T],
    elt: ManuallyDrop<T>,
    pos: usize,
}

impl<'a, T> Hole<'a, T> {
    /// Creates a new `Hole` at index `pos`.
    ///
    /// Unsafe because pos must be within the data slice.
    #[inline]
    unsafe fn new(data: &'a mut [T], pos: usize) -> Self {
        debug_assert!(pos < data.len());
        // SAFE: pos should be inside the slice
        let elt = ptr::read(data.get_unchecked(pos));
        Hole {
            data,
            elt: ManuallyDrop::new(elt),
            pos,
        }
    }

    #[inline]
    fn pos(&self) -> usize {
        self.pos
    }

    /// Returns a reference to the element removed.
    #[inline]
    fn element(&self) -> &T {
        &self.elt
    }

    /// Returns a reference to the element at `index`.
    ///
    /// Unsafe because index must be within the data slice and not equal to pos.
    #[inline]
    unsafe fn get(&self, index: usize) -> &T {
        debug_assert!(index != self.pos);
        debug_assert!(index < self.data.len());
        self.data.get_unchecked(index)
    }

    /// Move hole to new location
    ///
    /// Unsafe because index must be within the data slice and not equal to pos.
    #[inline]
    unsafe fn move_to(&mut self, index: usize) {
        debug_assert!(index != self.pos);
        debug_assert!(index < self.data.len());
        let ptr = self.data.as_mut_ptr();
        let index_ptr: *const _ = ptr.add(index);
        let hole_ptr = ptr.add(self.pos);
        ptr::copy_nonoverlapping(index_ptr, hole_ptr, 1);
        self.pos = index;
    }
}

impl<'a, T: Ord> Hole<'a, T> {
    /// Get largest element
    ///
    /// Unsafe because both elements must be within the data slice and not equal
    /// to pos.
    #[inline]
    unsafe fn max(&self, elem1: usize, elem2: usize) -> usize {
        if self.get(elem1) <= self.get(elem2) {
            elem2
        } else {
            elem1
        }
    }

    /// Get index of greatest sibling
    ///
    /// Unsafe because all siblings must be within the data slice and not equal
    /// to pos.
    #[inline]
    unsafe fn max_sibling<const D: usize>(&self, first_sibling: usize) -> usize {
        let mut sibling = first_sibling;
        match D {
            2 => {
                sibling += (self.get(sibling) <= self.get(sibling + 1)) as usize;
            }
            3 => {
                let sibling_a = self.max_sibling::<2>(sibling);
                let sibling_b = sibling + 2;
                sibling = self.max(sibling_a, sibling_b);
            }
            4 => {
                let sibling_a = self.max_sibling::<2>(sibling);
                let sibling_b = self.max_sibling::<2>(sibling + 2);
                sibling = self.max(sibling_a, sibling_b);
            }
            _ => {
                for other_sibling in sibling + 1..sibling + D {
                    if self.get(sibling) <= self.get(other_sibling) {
                        sibling = other_sibling;
                    }
                }
            }
        }
        sibling
    }

    /// Get index of greatest sibling within range
    ///
    /// Unsafe because end must be the length of the data slice, last sibling
    /// must be outside of the data slice and no sibling may be equal to pos.
    /// It is allowed for first_sibling to be outside of the data slice.
    #[inline]
    unsafe fn max_sibling_to<const D: usize>(&self, first_sibling: usize, end: usize) -> usize {
        let mut sibling = first_sibling;
        match D {
            2 => {}
            3 => {
                if sibling + 1 < end {
                    sibling = self.max_sibling::<2>(sibling);
                }
            }
            _ => {
                for other_sibling in sibling + 1..end {
                    if self.get(sibling) <= self.get(other_sibling) {
                        sibling = other_sibling;
                    }
                }
            }
        }
        sibling
    }
}

impl<T> Drop for Hole<'_, T> {
    #[inline]
    fn drop(&mut self) {
        // fill the hole again
        unsafe {
            let pos = self.pos;
            ptr::copy_nonoverlapping(&*self.elt, self.data.get_unchecked_mut(pos), 1);
        }
    }
}

/// An iterator over the elements of a `DaryHeap`.
///
/// This `struct` is created by [`DaryHeap::iter()`]. See its
/// documentation for more.
///
/// [`iter`]: DaryHeap::iter
#[must_use = "iterators are lazy and do nothing unless consumed"]
pub struct Iter<'a, T: 'a> {
    iter: slice::Iter<'a, T>,
}

impl<T> Default for Iter<'_, T> {
    /// Creates an empty `dary_heap::Iter`.
    ///
    /// ```
    /// let iter: dary_heap::Iter<'_, u8> = Default::default();
    /// assert_eq!(iter.len(), 0);
    /// ```
    fn default() -> Self {
        // `Default::default()` requires Rust 1.70.0 or later
        Iter { iter: [].iter() }
    }
}

impl<T: fmt::Debug> fmt::Debug for Iter<'_, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_tuple("Iter").field(&self.iter.as_slice()).finish()
    }
}

// FIXME(#26925) Remove in favor of `#[derive(Clone)]`
impl<T> Clone for Iter<'_, T> {
    fn clone(&self) -> Self {
        Iter {
            iter: self.iter.clone(),
        }
    }
}

impl<'a, T> Iterator for Iter<'a, T> {
    type Item = &'a T;

    #[inline]
    fn next(&mut self) -> Option<&'a T> {
        self.iter.next()
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        self.iter.size_hint()
    }

    #[inline]
    fn last(self) -> Option<&'a T> {
        self.iter.last()
    }
}

impl<'a, T> DoubleEndedIterator for Iter<'a, T> {
    #[inline]
    fn next_back(&mut self) -> Option<&'a T> {
        self.iter.next_back()
    }
}

impl<T> ExactSizeIterator for Iter<'_, T> {
    #[cfg(feature = "unstable_nightly")]
    fn is_empty(&self) -> bool {
        self.iter.is_empty()
    }
}

impl<T> FusedIterator for Iter<'_, T> {}

/// An owning iterator over the elements of a `DaryHeap`.
///
/// This `struct` is created by [`DaryHeap::into_iter()`]
/// (provided by the [`IntoIterator`] trait). See its documentation for more.
///
/// [`into_iter`]: DaryHeap::into_iter
#[derive(Clone)]
pub struct IntoIter<T> {
    iter: vec::IntoIter<T>,
}

impl<T: fmt::Debug> fmt::Debug for IntoIter<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_tuple("IntoIter")
            .field(&self.iter.as_slice())
            .finish()
    }
}

impl<T> Iterator for IntoIter<T> {
    type Item = T;

    #[inline]
    fn next(&mut self) -> Option<T> {
        self.iter.next()
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        self.iter.size_hint()
    }
}

impl<T> DoubleEndedIterator for IntoIter<T> {
    #[inline]
    fn next_back(&mut self) -> Option<T> {
        self.iter.next_back()
    }
}

impl<T> ExactSizeIterator for IntoIter<T> {
    #[cfg(feature = "unstable_nightly")]
    fn is_empty(&self) -> bool {
        self.iter.is_empty()
    }
}

impl<T> FusedIterator for IntoIter<T> {}

#[cfg(feature = "unstable_nightly")]
#[doc(hidden)]
unsafe impl<T> core::iter::TrustedFused for IntoIter<T> {}

impl<T> Default for IntoIter<T> {
    /// Creates an empty `dary_heap::IntoIter`.
    ///
    /// ```
    /// let iter: dary_heap::IntoIter<u8> = Default::default();
    /// assert_eq!(iter.len(), 0);
    /// ```
    fn default() -> Self {
        IntoIter {
            iter: Vec::new().into_iter(),
        }
    }
}

// In addition to the SAFETY invariants of the following two unsafe traits
// also refer to the vec::in_place_collect module documentation to get an overview
#[cfg(feature = "unstable_nightly")]
#[doc(hidden)]
unsafe impl<T> core::iter::SourceIter for IntoIter<T> {
    type Source = IntoIter<T>;

    #[inline]
    unsafe fn as_inner(&mut self) -> &mut Self::Source {
        self
    }
}

#[cfg(feature = "unstable_nightly")]
#[doc(hidden)]
unsafe impl<I> core::iter::InPlaceIterable for IntoIter<I> {
    const EXPAND_BY: Option<NonZeroUsize> = NonZeroUsize::new(1);
    const MERGE_BY: Option<NonZeroUsize> = NonZeroUsize::new(1);
}

#[must_use = "iterators are lazy and do nothing unless consumed"]
#[cfg(feature = "unstable")]
#[derive(Clone, Debug)]
pub struct IntoIterSorted<T, const D: usize> {
    inner: DaryHeap<T, D>,
}

#[cfg(feature = "unstable")]
impl<T: Ord, const D: usize> Iterator for IntoIterSorted<T, D> {
    type Item = T;

    #[inline]
    fn next(&mut self) -> Option<T> {
        self.inner.pop()
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let exact = self.inner.len();
        (exact, Some(exact))
    }
}

#[cfg(feature = "unstable")]
impl<T: Ord, const D: usize> ExactSizeIterator for IntoIterSorted<T, D> {}

#[cfg(feature = "unstable")]
impl<T: Ord, const D: usize> FusedIterator for IntoIterSorted<T, D> {}

#[cfg(all(feature = "unstable", feature = "unstable_nightly"))]
unsafe impl<T: Ord, const D: usize> core::iter::TrustedLen for IntoIterSorted<T, D> {}

/// A draining iterator over the elements of a `DaryHeap`.
///
/// This `struct` is created by [`DaryHeap::drain()`]. See its
/// documentation for more.
///
/// [`drain`]: DaryHeap::drain
#[derive(Debug)]
pub struct Drain<'a, T: 'a> {
    iter: vec::Drain<'a, T>,
}

impl<T> Iterator for Drain<'_, T> {
    type Item = T;

    #[inline]
    fn next(&mut self) -> Option<T> {
        self.iter.next()
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        self.iter.size_hint()
    }
}

impl<T> DoubleEndedIterator for Drain<'_, T> {
    #[inline]
    fn next_back(&mut self) -> Option<T> {
        self.iter.next_back()
    }
}

impl<T> ExactSizeIterator for Drain<'_, T> {
    #[cfg(feature = "unstable_nightly")]
    fn is_empty(&self) -> bool {
        self.iter.is_empty()
    }
}

impl<T> FusedIterator for Drain<'_, T> {}

/// A draining iterator over the elements of a `DaryHeap`.
///
/// This `struct` is created by [`DaryHeap::drain_sorted()`]. See its
/// documentation for more.
///
/// [`drain_sorted`]: DaryHeap::drain_sorted
#[cfg(feature = "unstable")]
#[derive(Debug)]
pub struct DrainSorted<'a, T: Ord, const D: usize> {
    inner: &'a mut DaryHeap<T, D>,
}

#[cfg(feature = "unstable")]
impl<'a, T: Ord, const D: usize> Drop for DrainSorted<'a, T, D> {
    /// Removes heap elements in heap order.
    fn drop(&mut self) {
        use core::mem::forget;

        struct DropGuard<'r, 'a, T: Ord, const D: usize>(&'r mut DrainSorted<'a, T, D>);

        impl<'r, 'a, T: Ord, const D: usize> Drop for DropGuard<'r, 'a, T, D> {
            fn drop(&mut self) {
                while self.0.inner.pop().is_some() {}
            }
        }

        while let Some(item) = self.inner.pop() {
            let guard = DropGuard(self);
            drop(item);
            forget(guard);
        }
    }
}

#[cfg(feature = "unstable")]
impl<T: Ord, const D: usize> Iterator for DrainSorted<'_, T, D> {
    type Item = T;

    #[inline]
    fn next(&mut self) -> Option<T> {
        self.inner.pop()
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let exact = self.inner.len();
        (exact, Some(exact))
    }
}

#[cfg(feature = "unstable")]
impl<T: Ord, const D: usize> ExactSizeIterator for DrainSorted<'_, T, D> {}

#[cfg(feature = "unstable")]
impl<T: Ord, const D: usize> FusedIterator for DrainSorted<'_, T, D> {}

#[cfg(all(feature = "unstable", feature = "unstable_nightly"))]
unsafe impl<T: Ord, const D: usize> core::iter::TrustedLen for DrainSorted<'_, T, D> {}

impl<T: Ord, const D: usize> From<Vec<T>> for DaryHeap<T, D> {
    /// Converts a `Vec<T>` into a `DaryHeap<T, D>`.
    ///
    /// This conversion happens in-place, and has *O*(*n*) time complexity.
    fn from(vec: Vec<T>) -> DaryHeap<T, D> {
        let mut heap = DaryHeap { data: vec };
        heap.rebuild();
        heap
    }
}

impl<T: Ord, const D: usize, const N: usize> From<[T; N]> for DaryHeap<T, D> {
    /// ```
    /// use dary_heap::TernaryHeap;
    ///
    /// let mut h1 = TernaryHeap::from([1, 4, 2, 3]);
    /// let mut h2: TernaryHeap<_> = [1, 4, 2, 3].into();
    /// while let Some((a, b)) = h1.pop().zip(h2.pop()) {
    ///     assert_eq!(a, b);
    /// }
    /// ```
    fn from(arr: [T; N]) -> Self {
        // With newer Rust versions `Self::from_iter(arr)` should be used, as
        // using `IntoIter::new` is deprecated from 1.59.0. However, this would
        // require a MSRV of 1.53.0, and both are equivalent behind the scenes.
        #[allow(deprecated)]
        core::array::IntoIter::new(arr).collect()
    }
}

impl<T, const D: usize> From<DaryHeap<T, D>> for Vec<T> {
    /// Converts a `DaryHeap<T, D>` into a `Vec<T>`.
    ///
    /// This conversion requires no data movement or allocation, and has
    /// constant time complexity.
    fn from(heap: DaryHeap<T, D>) -> Vec<T> {
        heap.data
    }
}

impl<T: Ord, const D: usize> FromIterator<T> for DaryHeap<T, D> {
    fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> DaryHeap<T, D> {
        DaryHeap::from(iter.into_iter().collect::<Vec<_>>())
    }
}

impl<T, const D: usize> IntoIterator for DaryHeap<T, D> {
    type Item = T;
    type IntoIter = IntoIter<T>;

    /// Creates a consuming iterator, that is, one that moves each value out of
    /// the *d*-ary heap in arbitrary order. The *d*-ary heap cannot be used
    /// after calling this.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use dary_heap::BinaryHeap;
    /// let heap = BinaryHeap::from([1, 2, 3, 4]);
    ///
    /// // Print 1, 2, 3, 4 in arbitrary order
    /// for x in heap.into_iter() {
    ///     // x has type i32, not &i32
    ///     println!("{x}");
    /// }
    /// ```
    fn into_iter(self) -> IntoIter<T> {
        IntoIter {
            iter: self.data.into_iter(),
        }
    }
}

impl<'a, T, const D: usize> IntoIterator for &'a DaryHeap<T, D> {
    type Item = &'a T;
    type IntoIter = Iter<'a, T>;

    fn into_iter(self) -> Iter<'a, T> {
        self.iter()
    }
}

impl<T: Ord, const D: usize> Extend<T> for DaryHeap<T, D> {
    #[inline]
    fn extend<I: IntoIterator<Item = T>>(&mut self, iter: I) {
        let guard = RebuildOnDrop {
            rebuild_from: self.len(),
            heap: self,
        };
        guard.heap.data.extend(iter);
    }

    #[inline]
    #[cfg(feature = "unstable_nightly")]
    fn extend_one(&mut self, item: T) {
        self.push(item);
    }

    #[inline]
    #[cfg(feature = "unstable_nightly")]
    fn extend_reserve(&mut self, additional: usize) {
        self.reserve(additional);
    }
}

impl<'a, T: 'a + Ord + Copy, const D: usize> Extend<&'a T> for DaryHeap<T, D> {
    fn extend<I: IntoIterator<Item = &'a T>>(&mut self, iter: I) {
        self.extend(iter.into_iter().cloned());
    }

    #[inline]
    #[cfg(feature = "unstable_nightly")]
    fn extend_one(&mut self, &item: &'a T) {
        self.push(item);
    }

    #[inline]
    #[cfg(feature = "unstable_nightly")]
    fn extend_reserve(&mut self, additional: usize) {
        self.reserve(additional);
    }
}

#[cfg(any(test, fuzzing))]
impl<T: Ord + fmt::Debug, const D: usize> DaryHeap<T, D> {
    /// Panics if the heap is in an inconsistent state
    #[track_caller]
    pub fn assert_valid_state(&self) {
        assert_ne!(D, 0, "Arity should be greater than zero");
        for (i, v) in self.iter().enumerate() {
            let children = D * i + 1..D * i + D;
            if children.start > self.len() {
                break;
            }
            for j in children {
                if let Some(x) = self.data.get(j) {
                    assert!(v >= x);
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use rand::{seq::SliceRandom, thread_rng};

    fn pop<const D: usize>() {
        let mut rng = thread_rng();
        let ntest = if cfg!(miri) { 1 } else { 10 };
        let nelem = if cfg!(miri) { 100 } else { 1000 };
        for _ in 0..ntest {
            let mut data: Vec<_> = (0..nelem).collect();
            data.shuffle(&mut rng);
            let mut heap = DaryHeap::<_, D>::from(data);
            heap.assert_valid_state();
            for i in (0..nelem).rev() {
                assert_eq!(heap.pop(), Some(i));
                heap.assert_valid_state();
            }
            assert_eq!(heap.pop(), None);
        }
    }

    #[test]
    #[should_panic]
    fn push_d0() {
        let mut heap = DaryHeap::<_, 0>::new();
        heap.push(42);
    }

    #[test]
    #[should_panic]
    fn from_vec_d0() {
        let _heap = DaryHeap::<_, 0>::from(vec![42]);
    }

    #[test]
    fn pop_d1() {
        pop::<1>();
    }

    #[test]
    fn pop_d2() {
        pop::<2>();
    }

    #[test]
    fn pop_d3() {
        pop::<3>();
    }

    #[test]
    fn pop_d4() {
        pop::<4>();
    }

    #[test]
    fn pop_d5() {
        pop::<5>();
    }

    #[test]
    fn pop_d6() {
        pop::<6>();
    }

    #[test]
    fn pop_d7() {
        pop::<7>();
    }

    #[test]
    fn pop_d8() {
        pop::<8>();
    }

    #[test]
    #[cfg(feature = "serde")]
    fn serde() {
        use serde_test::Token::{Seq, SeqEnd, I32};

        impl<T: PartialEq, const D: usize> PartialEq for DaryHeap<T, D> {
            fn eq(&self, other: &Self) -> bool {
                self.iter().zip(other).all(|(a, b)| a == b)
            }
        }

        let empty = [Seq { len: Some(0) }, SeqEnd];
        let part = [Seq { len: Some(3) }, I32(3), I32(1), I32(2), SeqEnd];
        let full = [Seq { len: Some(4) }, I32(4), I32(3), I32(2), I32(1), SeqEnd];

        let mut dary = BinaryHeap::<i32>::new();
        serde_test::assert_tokens(&dary, &empty);
        for i in [1, 2, 3] {
            dary.push(i);
        }
        serde_test::assert_tokens(&dary, &part);
        dary.push(4);
        serde_test::assert_tokens(&dary, &full);

        let mut std = alloc::collections::BinaryHeap::<i32>::new();
        serde_test::assert_ser_tokens(&std, &empty);
        for i in [1, 2, 3] {
            std.push(i);
        }
        serde_test::assert_ser_tokens(&std, &part);
        std.push(4);
        serde_test::assert_ser_tokens(&std, &full);
    }
}
