//! Common types and utilities used in incremental Merkle tree implementations.
//!
//! # Navigating Tree Structure
//!
//! Several different abstractions are used for navigating tree structure. Consider this example
//! binary tree:
//!
//! ```text
//!      a
//!     / \
//!    /   \
//!   /     \
//!   b      c
//!  / \    / \
//! d   e  f   g
//! ````
//!
//! **Location Abstractions:**
//!
//! - [Level] represents the height from the leaves. Examples: `e` is level 0, `c` is level 1, `a`
//!   is level 2.
//! - `index` is the 0-based distance from the left-most node _on a given [Level]_. Examples: `f`
//!   has index 2, `c` has index 1, and `a` has index 0.
//! - [Position] is a type abstraction of the index of a leaf at [Level] 0.
//! - [Address] locates any node within a tree by representing the [Level] and `index`. Examples:
//!   the [Address] of `c` is at [Level] 1 and index 1, the [Address] of `f` is at [Level] 0,
//!   index 2.
//!
//! **Relationship Abstractions:**
//!
//! A given node has these navigational relationships:
//!
//! - `parent` - the node directly above at one higher [Level].
//! - `child` - the complementary relationship to parent; a parent may have up to two children
//!    because only binary trees are supported.
//! - `sibling` - the other node with the same parent.
//! - `cousin` - a node at the same [Level] excluding the sibling.
//! - `ommer` - the parent's sibling.
//! - `root` - the single node with the largest [Level].
//! - `ancestor` - the parent or an ancestor of the parent; the root is an ancestor of every
//!   other node.
//!
//! Note: we often refer to `ommers` (plural) when describing leaf-to-root paths, so in that
//! context `ommers` refers to the node's ommer, plus each ancestor's ommer.

#![cfg_attr(docsrs, feature(doc_cfg))]

use either::Either;
use std::cmp::Ordering;
use std::convert::{TryFrom, TryInto};
use std::fmt;
use std::num::TryFromIntError;
use std::ops::{Add, AddAssign, Range, Sub};

pub mod frontier;

#[cfg(feature = "legacy-api")]
#[cfg_attr(docsrs, doc(cfg(feature = "legacy-api")))]
pub mod witness;

#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub mod testing;

/// A type for metadata that is used to determine when and how a leaf can be pruned from a tree.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum Retention<C> {
    Ephemeral,
    Checkpoint { id: C, is_marked: bool },
    Marked,
}

impl<C> Retention<C> {
    pub fn is_checkpoint(&self) -> bool {
        matches!(self, Retention::Checkpoint { .. })
    }

    pub fn is_marked(&self) -> bool {
        match self {
            Retention::Ephemeral => false,
            Retention::Checkpoint { is_marked, .. } => *is_marked,
            Retention::Marked => true,
        }
    }

