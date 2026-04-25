use crate::map::IdHashSet;
use id_arena::Arena as InnerArena;
use std::ops::{Index, IndexMut};

#[cfg(feature = "parallel")]
use rayon::iter::plumbing::UnindexedConsumer;
#[cfg(feature = "parallel")]
use rayon::prelude::*;

pub use id_arena::Id;

/// A wrapper around an `id_arena::Arena` that adds a tombstone set for deleting
/// items.
#[derive(Debug)]
pub struct TombstoneArena<T> {
    inner: InnerArena<T>,
    dead: IdHashSet<T>,
}

// Note: can't derive because that would require `T: Default`.
impl<T> Default for TombstoneArena<T> {
    fn default() -> TombstoneArena<T> {
        TombstoneArena {
            inner: Default::default(),
            dead: Default::default(),
        }
    }
}

/// Like `Drop` but for after an item is marked deleted from a `TombstoneArena`.
///
/// Note that this is *not* setting the item to a tombstone (eg turning an
/// `Option` into a `None`). Instead, this is an opportunity to clean up heap
/// allocations or whatever other resource.
///
/// Note that the item must still be in a valid state after `on_delete`, since
/// its `Drop` implementation may still be called in the future if it has one.
pub trait Tombstone {
    /// "Drop" this item.
    fn on_delete(&mut self) {
        // Do nothing by default.
    }
}

impl<T> TombstoneArena<T>
where
    T: Tombstone,
{
    /// Delete the item with the given id from the arena.
    pub fn delete(&mut self, id: Id<T>) {
        assert!(self.contains(id));
        self.dead.insert(id);
        self.inner[id].on_delete();
    }
}

impl<T> TombstoneArena<T> {
    pub fn alloc(&mut self, val: T) -> Id<T> {
        self.inner.alloc(val)
    }

    pub fn alloc_with_id<F>(&mut self, f: F) -> Id<T>
    where
        F: FnOnce(Id<T>) -> T,
    {
        let id = self.next_id();
        self.alloc(f(id))
    }

    pub fn get(&self, id: Id<T>) -> Option<&T> {
        if self.dead.contains(&id) {
            None
        } else {
            self.inner.get(id)
        }
    }

    pub fn get_mut(&mut self, id: Id<T>) -> Option<&mut T> {
        if self.dead.contains(&id) {
            None
        } else {
            self.inner.get_mut(id)
        }
    }

    pub fn next_id(&self) -> Id<T> {
        self.inner.next_id()
    }

    pub fn len(&self) -> usize {
        self.inner.len() - self.dead.len()
    }

    pub fn contains(&self, id: Id<T>) -> bool {
        self.inner.get(id).is_some() && !self.dead.contains(&id)
    }

    pub fn iter(&self) -> impl Iterator<Item = (Id<T>, &T)> {
        self.inner
            .iter()
            .filter(move |&(id, _)| !self.dead.contains(&id))
    }

    pub fn iter_mut(&mut self) -> IterMut<T> {
        IterMut {
            dead: &self.dead,
            inner: self.inner.iter_mut(),
        }
    }

    #[cfg(feature = "parallel")]
    pub fn par_iter(&self) -> impl ParallelIterator<Item = (Id<T>, &T)>
    where
        T: Sync,
    {
        self.inner
            .par_iter()
            .filter(move |&(id, _)| !self.dead.contains(&id))
    }

    #[cfg(feature = "parallel")]
    pub fn par_iter_mut(&mut self) -> ParIterMut<T>
    where
        T: Send + Sync,
    {
        ParIterMut {
            dead: &self.dead,
            inner: self.inner.par_iter_mut(),
        }
    }
}

impl<T> Index<Id<T>> for TombstoneArena<T> {
    type Output = T;

    fn index(&self, id: Id<T>) -> &T {
        assert!(!self.dead.contains(&id));
        &self.inner[id]
    }
}

impl<T> IndexMut<Id<T>> for TombstoneArena<T> {
    fn index_mut(&mut self, id: Id<T>) -> &mut T {
        assert!(!self.dead.contains(&id));
        &mut self.inner[id]
    }
}

#[derive(Debug)]
pub struct IterMut<'a, T: 'a> {
    dead: &'a IdHashSet<T>,
    inner: id_arena::IterMut<'a, T, id_arena::DefaultArenaBehavior<T>>,
}

impl<'a, T: 'a> Iterator for IterMut<'a, T> {
    type Item = (Id<T>, &'a mut T);

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            match self.inner.next() {
                Some((id, _)) if self.dead.contains(&id) => continue,
                x => return x,
            }
        }
    }
}

#[derive(Debug)]
#[cfg(feature = "parallel")]
pub struct ParIterMut<'a, T: 'a + Send + Sync> {
    dead: &'a IdHashSet<T>,
    inner: id_arena::ParIterMut<'a, T, id_arena::DefaultArenaBehavior<T>>,
}

#[cfg(feature = "parallel")]
impl<'a, T> ParallelIterator for ParIterMut<'a, T>
where
    T: Send + Sync,
{
    type Item = (Id<T>, &'a mut T);

    fn drive_unindexed<C>(self, consumer: C) -> C::Result
    where
        C: UnindexedConsumer<Self::Item>,
    {
        let dead = self.dead;
        self.inner
            .filter(move |&(id, _)| !dead.contains(&id))
            .drive_unindexed(consumer)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::rc::Rc;

    struct Doggo {
        good_boi: Option<Rc<()>>,
    }

    // oh god this is the saddest impl I have ever written for sure T.T
    impl Tombstone for Doggo {
        fn on_delete(&mut self) {
            self.good_boi = None;
        }
    }

    #[test]
    fn can_delete() {
        let mut a = TombstoneArena::<Doggo>::default();

        let rc = Rc::new(());
        assert_eq!(Rc::strong_count(&rc), 1);

        let id = a.alloc(Doggo {
            good_boi: Some(rc.clone()),
        });

        assert_eq!(Rc::strong_count(&rc), 2);
        assert!(a.contains(id));

        a.delete(id);

        assert_eq!(
            Rc::strong_count(&rc),
            1,
            "the on_delete should have been called"
        );
        assert!(
            !a.contains(id),
            "and the arena no longer contains the doggo :("
        );
    }
}
