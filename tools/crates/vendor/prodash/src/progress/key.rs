use std::ops::{Index, IndexMut};

use crate::progress::Task;

/// a level in the hierarchy of key components
///
/// _NOTE:_ This means we will show weird behaviour if there are more than 2^16 tasks at the same time on a level
/// as multiple progress handles will manipulate the same state.
pub type Level = u8;

pub(crate) type Id = u16;

/// A type identifying a spot in the hierarchy of `Tree` items.
#[derive(Copy, Clone, Default, Hash, Eq, PartialEq, Ord, PartialOrd, Debug)]
pub struct Key(Option<Id>, Option<Id>, Option<Id>, Option<Id>, Option<Id>, Option<Id>);

/// Determines if a sibling is above or below in the given level of hierarchy
#[derive(Default, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Debug)]
#[allow(missing_docs)]
pub enum SiblingLocation {
    Above,
    Below,
    AboveAndBelow,
    #[default]
    NotFound,
}

impl SiblingLocation {
    fn merge(&mut self, other: SiblingLocation) {
        use SiblingLocation::*;
        *self = match (*self, other) {
            (any, NotFound) => any,
            (NotFound, any) => any,
            (Above, Below) => AboveAndBelow,
            (Below, Above) => AboveAndBelow,
            (AboveAndBelow, _) => AboveAndBelow,
            (_, AboveAndBelow) => AboveAndBelow,
            (Above, Above) => Above,
            (Below, Below) => Below,
        };
    }
}

/// A type providing information about what's above and below `Tree` items.
#[derive(Copy, Clone, Default, Eq, PartialEq, Ord, PartialOrd, Debug)]
pub struct Adjacency(
    pub SiblingLocation,
    pub SiblingLocation,
    pub SiblingLocation,
    pub SiblingLocation,
    pub SiblingLocation,
    pub SiblingLocation,
);

impl Adjacency {
    /// Return the level at which this sibling is located in the hierarchy.
    pub fn level(&self) -> Level {
        use SiblingLocation::*;
        match self {
            Adjacency(NotFound, NotFound, NotFound, NotFound, NotFound, NotFound) => 0,
            Adjacency(_a, NotFound, NotFound, NotFound, NotFound, NotFound) => 1,
            Adjacency(_a, _b, NotFound, NotFound, NotFound, NotFound) => 2,
            Adjacency(_a, _b, _c, NotFound, NotFound, NotFound) => 3,
            Adjacency(_a, _b, _c, _d, NotFound, NotFound) => 4,
            Adjacency(_a, _b, _c, _d, _e, NotFound) => 5,
            Adjacency(_a, _b, _c, _d, _e, _f) => 6,
        }
    }
    /// Get a reference to the sibling location at `level`.
    pub fn get(&self, level: Level) -> Option<&SiblingLocation> {
        Some(match level {
            1 => &self.0,
            2 => &self.1,
            3 => &self.2,
            4 => &self.3,
            5 => &self.4,
            6 => &self.5,
            _ => return None,
        })
    }
    /// Get a mutable reference to the sibling location at `level`.
    pub fn get_mut(&mut self, level: Level) -> Option<&mut SiblingLocation> {
        Some(match level {
            1 => &mut self.0,
            2 => &mut self.1,
            3 => &mut self.2,
            4 => &mut self.3,
            5 => &mut self.4,
            6 => &mut self.5,
            _ => return None,
        })
    }
}

impl Index<Level> for Adjacency {
    type Output = SiblingLocation;
    fn index(&self, index: Level) -> &Self::Output {
        self.get(index).expect("adjacency index in bound")
    }
}

impl IndexMut<Level> for Adjacency {
    fn index_mut(&mut self, index: Level) -> &mut Self::Output {
        self.get_mut(index).expect("adjacency index in bound")
    }
}

impl Key {
    /// Return the key to the child identified by `child_id` located in a new nesting level below `self`.
    pub fn add_child(self, child_id: Id) -> Key {
        match self {
            Key(None, None, None, None, None, None) => Key(Some(child_id), None, None, None, None, None),
            Key(a, None, None, None, None, None) => Key(a, Some(child_id), None, None, None, None),
            Key(a, b, None, None, None, None) => Key(a, b, Some(child_id), None, None, None),
            Key(a, b, c, None, None, None) => Key(a, b, c, Some(child_id), None, None),
            Key(a, b, c, d, None, None) => Key(a, b, c, d, Some(child_id), None),
            Key(a, b, c, d, e, _f) => {
                crate::warn!("Maximum nesting level reached. Adding tasks to current parent");
                Key(a, b, c, d, e, Some(child_id))
            }
        }
    }