    pub fn map<'a, D, F: Fn(&'a C) -> D>(&'a self, f: F) -> Retention<D> {
        match self {
            Retention::Ephemeral => Retention::Ephemeral,
            Retention::Checkpoint { id, is_marked } => Retention::Checkpoint {
                id: f(id),
                is_marked: *is_marked,
            },
            Retention::Marked => Retention::Marked,
        }
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum Source {
    /// The sibling to the address can be derived from the incremental frontier
    /// at the contained ommer index
    Past(u8),
    /// The sibling to the address must be obtained from values discovered by
    /// the addition of more nodes to the tree
    Future,
}

#[must_use = "iterators are lazy and do nothing unless consumed"]
struct WitnessAddrsIter {
    root_level: Level,
    current: Address,
    ommer_count: u8,
}

impl Iterator for WitnessAddrsIter {
    type Item = (Address, Source);

    fn next(&mut self) -> Option<(Address, Source)> {
        if self.current.level() < self.root_level {
            let current = self.current;
            let source = if current.is_right_child() {
                Source::Past(self.ommer_count)
            } else {
                Source::Future
            };

            self.current = current.parent();
            if matches!(source, Source::Past(_)) {
                self.ommer_count += 1;
            }

            Some((current.sibling(), source))
        } else {
            None
        }
    }
}

/// A type representing the position of a leaf in a Merkle tree.
#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct Position(u64);

impl Position {
    /// Return whether the position refers to the right-hand child of a subtree with
    /// its root at level 1.
    pub fn is_right_child(&self) -> bool {
        self.0 & 0x1 == 1
    }

    /// Returns the minimum possible level of the root of a binary tree containing at least
    /// `self + 1` leaves.
    pub fn root_level(&self) -> Level {
        Level((u64::BITS - self.0.leading_zeros()) as u8)
    }

    /// Returns the number of cousins and/or ommers required to construct an authentication
    /// path to the root of a merkle tree that has `self + 1` leaves.
    pub fn past_ommer_count(&self) -> u8 {
        (0..self.root_level().0)
            .filter(|i| (self.0 >> i) & 0x1 == 1)
            .count()
            .try_into()
            .unwrap() // this is safe because we're counting within a `u8` range
    }

    /// Returns whether the binary tree having `self` as the position of the rightmost leaf
    /// contains a perfect balanced tree with a root at level `root_level` that contains the
    /// aforesaid leaf.
    pub fn is_complete_subtree(&self, root_level: Level) -> bool {
        !(0..(root_level.0)).any(|l| self.0 & (1 << l) == 0)
    }

    /// Returns an iterator over the addresses of nodes required to create a witness for this
    /// position, beginning with the sibling of the leaf at this position and ending with the
    /// sibling of the ancestor of the leaf at this position that is required to compute a root at
    /// the specified level.
    pub fn witness_addrs(&self, root_level: Level) -> impl Iterator<Item = (Address, Source)> {
        WitnessAddrsIter {
            root_level,
            current: Address::from(*self),
            ommer_count: 0,
        }
    }
}

impl From<Position> for u64 {
    fn from(p: Position) -> Self {
        p.0
    }
}

impl From<u64> for Position {
    fn from(sz: u64) -> Self {
        Self(sz)
    }
}

impl Add<u64> for Position {
    type Output = Position;
    fn add(self, other: u64) -> Self {
        Position(self.0 + other)
    }
}

impl AddAssign<u64> for Position {
    fn add_assign(&mut self, other: u64) {
        self.0 += other
    }
}

impl Sub<u64> for Position {
    type Output = Position;
    fn sub(self, other: u64) -> Self {
        if self.0 < other {
            panic!("position underflow");
        }
        Position(self.0 - other)
    }
}

impl TryFrom<usize> for Position {
    type Error = TryFromIntError;
    fn try_from(sz: usize) -> Result<Self, Self::Error> {
        <u64>::try_from(sz).map(Self)
    }
}

impl TryFrom<Position> for usize {
    type Error = TryFromIntError;
    fn try_from(p: Position) -> Result<Self, Self::Error> {
        <usize>::try_from(p.0)
    }
}

/// A type-safe wrapper for indexing into "levels" of a binary tree, such that
/// nodes at level `0` are leaves, nodes at level `1` are parents of nodes at
/// level `0`, and so forth. This type is capable of representing levels in
/// trees containing up to 2^255 leaves.
#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct Level(u8);

impl Level {
    pub const fn new(value: u8) -> Self {
        Self(value)
    }

    // TODO: replace with an instance for `Step<Level>` once `step_trait`
    // is stabilized
    pub fn iter_to(self, other: Level) -> impl Iterator<Item = Self> {
        (self.0..other.0).map(Level)
    }
}

impl Add<u8> for Level {
    type Output = Self;
    fn add(self, value: u8) -> Self {
        Self(self.0 + value)
    }
}

impl From<u8> for Level {
    fn from(value: u8) -> Self {
        Self(value)
    }
}

impl From<Level> for u8 {
    fn from(level: Level) -> u8 {
        level.0
    }
}

impl From<Level> for u32 {
    fn from(level: Level) -> u32 {
        level.0.into()
    }
}

impl From<Level> for u64 {
    fn from(level: Level) -> u64 {
        level.0.into()
    }
}

impl From<Level> for usize {
    fn from(level: Level) -> usize {
        // Supporting sub-8-bit platforms isn't on our roadmap.
        level.0 as usize
    }
}

impl TryFrom<usize> for Level {
    type Error = TryFromIntError;
    fn try_from(sz: usize) -> Result<Self, Self::Error> {
        <u8>::try_from(sz).map(Self)
    }
}

impl Sub<u8> for Level {
    type Output = Self;
    fn sub(self, value: u8) -> Self {
        if self.0 < value {
            panic!("underflow")
        }
        Self(self.0 - value)
    }
}

/// The address of a node of the Merkle tree.
/// When `level == 0`, the index has the same value as the
/// position.
#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Address {
    level: Level,
    index: u64,
}

