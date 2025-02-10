/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#![no_std]
#![deny(unsafe_code)]

//! A simple, fast, least-recently-used (LRU) cache.
//!
//! [`LRUCache`] uses a fixed-capacity array for storage. It provides `O(1)` insertion, and `O(n)`
//! lookup.  It does not require an allocator and can be used in `no_std` crates.
//!
//! See the [`LRUCache`] docs for details.

use arrayvec::ArrayVec;
use core::mem::replace;

#[cfg(test)]
mod tests;

/// A LRU cache using a statically-sized array for storage.
///
/// `LRUCache` uses a fixed-capacity array for storage. It provides `O(1)` insertion, and `O(n)`
/// lookup.
///
/// All items are stored inline within the `LRUCache`, so it does not impose any heap allocation or
/// indirection.  A linked list is used to record the cache order, so the items themselves do not
/// need to be moved when the order changes.  (This is important for speed if the items are large.)
///
/// # Example
///
/// ```
/// use uluru::LRUCache;
///
/// struct MyValue {
///     id: u32,
///     name: &'static str,
/// }
///
/// // A cache with a capacity of three.
/// type MyCache = LRUCache<MyValue, 3>;
///
/// // Create an empty cache, then insert some items.
/// let mut cache = MyCache::default();
/// cache.insert(MyValue { id: 1, name: "Mercury" });
/// cache.insert(MyValue { id: 2, name: "Venus" });
/// cache.insert(MyValue { id: 3, name: "Earth" });
///
/// // Use the `find` method to retrieve an item from the cache.
/// // This also "touches" the item, marking it most-recently-used.
/// let item = cache.find(|x| x.id == 1);
/// assert_eq!(item.unwrap().name, "Mercury");
///
/// // If the cache is full, inserting a new item evicts the least-recently-used item:
/// cache.insert(MyValue { id: 4, name: "Mars" });
/// assert!(cache.find(|x| x.id == 2).is_none());
/// ```
#[derive(Debug, Clone)]
pub struct LRUCache<T, const N: usize> {
    /// The most-recently-used entry is at index `head`. The entries form a linked list, linked to
    /// each other by indices within the `entries` array.  After an entry is added to the array,
    /// its index never changes, so these links are never invalidated.
    entries: ArrayVec<Entry<T>, N>,
    /// Index of the first entry. If the cache is empty, ignore this field.
    head: u16,
    /// Index of the last entry. If the cache is empty, ignore this field.
    tail: u16,
}

/// An entry in an `LRUCache`.
#[derive(Debug, Clone)]
struct Entry<T> {
    val: T,
    /// Index of the previous entry. If this entry is the head, ignore this field.
    prev: u16,
    /// Index of the next entry. If this entry is the tail, ignore this field.
    next: u16,
}

impl<T, const N: usize> Default for LRUCache<T, N> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T, const N: usize> LRUCache<T, N> {
    /// Create an empty cache.
    pub const fn new() -> Self {
        assert!(N < u16::MAX as usize, "Capacity overflow");
        LRUCache {
            entries: ArrayVec::new_const(),
            head: 0,
            tail: 0,
        }
    }

    /// Insert a given key in the cache.
    ///
    /// This item becomes the front (most-recently-used) item in the cache.  If the cache is full,
    /// the back (least-recently-used) item will be removed and returned.
    pub fn insert(&mut self, val: T) -> Option<T> {
        let new_entry = Entry {
            val,
            prev: 0,
            next: 0,
        };

        // If the cache is full, replace the oldest entry. Otherwise, add an entry.
        if self.entries.is_full() {
            let i = self.pop_back();
            let old_entry = replace(self.entry(i), new_entry);
            self.push_front(i);
            Some(old_entry.val)
        } else {
            let i = self.entries.len() as u16;
            self.entries.push(new_entry);
            self.push_front(i);
            None
        }
    }

    /// Returns the first item in the cache that matches the given predicate.
    /// Touches the result (makes it most-recently-used) on a hit.
    pub fn find<F>(&mut self, pred: F) -> Option<&mut T>
    where
        F: FnMut(&T) -> bool,
    {
        if self.touch(pred) {
            self.front_mut()
        } else {
            None
        }
    }

    /// Performs a lookup on the cache with the given test routine. Touches
    /// the result on a hit.
    pub fn lookup<F, R>(&mut self, mut pred: F) -> Option<R>
    where
        F: FnMut(&mut T) -> Option<R>,
    {
        let mut iter = self.iter_mut();
        while let Some((i, val)) = iter.next() {
            if let Some(r) = pred(val) {
                self.touch_index(i);
                return Some(r);
            }
        }
        None
    }

