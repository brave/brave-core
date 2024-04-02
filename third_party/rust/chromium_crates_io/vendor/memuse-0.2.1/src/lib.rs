//! Measure dynamic memory usage of your types!
//!
//! ## About
//!
//! Memory-tracking is a common activity in large applications, particularly ones
//! that receive data from a network and store it in memory. By monitoring how much
//! memory is used by different areas of the application, memory pressure can be
//! alleviated by ignoring new packets, or implementing random drop logic for DoS
//! mitigation.
//!
//! Measuring memory use on the stack is easy, with [`std::mem::size_of`] and
//! friends. Measuring memory allocated on the heap is more tricky. Applications can
//! use a custom global allocator to track the memory usage of different areas. This
//! isn't an option for reusable library code however, and the nearest alternative
//! (using custom allocators for individual types) is currently only an experimental
//! feature in nightly Rust ([`allocator_api`]).
//!
//! [`allocator_api`]: https://github.com/rust-lang/rust/issues/32838
//!
//! This crate takes a different approach: it provides traits that library authors
//! can use to expose dynamic memory usage information on their types. By composing
//! these implementations, we gain the ability to query the amount of heap-allocated
//! memory in use by specific instances of types at any point in time, without any
//! changes to the way in which these types are constructed.
//!
//! ## Minimum Supported Rust Version
//!
//! Requires Rust **1.51** or newer.
//!
//! In the future, we reserve the right to change MSRV (i.e. MSRV is out-of-scope for this
//! crate's SemVer guarantees), however when we do it will be accompanied by a minor
//! version bump.
//!
//! ## Usage
//!
//! ```
//! # use std::collections::HashMap;
//! use memuse::DynamicUsage;
//!
//! // Simple types don't allocate memory on the heap.
//! assert_eq!(7u64.dynamic_usage(), 0);
//! assert_eq!("I'm simple!".dynamic_usage(), 0);
//!
//! // When a type allocates memory, we can see it!
//! assert_eq!(vec![7u64; 2].dynamic_usage(), 16);
//!
//! // We see the memory the type has allocated, even if it isn't being used.
//! let empty: Vec<u32> = Vec::with_capacity(100);
//! assert_eq!(empty.len(), 0);
//! assert_eq!(empty.dynamic_usage(), 400);
//!
//! // For some types, we can't measure the exact memory usage, so we return a best
//! // estimate. If you need precision, call `dynamic_usage_bounds` which returns a
//! // lower bound, and (if known) an upper bound.
//! let map: HashMap<u8, u64> = HashMap::with_capacity(27);
//! let (lower, upper): (usize, Option<usize>) = map.dynamic_usage_bounds();
//! assert!(upper.is_none());
//! ```

#![forbid(unsafe_code)]
// Catch documentation errors caused by code changes.
#![deny(broken_intra_doc_links)]

use core::mem;
use std::collections::{BinaryHeap, LinkedList, VecDeque};

/// Trait for measuring the dynamic memory usage of types.
pub trait DynamicUsage {
    /// Returns a best estimate of the amount of heap-allocated memory used by this type.
    ///
    /// For most types, this will return an exact value. However, for types that use a
    /// complex allocation strategy (such as a `HashMap`), `memuse` cannot provide an
    /// exact heap allocation value, as it does not have access to the internal details
    /// and can only infer allocations from observable properties (such as the number of
    /// elements in a collection, or constants extracted from the implementation of the
    /// type). In those cases, this method returns a "best estimate" inferred from the
    /// implemented behaviour of the type. As more crates implement this trait themselves,
    /// the estimates will become more precise.
    ///
    /// The value returned by this method will always fall between the bounds returned by
    /// [`DynamicUsage::dynamic_usage_bounds`]:
    ///
    /// ```
    /// use std::collections::HashMap;
    /// use memuse::DynamicUsage;
    ///
    /// let a: HashMap<u8, u64> = HashMap::with_capacity(27);
    /// let usage = a.dynamic_usage();
    /// let (lower, upper) = a.dynamic_usage_bounds();
    ///
    /// assert!(lower <= usage);
    /// if let Some(upper) = upper {
    ///     assert!(usage <= upper);
    /// }
    /// ```
    fn dynamic_usage(&self) -> usize;

    /// Returns the lower and upper bounds on the amount of heap-allocated memory used by
    /// this type.
    ///
    /// The lower bound is always precise; a type cannot allocate fewer than zero bytes,
    /// and a collection cannot allocate fewer than the number of bytes required to store
    /// the entries it holds.
    ///
    /// The upper bound is only present if some property of the type ensures that its
    /// allocations do not exceed the bound, and is `None` otherwise (to indicate an
    /// unlimited upper bound).
    ///
    /// If the type's allocated memory is precisely known, then the lower and upper bounds
    /// will be equal.
    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>);
}

//
// Helper macros
//