impl Address {
    /// Construct a new address from its constituent parts.
    pub fn from_parts(level: Level, index: u64) -> Self {
        Address { level, index }
    }

    /// Returns the address at the given level that contains the specified leaf position.
    pub fn above_position(level: Level, position: Position) -> Self {
        Address {
            level,
            index: position.0 >> level.0,
        }
    }

    /// Returns the level of the root of the tree having its root at this address.
    pub fn level(&self) -> Level {
        self.level
    }

    /// Returns the index of the address.
    ///
    /// The index of an address is defined as the number of subtrees with their roots
    /// at the address's level that appear to the left of this address in a binary
    /// tree of arbitrary height > level * 2 + 1.
    pub fn index(&self) -> u64 {
        self.index
    }

    /// The address of the node one level higher than this in a binary tree that contains
    /// this address as either its left or right child.
    pub fn parent(&self) -> Address {
        Address {
            level: self.level + 1,
            index: self.index >> 1,
        }
    }

    /// Returns the address that shares the same parent as this address.
    pub fn sibling(&self) -> Address {
        Address {
            level: self.level,
            index: self.index ^ 1,
        }
    }

    /// Returns the immediate children of this address.
    pub fn children(&self) -> Option<(Address, Address)> {
        if self.level == Level::from(0) {
            None
        } else {
            let left = Address {
                level: self.level - 1,
                index: self.index << 1,
            };

            let right = Address {
                level: self.level - 1,
                index: (self.index << 1) + 1,
            };

            Some((left, right))
        }
    }

    /// Returns whether this address is an ancestor of the specified address.
    pub fn is_ancestor_of(&self, addr: &Self) -> bool {
        self.level > addr.level && { addr.index >> (self.level.0 - addr.level.0) == self.index }
    }

    /// Returns the common ancestor of `self` and `other` having the smallest level value.
    pub fn common_ancestor(&self, other: &Self) -> Self {
        // We can leverage the symmetry of a binary tree to share the calculation logic,
        // by ordering the nodes.
        let (higher, lower) = if self.level >= other.level {
            (self, other)
        } else {
            (other, self)
        };

        // We follow the lower node's subtree up to the same level as the higher node, and
        // then use their XOR distance to determine how many levels of the tree their
        // Merkle paths differ on.
        let lower_ancestor_idx = lower.index >> (higher.level.0 - lower.level.0);
        let index_delta = higher.index ^ lower_ancestor_idx;
        let level_delta = (u64::BITS - index_delta.leading_zeros()) as u8;
        Address {
            level: higher.level + level_delta,
            index: std::cmp::max(higher.index, lower_ancestor_idx) >> level_delta,
        }
    }

    /// Returns whether this address is an ancestor of, or is equal to,
    /// the specified address.
    pub fn contains(&self, addr: &Self) -> bool {
        self == addr || self.is_ancestor_of(addr)
    }

    /// Returns the minimum value among the range of leaf positions that are contained within the
    /// tree with its root at this address.
    pub fn position_range_start(&self) -> Position {
        (self.index << self.level.0).try_into().unwrap()
    }

    /// Returns the (exclusive) end of the range of leaf positions that are contained within the
    /// tree with its root at this address.
    pub fn position_range_end(&self) -> Position {
        ((self.index + 1) << self.level.0).try_into().unwrap()
    }

    /// Returns the maximum value among the range of leaf positions that are contained within the
    /// tree with its root at this address.
    pub fn max_position(&self) -> Position {
        self.position_range_end() - 1
    }

    /// Returns the end-exclusive range of leaf positions that are contained within the tree with
    /// its root at this address.
    pub fn position_range(&self) -> Range<Position> {
        Range {
            start: self.position_range_start(),
            end: self.position_range_end(),
        }
    }

