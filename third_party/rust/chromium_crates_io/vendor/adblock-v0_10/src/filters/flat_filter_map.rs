//! Holds the implementation of [FlatFilterMap].

use flatbuffers::{Follow, ForwardsUOffset, Vector};
use std::cmp::PartialOrd;

/// A map-like container that uses flatbuffer references.
/// Provides O(log n) lookup time using binary search on the sorted index.
pub(crate) struct FlatFilterMap<'a, I: PartialOrd + Copy, V> {
    index: &'a [I],
    values: Vector<'a, ForwardsUOffset<V>>,
}

/// Iterator over NetworkFilter objects from [FlatFilterMap]
pub(crate) struct FlatFilterMapIterator<'a, I: PartialOrd + Copy, V> {
    current_index: usize,
    key: I,
    indexes: &'a [I],
    values: Vector<'a, ForwardsUOffset<V>>,
}

impl<'a, I, V> Iterator for FlatFilterMapIterator<'a, I, V>
where
    I: PartialOrd + Copy,
    V: Follow<'a>,
{
    type Item = (usize, <V as Follow<'a>>::Inner);

    fn next(&mut self) -> Option<Self::Item> {
        if self.current_index < self.indexes.len() {
            if self.indexes[self.current_index] != self.key {
                return None;
            }
            let index = self.current_index;
            let filter = self.values.get(self.current_index);
            self.current_index += 1;
            Some((index, filter))
        } else {
            None
        }
    }
}

impl<'a, I: PartialOrd + Copy, V> FlatFilterMap<'a, I, V> {
    /// Construct [FlatFilterMap] from two vectors:
    /// - index: sorted array of keys
    /// - values: array of values, same length as index
    pub fn new(index: &'a [I], values: Vector<'a, ForwardsUOffset<V>>) -> Self {
        // Sanity check the size are equal. Note: next() will handle |values| correctly.
        debug_assert!(index.len() == values.len());

        debug_assert!(index.is_sorted());

        Self { index, values }
    }

    /// Get an iterator over NetworkFilter objects with the given hash key.
    pub fn get(&self, key: I) -> FlatFilterMapIterator<'a, I, V> {
        let start = self.index.partition_point(|x| *x < key);
        FlatFilterMapIterator {
            current_index: start,
            key,
            indexes: self.index,
            values: self.values,
        }
    }
}

impl<I: PartialOrd + Copy, V> FlatFilterMap<'_, I, V> {
    #[cfg(test)]
    pub fn total_size(&self) -> usize {
        self.index.len()
    }
}