    /// The level of hierarchy a node is placed in, i.e. the amount of path components
    pub fn level(&self) -> Level {
        match self {
            Key(None, None, None, None, None, None) => 0,
            Key(Some(_), None, None, None, None, None) => 1,
            Key(Some(_), Some(_), None, None, None, None) => 2,
            Key(Some(_), Some(_), Some(_), None, None, None) => 3,
            Key(Some(_), Some(_), Some(_), Some(_), None, None) => 4,
            Key(Some(_), Some(_), Some(_), Some(_), Some(_), None) => 5,
            Key(Some(_), Some(_), Some(_), Some(_), Some(_), Some(_)) => 6,
            _ => unreachable!("This is a bug - Keys follow a certain pattern"),
        }
    }

    /// Return the identifier for the item at `level`.
    fn get(&self, level: Level) -> Option<&Id> {
        match level {
            1 => self.0.as_ref(),
            2 => self.1.as_ref(),
            3 => self.2.as_ref(),
            4 => self.3.as_ref(),
            5 => self.4.as_ref(),
            6 => self.5.as_ref(),
            _ => None,
        }
    }

    /// Return true if the item identified by `other` shares the parent at `parent_level`.
    pub fn shares_parent_with(&self, other: &Key, parent_level: Level) -> bool {
        if parent_level < 1 {
            return true;
        }
        for level in 1..=parent_level {
            if let (Some(lhs), Some(rhs)) = (self.get(level), other.get(level)) {
                if lhs != rhs {
                    return false;
                }
            } else {
                return false;
            }
        }
        true
    }

    /// Compute the adjacency map for the key in `sorted` at the given `index`.
    ///
    /// It's vital that the invariant of `sorted` to actually be sorted by key is upheld
    /// for the result to be reliable.
    pub fn adjacency(sorted: &[(Key, Task)], index: usize) -> Adjacency {
        use SiblingLocation::*;
        let key = &sorted[index].0;
        let key_level = key.level();
        let mut adjecency = Adjacency::default();
        if key_level == 0 {
            return adjecency;
        }

        fn search<'a>(
            iter: impl Iterator<Item = &'a (Key, Task)>,
            key: &Key,
            key_level: Level,
            current_level: Level,
            _id_at_level: Id,
        ) -> Option<usize> {
            iter.map(|(k, _)| k)
                .take_while(|other| key.shares_parent_with(other, current_level.saturating_sub(1)))
                .enumerate()
                .find(|(_idx, k)| {
                    if current_level == key_level {
                        k.level() == key_level || k.level() + 1 == key_level
                    } else {
                        k.level() == current_level
                    }
                })
                .map(|(idx, _)| idx)
        }

        let upward_iter = |from: usize, key: &Key, level: Level, id_at_level: Id| {
            search(sorted[..from].iter().rev(), key, key_level, level, id_at_level)
        };
        let downward_iter = |from: usize, key: &Key, level: Level, id_at_level: Id| {
            sorted
                .get(from + 1..)
                .and_then(|s| search(s.iter(), key, key_level, level, id_at_level))
        };

        {
            let mut cursor = index;
            for level in (1..=key_level).rev() {
                if level == 1 {
                    adjecency[level].merge(Above); // the root or any other sibling on level one
                    continue;
                }
                if let Some(key_offset) = upward_iter(cursor, key, level, key[level]) {
                    cursor = index.saturating_sub(key_offset);
                    adjecency[level].merge(Above);
                }
            }
        }
        {
            let mut cursor = index;
            for level in (1..=key_level).rev() {
                if let Some(key_offset) = downward_iter(cursor, key, level, key[level]) {
                    cursor = index + key_offset;
                    adjecency[level].merge(Below);
                }
            }
        }
        for level in 1..key_level {
            if key_level == 1 && index + 1 == sorted.len() {
                continue;
            }
            adjecency[level] = match adjecency[level] {
                Above | Below | NotFound => NotFound,
                AboveAndBelow => AboveAndBelow,
            };
        }
        adjecency
    }

    /// The maximum amount of path components we can represent.
    pub const fn max_level() -> Level {
        6
    }
}

impl Index<Level> for Key {
    type Output = Id;

    fn index(&self, index: Level) -> &Self::Output {
        self.get(index).expect("key index in bound")
    }
}