    /// Returns either the ancestor of this address at the given level (if the level is greater
    /// than or equal to that of this address) or the range of indices of root addresses of
    /// subtrees with roots at the given level contained within the tree with its root at this
    /// address otherwise.
    pub fn context(&self, level: Level) -> Either<Address, Range<u64>> {
        if level >= self.level {
            Either::Left(Address {
                level,
                index: self.index >> (level.0 - self.level.0),
            })
        } else {
            let shift = self.level.0 - level.0;
            Either::Right(Range {
                start: self.index << shift,
                end: (self.index + 1) << shift,
            })
        }
    }

    /// Returns whether the tree with this root address contains the given leaf position, or if not
    /// whether an address at the same level with a greater or lesser index will contain the
    /// specified leaf position.
    pub fn position_cmp(&self, pos: Position) -> Ordering {
        let range = self.position_range();
        if range.start > pos {
            Ordering::Greater
        } else if range.end <= pos {
            Ordering::Less
        } else {
            Ordering::Equal
        }
    }

    /// Returns whether this address is the left-hand child of its parent
    pub fn is_left_child(&self) -> bool {
        self.index & 0x1 == 0
    }

    /// Returns whether this address is the right-hand child of its parent
    pub fn is_right_child(&self) -> bool {
        self.index & 0x1 == 1
    }

    pub fn current_incomplete(&self) -> Address {
        // find the first zero bit in the index, searching from the least significant bit
        let mut index = self.index;
        for level in self.level.0.. {
            if index & 0x1 == 1 {
                index >>= 1;
            } else {
                return Address {
                    level: Level(level),
                    index,
                };
            }
        }

        unreachable!("The loop will always terminate via return in at most 64 iterations.")
    }

    pub fn next_incomplete_parent(&self) -> Address {
        if self.is_right_child() {
            self.current_incomplete()
        } else {
            let complete = Address {
                level: self.level,
                index: self.index + 1,
            };
            complete.current_incomplete()
        }
    }

    /// Increments this address's index by 1 and returns the resulting address.
    pub fn next_at_level(&self) -> Address {
        Address {
            level: self.level,
            index: self.index + 1,
        }
    }
}

impl From<Position> for Address {
    fn from(p: Position) -> Self {
        Address {
            level: 0.into(),
            index: p.into(),
        }
    }
}

impl<'a> From<&'a Position> for Address {
    fn from(p: &'a Position) -> Self {
        Address {
            level: 0.into(),
            index: (*p).into(),
        }
    }
}

impl From<Address> for Option<Position> {
    fn from(addr: Address) -> Self {
        if addr.level == 0.into() {
            Some(addr.index.into())
        } else {
            None
        }
    }
}

impl<'a> From<&'a Address> for Option<Position> {
    fn from(addr: &'a Address) -> Self {
        if addr.level == 0.into() {
            Some(addr.index.into())
        } else {
            None
        }
    }
}

/// A path from a position in a particular commitment tree to the root of that tree.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct MerklePath<H, const DEPTH: u8> {
    path_elems: Vec<H>,
    position: Position,
}

impl<H, const DEPTH: u8> MerklePath<H, DEPTH> {
    /// Constructs a Merkle path directly from a path and position.
    #[allow(clippy::result_unit_err)]
    pub fn from_parts(path_elems: Vec<H>, position: Position) -> Result<Self, ()> {
        if path_elems.len() == usize::from(DEPTH) {
            Ok(MerklePath {
                path_elems,
                position,
            })
        } else {
            Err(())
        }
    }

    pub fn path_elems(&self) -> &[H] {
        &self.path_elems
    }

    pub fn position(&self) -> Position {
        self.position
    }
}

impl<H: Hashable, const DEPTH: u8> MerklePath<H, DEPTH> {
    /// Returns the root of the tree corresponding to this path applied to `leaf`.
    pub fn root(&self, leaf: H) -> H {
        self.path_elems
            .iter()
            .zip(0u8..)
            .fold(leaf, |root, (h, i)| {
                if (self.position.0 >> i) & 0x1 == 0 {
                    H::combine(i.into(), &root, h)
                } else {
                    H::combine(i.into(), h, &root)
                }
            })
    }
}

