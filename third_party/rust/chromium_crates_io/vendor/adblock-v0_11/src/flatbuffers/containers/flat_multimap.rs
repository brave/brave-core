use std::{collections::HashMap, marker::PhantomData};

use crate::flatbuffers::containers;
use containers::flat_serialize::{FlatBuilder, FlatMapBuilderOutput, FlatSerialize};
use containers::sorted_index::SortedIndex;
use flatbuffers::{Follow, Vector};

/// A map-like container that uses flatbuffer references.
/// Provides O(log n) lookup time using binary search on the sorted index.
/// I is a key type, Keys is specific container of keys, &[I] for fast indexing (u32, u64)
/// and flatbuffers::Vector<I> if there is no conversion from Vector (str) to slice.
pub(crate) struct FlatMultiMapView<'a, I: Ord, V, Keys>
where
    Keys: SortedIndex<I>,
    V: Follow<'a>,
{
    keys: Keys,
    values: Vector<'a, V>,
    _phantom: PhantomData<I>,
}

impl<'a, I: Ord + Copy, V, Keys> FlatMultiMapView<'a, I, V, Keys>
where
    Keys: SortedIndex<I> + Clone,
    V: Follow<'a>,
{
    pub fn new(keys: Keys, values: Vector<'a, V>) -> Self {
        debug_assert!(keys.len() == values.len());

        Self {
            keys,
            values,
            _phantom: PhantomData,
        }
    }

    pub fn get(&self, key: I) -> Option<FlatMultiMapViewIterator<'a, I, V, Keys>> {
        let index = self.keys.partition_point(|x| *x < key);
        if index < self.keys.len() && self.keys.get(index) == key {
            Some(FlatMultiMapViewIterator {
                index,
                key,
                keys: self.keys.clone(), // Cloning is 3-4% faster than & in benchmarks
                values: self.values,
            })
        } else {
            None
        }
    }

    #[cfg(test)]
    pub fn total_size(&self) -> usize {
        self.keys.len()
    }
}

pub(crate) struct FlatMultiMapViewIterator<'a, I: Ord + Copy, V, Keys>
where
    Keys: SortedIndex<I>,
    V: Follow<'a>,
{
    index: usize,
    key: I,
    keys: Keys,
    values: Vector<'a, V>,
}

impl<'a, I, V, Keys> Iterator for FlatMultiMapViewIterator<'a, I, V, Keys>
where
    I: Ord + Copy,
    V: Follow<'a>,
    Keys: SortedIndex<I>,
{
    type Item = (usize, <V as Follow<'a>>::Inner);

    fn next(&mut self) -> Option<Self::Item> {
        if self.index < self.keys.len() && self.keys.get(self.index) == self.key {
            self.index += 1;
            Some((self.index - 1, self.values.get(self.index - 1)))
        } else {
            None
        }
    }
}

#[derive(Default)]
pub(crate) struct FlatMultiMapBuilder<I, V> {
    map: HashMap<I, Vec<V>>,
}

impl<I: Ord + std::hash::Hash, V> FlatMultiMapBuilder<I, V> {
    pub fn from_filter_map(map: HashMap<I, Vec<V>>) -> Self {
        Self { map }
    }

    #[allow(dead_code)] // Unused code is allowed during cosmetic filter migration
    pub fn insert(&mut self, key: I, value: V) {
        self.map.entry(key).or_default().push(value);
    }

    pub fn finish<'a, B: FlatBuilder<'a>>(
        value: Self,
        builder: &mut B,
    ) -> FlatMapBuilderOutput<'a, I, V, B>
    where
        I: FlatSerialize<'a, B>,
        V: FlatSerialize<'a, B>,
    {
        let mut entries: Vec<_> = value.map.into_iter().collect();
        entries.sort_unstable_by(|(a, _), (b, _)| a.cmp(b));
        let mut indexes = Vec::with_capacity(entries.len());
        let mut values = Vec::with_capacity(entries.len());

        for (key, mv) in entries.into_iter() {
            let index = FlatSerialize::serialize(key, builder);
            for value in mv.into_iter() {
                indexes.push(index.clone());
                values.push(FlatSerialize::serialize(value, builder));
            }
        }

        let indexes_vec = builder.raw_builder().create_vector(&indexes);
        let values_vec = builder.raw_builder().create_vector(&values);

        FlatMapBuilderOutput {
            keys: indexes_vec,
            values: values_vec,
        }
    }
}

#[cfg(test)]
#[path = "../../../tests/unit/flatbuffers/containers/flat_multimap.rs"]
mod unit_tests;
