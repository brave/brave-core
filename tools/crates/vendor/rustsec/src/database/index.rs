//! Database indexes

pub use crate::set::Iter;

use super::entries::Slot;
use crate::{Map, Set, map, package};

/// Database index which maps package names to a set of advisory IDs
#[derive(Debug, Default)]
pub(crate) struct Index(Map<package::Name, Set<Slot>>);

impl Index {
    /// Create a new index
    pub fn new() -> Self {
        Self::default()
    }

    /// Insert an entry into the index
    pub fn insert(&mut self, key: &package::Name, slot: Slot) -> bool {
        let values = match self.0.entry(key.clone()) {
            map::Entry::Vacant(entry) => entry.insert(Set::new()),
            map::Entry::Occupied(entry) => entry.into_mut(),
        };

        values.insert(slot)
    }

    /// Get an iterator over advisory IDs for a given package name
    pub fn get(&self, key: &package::Name) -> Option<Iter<'_, Slot>> {
        self.0.get(key).map(|set| set.iter())
    }
}