/// A trait describing the operations that make a type suitable for use as
/// a leaf or node value in a merkle tree.
pub trait Hashable: fmt::Debug {
    fn empty_leaf() -> Self;

    /// Combines two provided nodes that both exist at the specified level of the tree,
    /// producing a new node at level `level + 1`.
    fn combine(level: Level, a: &Self, b: &Self) -> Self;

    /// Produces an empty root at the specified level of the tree by combining empty leaf values.
    ///
    /// At each successive level, the value is produced by combining the value at the level below
    /// with a copy of itself.
    fn empty_root(level: Level) -> Self
    where
        Self: Sized,
    {
        Level::from(0)
            .iter_to(level)
            .fold(Self::empty_leaf(), |v, lvl| Self::combine(lvl, &v, &v))
    }
}

#[cfg(test)]
pub(crate) mod tests {
    use crate::MerklePath;

    use super::{Address, Level, Position, Source};
    use core::ops::Range;
    use either::Either;

    #[test]
    fn position_is_complete_subtree() {
        assert!(Position(0).is_complete_subtree(Level(0)));
        assert!(Position(1).is_complete_subtree(Level(1)));
        assert!(!Position(2).is_complete_subtree(Level(1)));
        assert!(!Position(2).is_complete_subtree(Level(2)));
        assert!(Position(3).is_complete_subtree(Level(2)));
        assert!(!Position(4).is_complete_subtree(Level(2)));
        assert!(Position(7).is_complete_subtree(Level(3)));
        assert!(Position(u32::MAX as u64).is_complete_subtree(Level(32)));
    }

    #[test]
    fn position_past_ommer_count() {
        assert_eq!(0, Position(0).past_ommer_count());
        assert_eq!(1, Position(1).past_ommer_count());
        assert_eq!(1, Position(2).past_ommer_count());
        assert_eq!(2, Position(3).past_ommer_count());
        assert_eq!(1, Position(4).past_ommer_count());
        assert_eq!(3, Position(7).past_ommer_count());
        assert_eq!(1, Position(8).past_ommer_count());
    }

    #[test]
    fn position_root_level() {
        assert_eq!(Level(0), Position(0).root_level());
        assert_eq!(Level(1), Position(1).root_level());
        assert_eq!(Level(2), Position(2).root_level());
        assert_eq!(Level(2), Position(3).root_level());
        assert_eq!(Level(3), Position(4).root_level());
        assert_eq!(Level(3), Position(7).root_level());
        assert_eq!(Level(4), Position(8).root_level());
    }

    #[test]
    fn position_witness_addrs() {
        use Source::*;
        let path_elem = |l, i, s| (Address::from_parts(Level::from(l), i), s);
        assert_eq!(
            vec![path_elem(0, 1, Future), path_elem(1, 1, Future)],
            Position::from(0)
                .witness_addrs(Level::from(2))
                .collect::<Vec<_>>()
        );
        assert_eq!(
            vec![path_elem(0, 3, Future), path_elem(1, 0, Past(0))],
            Position::from(2)
                .witness_addrs(Level::from(2))
                .collect::<Vec<_>>()
        );
        assert_eq!(
            vec![
                path_elem(0, 2, Past(0)),
                path_elem(1, 0, Past(1)),
                path_elem(2, 1, Future)
            ],
            Position::from(3)
                .witness_addrs(Level::from(3))
                .collect::<Vec<_>>()
        );
        assert_eq!(
            vec![
                path_elem(0, 5, Future),
                path_elem(1, 3, Future),
                path_elem(2, 0, Past(0)),
                path_elem(3, 1, Future)
            ],
            Position::from(4)
                .witness_addrs(Level::from(4))
                .collect::<Vec<_>>()
        );
        assert_eq!(
            vec![
                path_elem(0, 7, Future),
                path_elem(1, 2, Past(0)),
                path_elem(2, 0, Past(1)),
                path_elem(3, 1, Future)
            ],
            Position::from(6)
                .witness_addrs(Level::from(4))
                .collect::<Vec<_>>()
        );
    }