    /// Returns the number of elements in the cache.
    #[inline]
    pub fn len(&self) -> usize {
        self.entries.len()
    }

    /// Returns true if the cache is empty.
    #[inline]
    pub fn is_empty(&self) -> bool {
        self.entries.is_empty()
    }

    /// Evict all elements from the cache.
    #[inline]
    pub fn clear(&mut self) {
        self.entries.clear();
    }

    /// Returns the front entry in the list (most recently used).
    pub fn front(&self) -> Option<&T> {
        self.entries.get(self.head as usize).map(|e| &e.val)
    }

    /// Returns a mutable reference to the front entry in the list (most recently used).
    pub fn front_mut(&mut self) -> Option<&mut T> {
        self.entries.get_mut(self.head as usize).map(|e| &mut e.val)
    }

    /// Returns the n-th entry in the list (most recently used).
    pub fn get(&self, index: usize) -> Option<&T> {
        self.iter().nth(index)
    }

    /// Touches the first item in the cache that matches the given predicate (marks it as
    /// most-recently-used).
    /// Returns `true` on a hit, `false` if no matches.
    pub fn touch<F>(&mut self, mut pred: F) -> bool
    where
        F: FnMut(&T) -> bool,
    {
        let mut iter = self.iter_mut();
        while let Some((i, val)) = iter.next() {
            if pred(val) {
                self.touch_index(i);
                return true;
            }
        }
        false
    }

    /// Iterate over the contents of this cache in order from most-recently-used to
    /// least-recently-used.
    pub fn iter(&self) -> Iter<'_, T, N> {
        Iter {
            pos: self.head,
            cache: self,
        }
    }

    /// Iterate mutably over the contents of this cache in order from most-recently-used to
    /// least-recently-used.
    fn iter_mut(&mut self) -> IterMut<'_, T, N> {
        IterMut {
            pos: self.head,
            cache: self,
        }
    }

    /// Touch a given entry, putting it first in the list.
    #[inline]
    fn touch_index(&mut self, i: u16) {
        if i != self.head {
            self.remove(i);
            self.push_front(i);
        }
    }

    #[inline(always)]
    fn entry(&mut self, i: u16) -> &mut Entry<T> {
        &mut self.entries[i as usize]
    }

    /// Remove an entry from the linked list.
    ///
    /// Note: This only unlinks the entry from the list; it does not remove it from the array.
    fn remove(&mut self, i: u16) {
        let prev = self.entry(i).prev;
        let next = self.entry(i).next;

        if i == self.head {
            self.head = next;
        } else {
            self.entry(prev).next = next;
        }

        if i == self.tail {
            self.tail = prev;
        } else {
            self.entry(next).prev = prev;
        }
    }

    /// Insert a new entry at the head of the list.
    fn push_front(&mut self, i: u16) {
        if self.entries.len() == 1 {
            self.tail = i;
        } else {
            self.entry(i).next = self.head;
            self.entry(self.head).prev = i;
        }
        self.head = i;
    }

    /// Remove the last entry from the linked list. Returns the index of the removed entry.
    ///
    /// Note: This only unlinks the entry from the list; it does not remove it from the array.
    fn pop_back(&mut self) -> u16 {
        let new_tail = self.entry(self.tail).prev;
        replace(&mut self.tail, new_tail)
    }
}

/// Mutable iterator over values in an `LRUCache`, from most-recently-used to least-recently-used.
struct IterMut<'a, T, const N: usize> {
    cache: &'a mut LRUCache<T, N>,
    pos: u16,
}

impl<'a, T, const N: usize> IterMut<'a, T, N> {
    fn next(&mut self) -> Option<(u16, &mut T)> {
        let index = self.pos;
        let entry = self.cache.entries.get_mut(index as usize)?;

        self.pos = if index == self.cache.tail {
            N as u16 // Point past the end of the array to signal we are done.
        } else {
            entry.next
        };
        Some((index, &mut entry.val))
    }
}

/// Iterator over values in an [`LRUCache`], from most-recently-used to least-recently-used.
pub struct Iter<'a, T, const N: usize> {
    cache: &'a LRUCache<T, N>,
    pos: u16,
}

impl<'a, T, const N: usize> Iterator for Iter<'a, T, N> {
    type Item = &'a T;

    fn next(&mut self) -> Option<&'a T> {
        let entry = self.cache.entries.get(self.pos as usize)?;

        self.pos = if self.pos == self.cache.tail {
            N as u16 // Point past the end of the array to signal we are done.
        } else {
            entry.next
        };
        Some(&entry.val)
    }
}
