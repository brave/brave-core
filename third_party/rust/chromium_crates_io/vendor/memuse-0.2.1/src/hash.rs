//! `DynamicUsage` impls for `HashMap` and `HashSet`.
//!
//! Rust's `HashMap` and `HashSet` are backed by the the `hashbrown` crate.

use core::mem;
use std::{
    alloc::Layout,
    collections::{HashMap, HashSet},
};

use crate::DynamicUsage;

// The widths are sourced from here:
//   https://github.com/rust-lang/hashbrown/blob/dbd6dbe30a4076c0ea65ca5bd57036c27f3cc7c9/src/raw/mod.rs#L15-L36
//   https://github.com/rust-lang/hashbrown/blob/dbd6dbe30a4076c0ea65ca5bd57036c27f3cc7c9/src/raw/generic.rs#L5-L21
//   https://github.com/rust-lang/hashbrown/blob/dbd6dbe30a4076c0ea65ca5bd57036c27f3cc7c9/src/raw/sse2.rs#L14-L19

#[cfg(all(target_feature = "sse2", target_arch = "x86", not(miri)))]
pub(crate) const WIDTH: usize = mem::size_of::<core::arch::x86::__m128i>();
#[cfg(all(target_feature = "sse2", target_arch = "x86_64", not(miri)))]
pub(crate) const WIDTH: usize = mem::size_of::<core::arch::x86_64::__m128i>();
#[cfg(all(
    any(not(target_feature = "sse2"), miri),
    any(
        target_pointer_width = "64",
        target_arch = "aarch64",
        target_arch = "x86_64",
        target_arch = "wasm32",
    )
))]
pub(crate) const WIDTH: usize = mem::size_of::<u64>();
#[cfg(all(
    any(not(target_feature = "sse2"), miri),
    target_pointer_width = "32",
    not(target_arch = "aarch64"),
    not(target_arch = "x86_64"),
    not(target_arch = "wasm32"),
))]
pub(crate) const WIDTH: usize = mem::size_of::<u32>();

fn dynamic_usage_for_capacity<K, V>(cap: usize) -> usize {
    // The bucket calculation is sourced from here:
    //   https://github.com/rust-lang/hashbrown/blob/dbd6dbe30a4076c0ea65ca5bd57036c27f3cc7c9/src/raw/mod.rs#L187-L216
    //
    // hashbrown's RawTable::buckets is not accessible via the std HashMap, and
    // HashMap::capacity is a lower bound. However, hashbrown has an invariant that
    // the number of buckets is a power of two, so usually we'll calculate the correct
    // memory usage, and occasionally we'll undercount by around a factor of two.
    let buckets = {
        if cap < 8 {
            return if cap < 4 { 4 } else { 8 };
        }
        let adjusted_cap = (cap * 8) / 7;
        adjusted_cap.next_power_of_two()
    };

    // The memory usage calculation is sourced from here:
    //   https://github.com/rust-lang/hashbrown/blob/dbd6dbe30a4076c0ea65ca5bd57036c27f3cc7c9/src/raw/mod.rs#L240-L265

    let layout = Layout::new::<(K, V)>();
    let size = layout.size();
    let ctrl_align = usize::max(layout.align(), WIDTH);

    let ctrl_offset = (size * buckets + ctrl_align - 1) & !(ctrl_align - 1);
    ctrl_offset + buckets + WIDTH
}

impl<K: DynamicUsage, V: DynamicUsage> DynamicUsage for HashMap<K, V> {
    fn dynamic_usage(&self) -> usize {
        dynamic_usage_for_capacity::<K, V>(self.capacity())
            + self
                .iter()
                .map(|(k, v)| k.dynamic_usage() + v.dynamic_usage())
                .sum::<usize>()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        (
            dynamic_usage_for_capacity::<K, V>(self.capacity())
                + self
                    .iter()
                    .map(|(k, v)| k.dynamic_usage_bounds().0 + v.dynamic_usage_bounds().0)
                    .sum::<usize>(),
            None,
        )
    }
}

impl<T: DynamicUsage> DynamicUsage for HashSet<T> {
    fn dynamic_usage(&self) -> usize {
        // HashSet<T> is just HashMap<T, ()>
        dynamic_usage_for_capacity::<T, ()>(self.capacity())
            + self.iter().map(DynamicUsage::dynamic_usage).sum::<usize>()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        (
            dynamic_usage_for_capacity::<T, ()>(self.capacity())
                + self
                    .iter()
                    .map(|k| k.dynamic_usage_bounds().0)
                    .sum::<usize>(),
            None,
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hashmap() {
        let h: HashMap<u16, u32> = HashMap::with_capacity(12);

        // - Capacity of 12 -> 16 buckets
        // - Overhead is 1 byte per bucket
        // - Fixed overhead of WIDTH
        let lower = 16 * (mem::size_of::<(u16, u32)>() + 1) + WIDTH;
        assert_eq!(h.dynamic_usage(), lower);
        assert_eq!(h.dynamic_usage_bounds(), (lower, None));
    }

    #[test]
    fn hashset() {
        let h: HashSet<u16> = HashSet::with_capacity(17);

        // - Capacity of 17 -> 32 buckets
        // - Overhead is 1 byte per bucket
        // - Fixed overhead of WIDTH
        let lower = 32 * (mem::size_of::<u16>() + 1) + WIDTH;
        assert_eq!(h.dynamic_usage(), lower);
        assert_eq!(h.dynamic_usage_bounds(), (lower, None));
    }
}