    #[test]
    fn address_current_incomplete() {
        let addr = |l, i| Address::from_parts(Level(l), i);
        assert_eq!(addr(0, 0), addr(0, 0).current_incomplete());
        assert_eq!(addr(1, 0), addr(0, 1).current_incomplete());
        assert_eq!(addr(0, 2), addr(0, 2).current_incomplete());
        assert_eq!(addr(2, 0), addr(0, 3).current_incomplete());
    }

    #[test]
    fn address_next_incomplete_parent() {
        let addr = |l, i| Address::from_parts(Level(l), i);
        assert_eq!(addr(1, 0), addr(0, 0).next_incomplete_parent());
        assert_eq!(addr(1, 0), addr(0, 1).next_incomplete_parent());
        assert_eq!(addr(2, 0), addr(0, 2).next_incomplete_parent());
        assert_eq!(addr(2, 0), addr(0, 3).next_incomplete_parent());
        assert_eq!(addr(3, 0), addr(2, 0).next_incomplete_parent());
        assert_eq!(addr(1, 2), addr(0, 4).next_incomplete_parent());
        assert_eq!(addr(3, 0), addr(1, 2).next_incomplete_parent());
    }

    #[test]
    fn addr_is_ancestor() {
        let l0 = Level(0);
        let l1 = Level(1);
        assert!(Address::from_parts(l1, 0).is_ancestor_of(&Address::from_parts(l0, 0)));
        assert!(Address::from_parts(l1, 0).is_ancestor_of(&Address::from_parts(l0, 1)));
        assert!(!Address::from_parts(l1, 0).is_ancestor_of(&Address::from_parts(l0, 2)));
    }

    #[test]
    fn addr_position_range() {
        assert_eq!(
            Address::from_parts(Level(0), 0).position_range(),
            Range {
                start: Position(0),
                end: Position(1)
            }
        );
        assert_eq!(
            Address::from_parts(Level(1), 0).position_range(),
            Range {
                start: Position(0),
                end: Position(2)
            }
        );
        assert_eq!(
            Address::from_parts(Level(2), 1).position_range(),
            Range {
                start: Position(4),
                end: Position(8)
            }
        );
    }

    #[test]
    fn addr_above_position() {
        assert_eq!(
            Address::above_position(Level(3), Position(9)),
            Address::from_parts(Level(3), 1)
        );
    }

    #[test]
    fn addr_children() {
        assert_eq!(Address::from_parts(Level(0), 1).children(), None);

        assert_eq!(
            Address::from_parts(Level(3), 1).children(),
            Some((
                Address::from_parts(Level(2), 2),
                Address::from_parts(Level(2), 3),
            ))
        );
    }

    #[test]
    fn addr_is_ancestor_of() {
        assert!(Address::from_parts(Level(3), 1).is_ancestor_of(&Address::from_parts(Level(2), 2)));
        assert!(Address::from_parts(Level(3), 1).is_ancestor_of(&Address::from_parts(Level(1), 7)));
        assert!(!Address::from_parts(Level(3), 1).is_ancestor_of(&Address::from_parts(Level(1), 8)));
    }

    #[test]
    fn addr_context() {
        assert_eq!(
            Address::from_parts(Level(3), 1).context(Level(0)),
            Either::Right(Range { start: 8, end: 16 })
        );

        assert_eq!(
            Address::from_parts(Level(3), 4).context(Level(5)),
            Either::Left(Address::from_parts(Level(5), 1))
        );
    }

    #[test]
    fn merkle_path_root() {
        let path: MerklePath<String, 3> = MerklePath::from_parts(
            vec!["a".to_string(), "cd".to_string(), "efgh".to_string()],
            Position(1),
        )
        .unwrap();

        assert_eq!(path.root("b".to_string()), "abcdefgh".to_string());

        let path: MerklePath<String, 3> = MerklePath::from_parts(
            vec!["d".to_string(), "ab".to_string(), "efgh".to_string()],
            Position(2),
        )
        .unwrap();

        assert_eq!(path.root("c".to_string()), "abcdefgh".to_string());
    }

