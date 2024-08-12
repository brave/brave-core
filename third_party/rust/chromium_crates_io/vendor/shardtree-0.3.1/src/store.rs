//! Traits and structs for storing [`ShardTree`]s.
//!
//! [`ShardTree`]: crate::ShardTree
//!
//! # Structure
//!
//! The tree is represented as an ordered collection of fixed-depth subtrees, or "shards".
//! Each shard is a [`LocatedPrunableTree`]. The roots of the shards form the leaves in
//! the "cap", which is a [`PrunableTree`].
//!
//! ```text
//! Level
//!   3           root         \
//!               / \           |
//!             /     \         |
//!   2       /         \        } cap
//!         / \         / \     |
//!        /   \       /   \    |
//!   1   A     B     C     D  /  \
//!      / \   / \   / \   / \     } shards
//!   0 /\ /\ /\ /\ /\ /\ /\ /\   /
//! ```

use std::collections::BTreeSet;

use incrementalmerkletree::{Address, Position};

use crate::{LocatedPrunableTree, PrunableTree};

pub mod caching;
pub mod memory;

/// A capability for storage of fragment subtrees of the [`ShardTree`] type.
///
/// All fragment subtrees must have roots at level `SHARD_HEIGHT`.
///
/// [`ShardTree`]: crate::ShardTree
pub trait ShardStore {
    /// The type used for leaves and nodes in the tree.
    type H;

    /// The type used to identify checkpointed positions in the tree.
    type CheckpointId;

    /// The error type for operations on this store.
    type Error: std::error::Error;

    /// Returns the subtree at the given root address, if any such subtree exists.
    fn get_shard(
        &self,
        shard_root: Address,
    ) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error>;

    /// Returns the subtree containing the maximum inserted leaf position.
    fn last_shard(&self) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error>;

    /// Inserts or replaces the subtree having the same root address as the provided tree.
    ///
    /// Implementations of this method MUST enforce the constraint that the root address
    /// of the provided subtree has level `SHARD_HEIGHT`.
    fn put_shard(&mut self, subtree: LocatedPrunableTree<Self::H>) -> Result<(), Self::Error>;

    /// Returns the vector of addresses corresponding to the roots of subtrees stored in this
    /// store.
    fn get_shard_roots(&self) -> Result<Vec<Address>, Self::Error>;

    /// Removes subtrees from the underlying store having root addresses at indices greater
    /// than or equal to that of the specified address.
    ///
    /// Implementations of this method MUST enforce the constraint that the root address
    /// provided has level `SHARD_HEIGHT`.
    fn truncate(&mut self, from: Address) -> Result<(), Self::Error>;

    /// A tree that is used to cache the known roots of subtrees in the "cap" - the top part of the
    /// tree, which contains parent nodes produced by hashing the roots of the individual shards.
    /// Nodes in the cap have levels in the range `SHARD_HEIGHT..DEPTH`. Note that the cap may be
    /// sparse, in the same way that individual shards may be sparse.
    fn get_cap(&self) -> Result<PrunableTree<Self::H>, Self::Error>;

    /// Persists the provided cap to the data store.
    fn put_cap(&mut self, cap: PrunableTree<Self::H>) -> Result<(), Self::Error>;

    /// Returns the identifier for the checkpoint with the lowest associated position value.
    fn min_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error>;

    /// Returns the identifier for the checkpoint with the highest associated position value.
    fn max_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error>;

    /// Adds a checkpoint to the data store.
    fn add_checkpoint(
        &mut self,
        checkpoint_id: Self::CheckpointId,
        checkpoint: Checkpoint,
    ) -> Result<(), Self::Error>;

    /// Returns the number of checkpoints maintained by the data store
    fn checkpoint_count(&self) -> Result<usize, Self::Error>;

    /// Returns the id and position of the checkpoint, if any. Returns `None` if
    /// `checkpoint_depth == 0` or if insufficient checkpoints exist to seek back
    /// to the requested depth.
    fn get_checkpoint_at_depth(
        &self,
        checkpoint_depth: usize,
    ) -> Result<Option<(Self::CheckpointId, Checkpoint)>, Self::Error>;

    /// Returns the checkpoint corresponding to the specified checkpoint identifier.
    fn get_checkpoint(
        &self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<Option<Checkpoint>, Self::Error>;

    /// Iterates in checkpoint ID order over the first `limit` checkpoints, applying the
    /// given callback to each.
    fn with_checkpoints<F>(&mut self, limit: usize, callback: F) -> Result<(), Self::Error>
    where
        F: FnMut(&Self::CheckpointId, &Checkpoint) -> Result<(), Self::Error>;

    /// Update the checkpoint having the given identifier by mutating it with the provided
    /// function, and persist the updated checkpoint to the data store.
    ///
    /// Returns `Ok(true)` if the checkpoint was found, `Ok(false)` if no checkpoint with the
    /// provided identifier exists in the data store, or an error if a storage error occurred.
    fn update_checkpoint_with<F>(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
        update: F,
    ) -> Result<bool, Self::Error>
    where
        F: Fn(&mut Checkpoint) -> Result<(), Self::Error>;

    /// Removes a checkpoint from the data store.
    ///
    /// If no checkpoint exists with the given ID, this does nothing.
    fn remove_checkpoint(&mut self, checkpoint_id: &Self::CheckpointId) -> Result<(), Self::Error>;

    /// Removes checkpoints with identifiers greater than or equal to the given identifier.
    fn truncate_checkpoints(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<(), Self::Error>;
}

impl<S: ShardStore> ShardStore for &mut S {
    type H = S::H;
    type CheckpointId = S::CheckpointId;
    type Error = S::Error;

    fn get_shard(
        &self,
        shard_root: Address,
    ) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        S::get_shard(*self, shard_root)
    }

    fn last_shard(&self) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        S::last_shard(*self)
    }