/// Helper to implement [`DynamicUsage`] for simple types that don't allocate.
///
/// # Examples
///
/// ```
/// // Must be imported so it is accessible to the macro.
/// use memuse::DynamicUsage;
///
/// struct RegisterByte(u8);
/// struct RegisterWord(u16);
///
/// memuse::impl_no_dynamic_usage!(RegisterByte, RegisterWord);
/// ```
///
/// The above is equivalent to:
/// ```
/// use memuse::DynamicUsage;
///
/// struct RegisterByte(u8);
/// struct RegisterWord(u16);
///
/// impl DynamicUsage for RegisterByte {
///     #[inline(always)]
///     fn dynamic_usage(&self) -> usize {
///         0
///     }
///
///     #[inline(always)]
///     fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
///         (0, Some(0))
///     }
/// }
///
/// impl DynamicUsage for RegisterWord {
///     #[inline(always)]
///     fn dynamic_usage(&self) -> usize {
///         0
///     }
///
///     #[inline(always)]
///     fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
///         (0, Some(0))
///     }
/// }
/// ```
#[macro_export]
macro_rules! impl_no_dynamic_usage {
    ($($type:ty),+) => {
        $(
            impl DynamicUsage for $type {
                #[inline(always)]
                fn dynamic_usage(&self) -> usize {
                    0
                }

                #[inline(always)]
                fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
                    (0, Some(0))
                }
            }
        )+
    };
}

macro_rules! impl_iterable_dynamic_usage {
    ($type:ty, $base_usage:expr) => {
        impl<T: DynamicUsage> DynamicUsage for $type {
            fn dynamic_usage(&self) -> usize {
                $base_usage(self) + self.iter().map(DynamicUsage::dynamic_usage).sum::<usize>()
            }

            fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
                let base = $base_usage(self);
                let (lower, upper) = self.iter().map(DynamicUsage::dynamic_usage_bounds).fold(
                    (0, Some(0)),
                    |(acc_lower, acc_upper), (lower, upper)| {
                        (acc_lower + lower, acc_upper.zip(upper).map(|(a, b)| a + b))
                    },
                );
                (base + lower, upper.map(|u| base + u))
            }
        }
    };
}

//
// Primitives
//

impl_no_dynamic_usage!(());
impl_no_dynamic_usage!(i8, i16, i32, i64, i128, isize);
impl_no_dynamic_usage!(u8, u16, u32, u64, u128, usize);
impl_no_dynamic_usage!(f32, f64, bool);
impl_no_dynamic_usage!(char, str);

// Tuples are handled below (so they render more nicely in docs)

impl<T: DynamicUsage, const N: usize> DynamicUsage for [T; N] {
    fn dynamic_usage(&self) -> usize {
        self.iter().map(DynamicUsage::dynamic_usage).sum::<usize>()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        self.iter().map(DynamicUsage::dynamic_usage_bounds).fold(
            (0, Some(0)),
            |(acc_lower, acc_upper), (lower, upper)| {
                (acc_lower + lower, acc_upper.zip(upper).map(|(a, b)| a + b))
            },
        )
    }
}

impl_iterable_dynamic_usage!([T], |_| 0);

//
// Structs
//

impl DynamicUsage for String {
    fn dynamic_usage(&self) -> usize {
        self.capacity()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        let usage = self.capacity();
        (usage, Some(usage))
    }
}

//
// Containers
//

impl<T: DynamicUsage> DynamicUsage for Box<T> {
    fn dynamic_usage(&self) -> usize {
        mem::size_of::<T>() + self.as_ref().dynamic_usage()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        let box_size = mem::size_of::<T>();
        let (inner_lower, inner_upper) = self.as_ref().dynamic_usage_bounds();
        (box_size + inner_lower, inner_upper.map(|u| box_size + u))
    }
}

impl<T: DynamicUsage> DynamicUsage for Option<T> {
    fn dynamic_usage(&self) -> usize {
        self.as_ref().map(DynamicUsage::dynamic_usage).unwrap_or(0)
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        self.as_ref()
            .map(DynamicUsage::dynamic_usage_bounds)
            .unwrap_or((0, Some(0)))
    }
}

impl<T: DynamicUsage, E: DynamicUsage> DynamicUsage for Result<T, E> {
    fn dynamic_usage(&self) -> usize {
        match self {
            Ok(t) => t.dynamic_usage(),
            Err(e) => e.dynamic_usage(),
        }
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        match self {
            Ok(t) => t.dynamic_usage_bounds(),
            Err(e) => e.dynamic_usage_bounds(),
        }
    }
}

//
// Collections
//

impl_iterable_dynamic_usage!(Vec<T>, |c: &Vec<T>| c.capacity() * mem::size_of::<T>());