    #[test]
    fn addr_common_ancestor() {
        //                                       rt
        //             ---------------                        ----------------
        //       -------             -------              right              -------
        //   -----     left      -----     -----     -----     -----     -----     -----
        //  --   --   --   --   --   --   --   --   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(2), 1).common_ancestor(&Address::from_parts(Level(3), 2)),
            Address::from_parts(Level(5), 0)
        );
        //                           --------------------------
        //             ---------------                        ----------------
        //       -------               rt                -------             -------
        //   -----     -----     left      -----     -----     -----     -----     -----
        //  --   --   --   --   --   --   --   rg   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(2), 2).common_ancestor(&Address::from_parts(Level(1), 7)),
            Address::from_parts(Level(3), 1)
        );
        //                           --------------------------
        //             ---------------                        ----------------
        //       -------               rt                -------             -------
        //   -----     -----     left      -----     -----     -----     -----     -----
        //  --   --   --   --   --   --   rg   --   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(2), 2).common_ancestor(&Address::from_parts(Level(1), 6)),
            Address::from_parts(Level(3), 1)
        );
        //                           --------------------------
        //             ---------------                        ----------------
        //       -------             -------             -------             -------
        //   -----     -----      all      -----     -----     -----     -----     -----
        //  --   --   --   --   --   --   --   --   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(2), 2).common_ancestor(&Address::from_parts(Level(2), 2)),
            Address::from_parts(Level(2), 2)
        );
        //                           --------------------------
        //             ---------------                        ----------------
        //       -------             -------             -------             -------
        //   -----     -----     lf,rt     -----     -----     -----     -----     -----
        //  --   --   --   --   --   --   --   --   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - - rg -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(2), 2).common_ancestor(&Address::from_parts(Level(0), 9)),
            Address::from_parts(Level(2), 2)
        );
        //                           --------------------------
        //             ---------------                        ----------------
        //       -------             -------             -------             -------
        //   -----     -----     rg,rt     -----     -----     -----     -----     -----
        //  --   --   --   --   --   --   --   --   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - - lf -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(0), 9).common_ancestor(&Address::from_parts(Level(2), 2)),
            Address::from_parts(Level(2), 2)
        );
        //                           --------------------------
        //             ---------------                        ----------------
        //       -------             -------             -------             -------
        //   -----     -----     -----       rt      -----     -----     -----     -----
        //  --   --   --   --   --   --   --   --   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - -  - -  - lf - - rg -  - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(0), 12).common_ancestor(&Address::from_parts(Level(0), 15)),
            Address::from_parts(Level(2), 3)
        );
        //                           --------------------------
        //             ---------------                        ----------------
        //       -------             -------             -------             -------
        //   -----     -----     -----       rt      -----     -----     -----     -----
        //  --   --   --   --   --   --   --   --   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - -  - -  - - lf - rg -  - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(0), 13).common_ancestor(&Address::from_parts(Level(0), 15)),
            Address::from_parts(Level(2), 3)
        );
        //                           --------------------------
        //             ---------------                        ----------------
        //       -------             -------             -------             -------
        //   -----     -----     -----       rt      -----     -----     -----     -----
        //  --   --   --   --   --   --   --   --   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - -  - -  - - lf rg - -  - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(0), 13).common_ancestor(&Address::from_parts(Level(0), 14)),
            Address::from_parts(Level(2), 3)
        );
        //                           --------------------------
        //             ---------------                        ----------------
        //       -------             -------             -------             -------
        //   -----     -----     -----     -----     -----     -----     -----     -----
        //  --   --   --   --   --   --   --   rt   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - -  - -  - - - lf  rg - - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(0), 14).common_ancestor(&Address::from_parts(Level(0), 15)),
            Address::from_parts(Level(1), 7)
        );
        //                                       rt
        //             ---------------                        ----------------
        //       -------             -------             -------             -------
        //   -----     -----     -----     -----     -----     -----     -----     -----
        //  --   --   --   --   --   --   --   --   --   --   --   --   --   --   --   --
        // -  - -  - -  - -  - -  - -  - -  - - lf rg - -  - -  - -  - -  - -  - -  - -  -
        assert_eq!(
            Address::from_parts(Level(0), 15).common_ancestor(&Address::from_parts(Level(0), 16)),
            Address::from_parts(Level(5), 0)
        );
    }
}