    fn put_shard(&mut self, subtree: LocatedPrunableTree<Self::H>) -> Result<(), Self::Error> {
        S::put_shard(*self, subtree)
    }

    fn get_shard_roots(&self) -> Result<Vec<Address>, Self::Error> {
        S::get_shard_roots(*self)
    }

    fn get_cap(&self) -> Result<PrunableTree<Self::H>, Self::Error> {
        S::get_cap(*self)
    }

    fn put_cap(&mut self, cap: PrunableTree<Self::H>) -> Result<(), Self::Error> {
        S::put_cap(*self, cap)
    }

    fn truncate(&mut self, from: Address) -> Result<(), Self::Error> {
        S::truncate(*self, from)
    }

    fn min_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        S::min_checkpoint_id(self)
    }

    fn max_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        S::max_checkpoint_id(self)
    }

    fn add_checkpoint(
        &mut self,
        checkpoint_id: Self::CheckpointId,
        checkpoint: Checkpoint,
    ) -> Result<(), Self::Error> {
        S::add_checkpoint(self, checkpoint_id, checkpoint)
    }

    fn checkpoint_count(&self) -> Result<usize, Self::Error> {
        S::checkpoint_count(self)
    }

    fn get_checkpoint_at_depth(
        &self,
        checkpoint_depth: usize,
    ) -> Result<Option<(Self::CheckpointId, Checkpoint)>, Self::Error> {
        S::get_checkpoint_at_depth(self, checkpoint_depth)
    }

    fn get_checkpoint(
        &self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<Option<Checkpoint>, Self::Error> {
        S::get_checkpoint(self, checkpoint_id)
    }

    fn with_checkpoints<F>(&mut self, limit: usize, callback: F) -> Result<(), Self::Error>
    where
        F: FnMut(&Self::CheckpointId, &Checkpoint) -> Result<(), Self::Error>,
    {
        S::with_checkpoints(self, limit, callback)
    }

    fn update_checkpoint_with<F>(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
        update: F,
    ) -> Result<bool, Self::Error>
    where
        F: Fn(&mut Checkpoint) -> Result<(), Self::Error>,
    {
        S::update_checkpoint_with(self, checkpoint_id, update)
    }

    fn remove_checkpoint(&mut self, checkpoint_id: &Self::CheckpointId) -> Result<(), Self::Error> {
        S::remove_checkpoint(self, checkpoint_id)
    }

    fn truncate_checkpoints(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        S::truncate_checkpoints(self, checkpoint_id)
    }
}

/// An enumeration of possible checkpoint locations.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum TreeState {
    /// Checkpoints of the empty tree.
    Empty,
    /// Checkpoint at a (possibly pruned) leaf state corresponding to the
    /// wrapped leaf position.
    AtPosition(Position),
}

/// The information required to save the state of a [`ShardTree`] at some [`Position`].
///
/// [`ShardTree`]: crate::ShardTree
#[derive(Clone, Debug)]
pub struct Checkpoint {
    tree_state: TreeState,
    marks_removed: BTreeSet<Position>,
}

impl Checkpoint {
    pub fn tree_empty() -> Self {
        Checkpoint {
            tree_state: TreeState::Empty,
            marks_removed: BTreeSet::new(),
        }
    }

    pub fn at_position(position: Position) -> Self {
        Checkpoint {
            tree_state: TreeState::AtPosition(position),
            marks_removed: BTreeSet::new(),
        }
    }

    pub fn from_parts(tree_state: TreeState, marks_removed: BTreeSet<Position>) -> Self {
        Checkpoint {
            tree_state,
            marks_removed,
        }
    }

    pub fn tree_state(&self) -> TreeState {
        self.tree_state
    }

    pub fn marks_removed(&self) -> &BTreeSet<Position> {
        &self.marks_removed
    }

    pub fn is_tree_empty(&self) -> bool {
        matches!(self.tree_state, TreeState::Empty)
    }

    pub fn position(&self) -> Option<Position> {
        match self.tree_state {
            TreeState::Empty => None,
            TreeState::AtPosition(pos) => Some(pos),
        }
    }

    pub(crate) fn mark_removed(&mut self, position: Position) {
        self.marks_removed.insert(position);
    }
}