impl_iterable_dynamic_usage!(BinaryHeap<T>, |c: &BinaryHeap<T>| {
    // BinaryHeap<T> is a wrapper around Vec<T>
    c.capacity() * mem::size_of::<T>()
});

impl_iterable_dynamic_usage!(LinkedList<T>, |c: &LinkedList<T>| {
    c.len() * mem::size_of::<T>()
});

impl_iterable_dynamic_usage!(VecDeque<T>, |c: &VecDeque<T>| {
    // +1 since the ringbuffer always leaves one space empty.
    (c.capacity() + 1) * mem::size_of::<T>()
});

mod hash;

//
// External crate types (provided for helpfulness, since `DynamicUsage` can only be
// implemented either here or in the external crate).
//

#[cfg(feature = "nonempty")]
impl_iterable_dynamic_usage!(nonempty::NonEmpty<T>, |c: &nonempty::NonEmpty<T>| {
    // NonEmpty<T> stores its head element separately from its tail Vec<T>.
    (c.capacity() - 1) * mem::size_of::<T>()
});

//
// Larger definitions (placed at the end so they render more nicely in docs).
//

mod tuple;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn standard_types() {
        assert_eq!(129u8.dynamic_usage(), 0);
        assert_eq!(3i128.dynamic_usage(), 0);
        assert_eq!(7.0f32.dynamic_usage(), 0);
        assert_eq!("foobar".dynamic_usage(), 0);

        assert_eq!(129u8.dynamic_usage_bounds(), (0, Some(0)));
        assert_eq!(3i128.dynamic_usage_bounds(), (0, Some(0)));
        assert_eq!(7.0f32.dynamic_usage_bounds(), (0, Some(0)));
        assert_eq!("foobar".dynamic_usage_bounds(), (0, Some(0)));
    }

    #[test]
    fn string() {
        assert_eq!(String::new().dynamic_usage(), 0);
        assert_eq!("foobar".to_string().dynamic_usage(), 6);

        assert_eq!(String::new().dynamic_usage_bounds(), (0, Some(0)));
        assert_eq!("foobar".to_string().dynamic_usage_bounds(), (6, Some(6)));
    }

    #[test]
    fn boxed() {
        let a: u64 = 7;
        assert_eq!(a.dynamic_usage(), 0);
        assert_eq!(a.dynamic_usage_bounds(), (0, Some(0)));

        let b: Box<u64> = Box::new(42);
        assert_eq!(b.dynamic_usage(), 8);
        assert_eq!(b.dynamic_usage_bounds(), (8, Some(8)));

        let capacity = 7;
        let c: Box<Vec<u16>> = Box::new(Vec::with_capacity(capacity));
        let expected = mem::size_of::<Vec<u16>>() + capacity * mem::size_of::<u16>();
        assert_eq!(c.dynamic_usage(), expected);
        assert_eq!(c.dynamic_usage_bounds(), (expected, Some(expected)));
    }

    #[test]
    fn option() {
        let a: Option<Vec<u8>> = None;
        let b: Option<Vec<u8>> = Some(vec![7u8; 4]);
        assert_eq!(a.dynamic_usage(), 0);
        assert_eq!(a.dynamic_usage_bounds(), (0, Some(0)));
        assert_eq!(b.dynamic_usage(), 4);
        assert_eq!(b.dynamic_usage_bounds(), (4, Some(4)));
    }

    #[test]
    fn array() {
        let a = [7; 42];
        assert_eq!(a.dynamic_usage(), 0);
        assert_eq!(a.dynamic_usage_bounds(), (0, Some(0)));

        let mut b = [None, None, None, None];
        assert_eq!(b.dynamic_usage(), 0);
        assert_eq!(b.dynamic_usage_bounds(), (0, Some(0)));

        b[0] = Some(vec![4u8; 20]);
        assert_eq!(b.dynamic_usage(), 20);
        assert_eq!(b.dynamic_usage_bounds(), (20, Some(20)));
    }

    #[test]
    fn vec() {
        let capacity = 7;
        let mut a = Vec::with_capacity(capacity);
        a.push(42u64);

        let expected = capacity * mem::size_of::<u64>();
        assert_eq!(a.dynamic_usage(), expected);
        assert_eq!(a.dynamic_usage_bounds(), (expected, Some(expected)));
    }

    #[cfg(feature = "nonempty")]
    #[test]
    fn nonempty() {
        let a = nonempty::NonEmpty::new(42);
        assert_eq!(a.dynamic_usage(), 0);
        assert_eq!(a.dynamic_usage_bounds(), (0, Some(0)));

        const CAPACITY: usize = 7;
        let b = nonempty::NonEmpty::from_slice(&[27u128; CAPACITY]).unwrap();

        let expected = (CAPACITY - 1) * mem::size_of::<u128>();
        assert_eq!(b.dynamic_usage(), expected);
        assert_eq!(b.dynamic_usage_bounds(), (expected, Some(expected)));
    }
}
