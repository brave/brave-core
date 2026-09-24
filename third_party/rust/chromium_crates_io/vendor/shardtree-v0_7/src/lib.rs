//! `shardtree` is a space-efficient fixed-depth Merkle tree structure that is
//! densely filled from the left. It supports:
//!
//! - *Out-of-order insertion*: leaves and nodes may be inserted into the tree
//!   in arbitrary order. The structure will keep track of the right-most filled
//!   position as the frontier of the tree; any unfilled leaves to the left of
//!   this position are considered "missing", while any unfilled leaves to the
//!   right of this position are considered "empty".
//! - *Witnessing*: Individual leaves of the Merkle tree may be marked such that
//!   witnesses will be maintained for the marked leaves as additional nodes are
//!   inserted into the tree, but leaf and node data not specifically required
//!   to maintain these witnesses is not retained, for space efficiency.
//! - *Checkpointing*: the tree may be reset to a previously checkpointed state,
//!   up to a fixed number of checkpoints.
//!
//! # Structure: shards and the cap
//!
//! A tree of depth `DEPTH` is split horizontally at level `SHARD_HEIGHT` into
//! two parts:
//!
//! - **Shards**: the fixed-height subtrees rooted at level `SHARD_HEIGHT`,
//!   covering levels `0..SHARD_HEIGHT`. They hold the bulk of the data and are
//!   the unit of persistence. Each shard is a [`LocatedPrunableTree`].
//! - **Cap**: the upper part, levels `SHARD_HEIGHT..=DEPTH`. Its leaves are the
//!   shard roots, and its internal nodes hash those roots up to the tree root.
//!   The cap is a single [`PrunableTree`].
//!
//! Both parts are [`PrunableTree`]s: every node may carry a cached hash
//! annotation, and leaves carry [`RetentionFlags`] that decide whether they may
//! be pruned. Either part may be sparse (unknown subtrees are [`Node::Nil`]).
//!
//! This layout is what lets witnesses advance cheaply: rather than re-inserting
//! every intermediate leaf, only the roots of the complete shards between the
//! marked leaf and the frontier are inserted, plus the path from the marked
//! leaf to its shard root. The shards carry the persisted state; the cap is a
//! comparatively small, possibly-sparse cache of the upper node hashes, filled
//! when subtree roots are inserted or when a `*_caching` query (e.g.
//! [`ShardTree::root_caching`]) writes computed hashes back. See the [`store`]
//! module for the storage model and a diagram.
//!
//! # Key types
//!
//! - [`ShardTree`]: the top-level tree and main entry point.
//! - [`store::ShardStore`]: the persistence trait.
//!   [`store::memory::MemoryShardStore`] keeps everything in memory;
//!   [`store::caching::CachingShardStore`] wraps a backend with a write-back
//!   cache.
//! - [`Tree`] / [`LocatedTree`]: the core node structure, and the same paired
//!   with its absolute [`Address`](incrementalmerkletree::Address).
//! - [`PrunableTree`] / [`LocatedPrunableTree`]: a [`Tree`] specialized for
//!   Merkle storage, with cached-hash annotations and prunable leaves.
//! - [`store::Checkpoint`]: a saved position the tree can be rewound to.

#![cfg_attr(docsrs, feature(doc_cfg))]

use core::fmt::Debug;
use either::Either;
use incrementalmerkletree::frontier::Frontier;
use incrementalmerkletree::Marking;
use std::collections::{BTreeMap, BTreeSet};
use std::sync::Arc;
use tracing::{debug, trace};

use incrementalmerkletree::{
    frontier::NonEmptyFrontier, Address, Hashable, Level, MerklePath, Position, Retention,
};

use self::{
    error::{InsertionError, QueryError, ShardTreeError},
    store::{Checkpoint, ShardStore, TreeState},
};

mod batch;
pub use self::batch::BatchInsertionResult;

mod tree;
pub use self::tree::{LocatedTree, Node, Tree};

mod prunable;
pub use self::prunable::{IncompleteAt, LocatedPrunableTree, PrunableTree, RetentionFlags};

pub mod error;
pub mod store;

#[cfg(any(test, feature = "test-dependencies"))]
pub mod testing;

#[cfg(feature = "legacy-api")]
#[cfg_attr(docsrs, doc(cfg(feature = "legacy-api")))]
mod legacy;

/// A wrapper around a node hash `H` that treats the cap of a [`ShardTree`] as a
/// standalone tree of `DEPTH - SHARD_HEIGHT` levels.
///
/// The cap is the upper part of a shard tree, spanning levels
/// `SHARD_HEIGHT..=DEPTH`; its leaves are the shard roots, which live at level
/// `SHARD_HEIGHT` of the full tree. `LevelShifter` re-bases those levels so that
/// a shard root (full-tree level `SHARD_HEIGHT`) becomes a level-`0` leaf of the
/// cap. Its [`Hashable`] implementation shifts every level argument up by
/// `SHARD_HEIGHT` before delegating to the wrapped `H`, so combining and
/// computing empty roots at cap-relative levels produces exactly the same hashes
/// as operating at the corresponding full-tree levels.
///
/// This lets the existing tree-construction machinery (which works in terms of
/// level-`0` leaves) be reused to batch-insert shard roots into the cap, without
/// the caller having to translate levels at every step.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct LevelShifter<H, const SHARD_HEIGHT: u8>(pub H);

impl<H: Hashable, const SHARD_HEIGHT: u8> Hashable for LevelShifter<H, SHARD_HEIGHT> {
    fn empty_leaf() -> Self {
        Self(H::empty_root(SHARD_HEIGHT.into()))
    }

    fn combine(level: Level, a: &Self, b: &Self) -> Self {
        Self(H::combine(level + SHARD_HEIGHT, &a.0, &b.0))
    }

    fn empty_root(level: Level) -> Self
    where
        Self: Sized,
    {
        Self(H::empty_root(level + SHARD_HEIGHT))
    }
}

/// A sparse binary Merkle tree of the specified depth, represented as an ordered collection of
/// subtrees (shards) of a given maximum height.
///
/// This tree maintains a collection of "checkpoints" which represent positions, usually near the
/// front of the tree, that are maintained such that it's possible to truncate nodes to the right
/// of the specified position.
#[derive(Debug)]
pub struct ShardTree<S, const DEPTH: u8, const SHARD_HEIGHT: u8>
where
    S: ShardStore,
{
    /// The vector of tree shards.
    store: S,
    /// The maximum number of checkpoints to retain before pruning.
    max_checkpoints: usize,
}

impl<
        H: Hashable + Clone + PartialEq,
        C: Clone + Debug + Ord,
        S: ShardStore<H = H, CheckpointId = C>,
        const DEPTH: u8,
        const SHARD_HEIGHT: u8,
    > ShardTree<S, DEPTH, SHARD_HEIGHT>
{
    /// Creates a new empty tree.
    pub fn new(store: S, max_checkpoints: usize) -> Self {
        Self {
            store,
            max_checkpoints,
        }
    }

    /// Consumes this tree and returns its underlying [`ShardStore`].
    pub fn into_store(self) -> S {
        self.store
    }

    /// Returns a reference to the underlying [`ShardStore`].
    pub fn store(&self) -> &S {
        &self.store
    }

    /// Returns a mutable reference to the underlying [`ShardStore`].
    pub fn store_mut(&mut self) -> &mut S {
        &mut self.store
    }

    /// Returns the maximum number of checkpoints to retain before pruning.
    pub fn max_checkpoints(&self) -> usize {
        self.max_checkpoints
    }

    /// Returns the root address of the tree.
    pub fn root_addr() -> Address {
        Address::from_parts(Level::from(DEPTH), 0)
    }

    /// Returns the fixed level of subtree roots within the vector of subtrees used as this tree's
    /// representation.
    pub fn subtree_level() -> Level {
        Level::from(SHARD_HEIGHT)
    }

    /// Returns the root address of the subtree that contains the specified position.
    pub fn subtree_addr(pos: Position) -> Address {
        Address::above_position(Self::subtree_level(), pos)
    }

    fn max_subtree_index() -> u64 {
        (0x1 << (DEPTH - SHARD_HEIGHT)) - 1
    }

    /// Returns the leaf value at the specified position, if it is a marked leaf.
    pub fn get_marked_leaf(
        &self,
        position: Position,
    ) -> Result<Option<H>, ShardTreeError<S::Error>> {
        Ok(self
            .store
            .get_shard(Self::subtree_addr(position))
            .map_err(ShardTreeError::Storage)?
            .and_then(|t| t.value_at_position(position).cloned())
            .and_then(|(v, r)| if r.is_marked() { Some(v) } else { None }))
    }

    /// Returns the positions of marked leaves in the tree.
    pub fn marked_positions(&self) -> Result<BTreeSet<Position>, ShardTreeError<S::Error>> {
        let mut result = BTreeSet::new();
        for subtree_addr in &self
            .store
            .get_shard_roots()
            .map_err(ShardTreeError::Storage)?
        {
            if let Some(subtree) = self
                .store
                .get_shard(*subtree_addr)
                .map_err(ShardTreeError::Storage)?
            {
                result.append(&mut subtree.marked_positions());
            }
        }
        Ok(result)
    }

    /// Returns the frontier of the tree.
    ///
    /// The frontier consists of the rightmost leaf and the ommers (sibling hashes)
    /// needed to compute the root. This method assembles ommers from within the
    /// last shard (below `SHARD_HEIGHT`) and from sibling subtree roots above the
    /// shard level.
    ///
    /// Returns [`Frontier::empty`] if the tree has no shards or if the frontier
    /// cannot be determined from the available data.
    pub fn frontier(&self) -> Result<Frontier<H, DEPTH>, ShardTreeError<S::Error>> {
        let last_shard = self.store.last_shard().map_err(ShardTreeError::Storage)?;
        let last_shard = match last_shard {
            Some(s) => s,
            None => return Ok(Frontier::empty()),
        };

        let (position, leaf, mut ommers) =
            match last_shard.frontier_ommers().map_err(|e| match e {
                prunable::FrontierError::TreeIncomplete { address } => {
                    ShardTreeError::Query(QueryError::TreeIncomplete(vec![address]))
                }
                _ => {
                    // FIXME: We need a breaking release to add a more correct error type
                    // for this case; at that time, we should also make `ShardTreeError`
                    // non-exhaustive.
                    let shard_addr = last_shard.root_addr();
                    ShardTreeError::Query(QueryError::TreeIncomplete(vec![shard_addr]))
                }
            })? {
                Some(parts) => parts,
                None => return Ok(Frontier::empty()),
            };

        // Walk from the shard root address up to the tree root, collecting ommers
        // from sibling subtrees above the shard level.
        let mut cur_addr = last_shard.root_addr;
        while cur_addr.level() < Level::from(DEPTH) {
            if cur_addr.is_right_child() {
                let sibling_hash = self.root(cur_addr.sibling(), position + 1)?;
                ommers.push(sibling_hash);
            }
            cur_addr = cur_addr.parent();
        }

        // FIXME: this should also have a better error variant.
        Frontier::from_parts(position, leaf, ommers)
            .map_err(|_| ShardTreeError::Query(QueryError::TreeIncomplete(vec![Self::root_addr()])))
    }

    /// Inserts a new root into the tree at the given address.
    ///
    /// The level associated with the given address may not exceed `DEPTH`.
    /// This will return an error if the specified hash conflicts with any existing annotation.
    pub fn insert(&mut self, root_addr: Address, value: H) -> Result<(), ShardTreeError<S::Error>> {
        if root_addr.level() > Self::root_addr().level() {
            return Err(ShardTreeError::Insert(InsertionError::NotContained(
                root_addr,
            )));
        }

        let to_insert = LocatedTree {
            root_addr,
            root: Tree::leaf((value, RetentionFlags::EPHEMERAL)),
        };

        // The cap will retain nodes at the level of the shard roots or higher.
        if root_addr.level() >= Self::subtree_level() {
            let cap = LocatedTree {
                root: self.store.get_cap().map_err(ShardTreeError::Storage)?,
                root_addr: Self::root_addr(),
            };

            cap.insert_subtree(to_insert.clone(), false)
                .map_err(ShardTreeError::Insert)
                .and_then(|(updated_cap, _)| {
                    self.store
                        .put_cap(updated_cap.root)
                        .map_err(ShardTreeError::Storage)
                })?;
        }

        if let Either::Left(shard_root_addr) = root_addr.context(Self::subtree_level()) {
            let shard = self
                .store
                .get_shard(shard_root_addr)
                .map_err(ShardTreeError::Storage)?
                .unwrap_or_else(|| LocatedTree {
                    root_addr: shard_root_addr,
                    root: Tree::empty(),
                });

            let updated_shard = shard
                .insert_subtree(to_insert, false)
                .map_err(ShardTreeError::Insert)
                .map(|(t, _)| t)?;

            self.store
                .put_shard(updated_shard)
                .map_err(ShardTreeError::Storage)?;
        }

        Ok(())
    }

    /// Append a single value at the first unfilled position greater than the maximum position of
    /// any previously inserted leaf.
    ///
    /// Prefer to use [`Self::batch_insert`] when appending multiple values, as these operations
    /// require fewer traversals of the tree than are necessary when performing multiple sequential
    /// calls to [`Self::append`].
    pub fn append(
        &mut self,
        value: H,
        retention: Retention<C>,
    ) -> Result<(), ShardTreeError<S::Error>> {
        if let Retention::Checkpoint { id, .. } = &retention {
            if self
                .store
                .max_checkpoint_id()
                .map_err(ShardTreeError::Storage)?
                .as_ref()
                >= Some(id)
            {
                return Err(InsertionError::CheckpointOutOfOrder.into());
            }
        }

        let (append_result, position, checkpoint_id) =
            if let Some(subtree) = self.store.last_shard().map_err(ShardTreeError::Storage)? {
                if subtree.root().is_full() {
                    // If the shard is full, then construct a successor tree.
                    let addr = subtree.root_addr;
                    if subtree.root_addr.index() < Self::max_subtree_index() {
                        LocatedTree::empty(addr.next_at_level()).append(value, retention)?
                    } else {
                        return Err(InsertionError::TreeFull.into());
                    }
                } else {
                    // Otherwise, just append to the shard.
                    subtree.append(value, retention)?
                }
            } else {
                let root_addr = Address::from_parts(Self::subtree_level(), 0);
                LocatedTree::empty(root_addr).append(value, retention)?
            };

        self.store
            .put_shard(append_result)
            .map_err(ShardTreeError::Storage)?;
        if let Some(c) = checkpoint_id {
            self.store
                .add_checkpoint(c, Checkpoint::at_position(position))
                .map_err(ShardTreeError::Storage)?;
        }

        self.prune_excess_checkpoints()?;

        Ok(())
    }

    /// Add the leaf and ommers of the provided frontier to the tree.
    ///
    /// The leaf and ommers will be added as nodes within the subtree corresponding to the
    /// frontier's position, and the tree state corresponding to that frontier will be marked as
    /// specified by the leaf retention.
    ///
    /// This method may be used to add a checkpoint for the empty tree; note that
    /// [`Retention::Marked`] is invalid for the empty tree.
    #[tracing::instrument(skip(self, frontier, leaf_retention))]
    pub fn insert_frontier(
        &mut self,
        frontier: Frontier<H, DEPTH>,
        leaf_retention: Retention<C>,
    ) -> Result<(), ShardTreeError<S::Error>> {
        if let Some(nonempty_frontier) = frontier.take() {
            self.insert_frontier_nodes(nonempty_frontier, leaf_retention)
        } else {
            match leaf_retention {
                Retention::Ephemeral | Retention::Reference => Ok(()),
                Retention::Checkpoint {
                    id,
                    marking: Marking::None | Marking::Reference,
                } => self
                    .store
                    .add_checkpoint(id, Checkpoint::tree_empty())
                    .map_err(ShardTreeError::Storage),
                Retention::Marked
                | Retention::Checkpoint {
                    marking: Marking::Marked,
                    ..
                } => Err(ShardTreeError::Insert(
                    InsertionError::MarkedRetentionInvalid,
                )),
            }
        }
    }

    /// Add the leaf and ommers of the provided frontier as nodes within the subtree corresponding
    /// to the frontier's position, and update the cap to include the ommer nodes at levels greater
    /// than or equal to the shard height.
    #[tracing::instrument(skip(self, frontier, leaf_retention))]
    pub fn insert_frontier_nodes(
        &mut self,
        frontier: NonEmptyFrontier<H>,
        leaf_retention: Retention<C>,
    ) -> Result<(), ShardTreeError<S::Error>> {
        let leaf_position = frontier.position();
        let subtree_root_addr = Address::above_position(Self::subtree_level(), leaf_position);

        let current_shard = self
            .store
            .get_shard(subtree_root_addr)
            .map_err(ShardTreeError::Storage)?
            .unwrap_or_else(|| LocatedTree::empty(subtree_root_addr));

        let (updated_subtree, supertree) =
            current_shard.insert_frontier_nodes(frontier, &leaf_retention)?;
        self.store
            .put_shard(updated_subtree)
            .map_err(ShardTreeError::Storage)?;

        if let Some(supertree) = supertree {
            let new_cap = LocatedTree {
                root_addr: Self::root_addr(),
                root: self.store.get_cap().map_err(ShardTreeError::Storage)?,
            }
            .insert_subtree(supertree, leaf_retention.is_marked())?;

            self.store
                .put_cap(new_cap.0.root)
                .map_err(ShardTreeError::Storage)?;
        }

        if let Retention::Checkpoint { id, .. } = leaf_retention {
            trace!("Adding checkpoint {:?} at {:?}", id, leaf_position);
            self.store
                .add_checkpoint(id, Checkpoint::at_position(leaf_position))
                .map_err(ShardTreeError::Storage)?;
        }

        self.prune_excess_checkpoints()?;
        Ok(())
    }

    /// Insert a tree by decomposing it into its `SHARD_HEIGHT` or smaller parts (if necessary)
    /// and inserting those at their appropriate locations.
    #[tracing::instrument(skip(self, tree, checkpoints))]
    pub fn insert_tree(
        &mut self,
        tree: LocatedPrunableTree<H>,
        checkpoints: BTreeMap<C, Position>,
    ) -> Result<Vec<IncompleteAt>, ShardTreeError<S::Error>> {
        let mut all_incomplete = vec![];
        for subtree in tree.decompose_to_level(Self::subtree_level()).into_iter() {
            // `ShardTree::max_leaf_position` relies on the invariant that the last shard
            // in the subtrees vector is never created without a leaf then being added to
            // it. `LocatedTree::decompose_to_level` can return a trailing empty subtree
            // for some inputs, and given that it is always correct to not insert an empty
            // subtree into `self`, we maintain the invariant by skipping empty subtrees.
            if subtree.root().is_empty() {
                debug!("Subtree with root {:?} is empty.", subtree.root_addr);
                continue;
            }

            // `LocatedTree::decompose_to_level` will return the tree as-is if it is
            // smaller than a shard, so we can't assume that the address of `subtree` is a
            // valid shard address.
            let root_addr = Self::subtree_addr(subtree.root_addr.position_range_start());

            let contains_marked = subtree.root.contains_marked();
            let current_shard = self
                .store
                .get_shard(root_addr)
                .map_err(ShardTreeError::Storage)?
                .unwrap_or_else(|| LocatedTree::empty(root_addr));
            let (replacement_shard, mut incomplete) =
                current_shard.insert_subtree(subtree, contains_marked)?;
            self.store
                .put_shard(replacement_shard)
                .map_err(ShardTreeError::Storage)?;
            all_incomplete.append(&mut incomplete);
        }

        for (id, position) in checkpoints.into_iter() {
            self.store
                .add_checkpoint(id, Checkpoint::at_position(position))
                .map_err(ShardTreeError::Storage)?;
        }
        self.prune_excess_checkpoints()?;

        Ok(all_incomplete)
    }

    /// Adds a checkpoint at the rightmost leaf state of the tree.
    ///
    /// # Panics
    ///
    /// Panics if `root` represents a parent node but `root_addr` is a depth-0 leaf address.
    pub fn checkpoint(&mut self, checkpoint_id: C) -> Result<bool, ShardTreeError<S::Error>> {
        /// Pre-condition: `root_addr` must be the address of `root`.
        fn go<H>(root_addr: Address, root: &PrunableTree<H>) -> Option<(PrunableTree<H>, Position)>
        where
            H: Hashable + Clone + PartialEq,
        {
            match &root.0 {
                Node::Parent { ann, left, right } => {
                    let (l_addr, r_addr) = root_addr
                        .children()
                        .expect("has children because we checked `root` is a parent");
                    go(r_addr, right).map_or_else(
                        || {
                            go(l_addr, left).map(|(new_left, pos)| {
                                (
                                    Tree::unite(
                                        l_addr.level(),
                                        ann.clone(),
                                        new_left,
                                        Tree::empty(),
                                    ),
                                    pos,
                                )
                            })
                        },
                        |(new_right, pos)| {
                            Some((
                                Tree::unite(
                                    l_addr.level(),
                                    ann.clone(),
                                    left.as_ref().clone(),
                                    new_right,
                                ),
                                pos,
                            ))
                        },
                    )
                }
                Node::Leaf { value: (h, r) } => Some((
                    Tree::leaf((h.clone(), *r | RetentionFlags::CHECKPOINT)),
                    root_addr.max_position(),
                )),
                Node::Nil => None,
            }
        }

        // checkpoint identifiers at the tip must be in increasing order
        if self
            .store
            .max_checkpoint_id()
            .map_err(ShardTreeError::Storage)?
            .as_ref()
            >= Some(&checkpoint_id)
        {
            return Ok(false);
        }

        // Update the rightmost subtree to add the `CHECKPOINT` flag to the right-most leaf (which
        // need not be a level-0 leaf; it's fine to rewind to a pruned state).
        if let Some(subtree) = self.store.last_shard().map_err(ShardTreeError::Storage)? {
            if let Some((replacement, pos)) = go(subtree.root_addr, &subtree.root) {
                self.store
                    .put_shard(LocatedTree {
                        root_addr: subtree.root_addr,
                        root: replacement,
                    })
                    .map_err(ShardTreeError::Storage)?;
                self.store
                    .add_checkpoint(checkpoint_id, Checkpoint::at_position(pos))
                    .map_err(ShardTreeError::Storage)?;

                // early return once we've updated the tree state
                self.prune_excess_checkpoints()?;
                return Ok(true);
            }
        }

        self.store
            .add_checkpoint(checkpoint_id, Checkpoint::tree_empty())
            .map_err(ShardTreeError::Storage)?;

        // TODO: it should not be necessary to do this on every checkpoint,
        // but currently that's how the reference tree behaves so we're maintaining
        // those semantics for test compatibility.
        self.prune_excess_checkpoints()?;
        Ok(true)
    }

    /// Marks the checkpoint having the given identifier for retention, exempting it from automatic
    /// pruning of excess checkpoints.
    ///
    /// A retained checkpoint is excluded from the `max_checkpoints` budget and is never removed by
    /// [`Self::prune_excess_checkpoints`]; it persists until it is released via
    /// [`Self::remove_retained_checkpoint`]. As a result, its root, and witnesses for marked leaves
    /// at or before it, remain computable even after it has aged more than `max_checkpoints` behind
    /// the tip of the tree. The identifier may be recorded even if no checkpoint with that
    /// identifier currently exists.
    pub fn ensure_retained(&mut self, checkpoint_id: C) -> Result<(), ShardTreeError<S::Error>> {
        self.store
            .add_retained_checkpoint(checkpoint_id)
            .map_err(ShardTreeError::Storage)
    }

    /// Releases a checkpoint previously retained via [`Self::ensure_retained`], allowing it to be
    /// pruned normally. Has no effect if the checkpoint was not retained.
    pub fn remove_retained_checkpoint(
        &mut self,
        checkpoint_id: &C,
    ) -> Result<(), ShardTreeError<S::Error>> {
        self.store
            .remove_retained_checkpoint(checkpoint_id)
            .map_err(ShardTreeError::Storage)
    }

    /// Removes the oldest checkpoints until at most `max_checkpoints` remain,
    /// clearing the `CHECKPOINT` and `MARKED` retention flags they were keeping
    /// alive so the affected leaves become prunable.
    ///
    /// Called after any operation that may add a checkpoint (e.g. [`Self::append`],
    /// [`Self::insert_frontier`], [`Self::batch_insert`], and [`Self::checkpoint`])
    /// to keep the checkpoint set bounded.
    #[tracing::instrument(skip(self))]
    fn prune_excess_checkpoints(&mut self) -> Result<(), ShardTreeError<S::Error>> {
        let checkpoint_count = self
            .store
            .checkpoint_count()
            .map_err(ShardTreeError::Storage)?;

        // Checkpoints in the explicit retention set are pinned: they are excluded from the
        // `max_checkpoints` budget and are never removed by automatic pruning. The budget therefore
        // applies only to the non-retained checkpoints.
        let retained = self
            .store
            .retained_checkpoints()
            .map_err(ShardTreeError::Storage)?;
        let mut retained_existing = 0usize;
        self.store
            .for_each_checkpoint(checkpoint_count, |cid, _| {
                if retained.contains(cid) {
                    retained_existing += 1;
                }
                Ok(())
            })
            .map_err(ShardTreeError::Storage)?;
        let prunable_count = checkpoint_count - retained_existing;

        trace!(
            "Tree has {} checkpoints ({} retained), max is {}",
            checkpoint_count,
            retained_existing,
            self.max_checkpoints,
        );
        if prunable_count > self.max_checkpoints {
            // Batch removals by subtree & create a list of the checkpoint identifiers that
            // will be removed from the checkpoints map. Removal candidates are the oldest
            // checkpoints outside the retention set, so they interleave with the retained
            // ones. Clearing their flags in a single ordered pass would let a removed
            // checkpoint re-mark a flag that a survivor at the same position still needs, so
            // the mark and unmark steps run as two passes over the checkpoints.
            let remove_count = prunable_count - self.max_checkpoints;
            let mut checkpoints_to_delete = vec![];
            let mut clear_positions: BTreeMap<Address, BTreeMap<Position, RetentionFlags>> =
                BTreeMap::new();
            self.store
                .with_checkpoints(checkpoint_count, |cid, checkpoint| {
                    let removing =
                        !retained.contains(cid) && checkpoints_to_delete.len() < remove_count;
                    // First pass: mark the flags referenced by every checkpoint being
                    // removed, batched by subtree.
                    if removing {
                        checkpoints_to_delete.push(cid.clone());

                        // Mark the flags to be cleared from the given position.
                        let mut mark_at = |pos, flags_to_clear: RetentionFlags| {
                            let subtree_addr = Self::subtree_addr(pos);
                            clear_positions
                                .entry(subtree_addr)
                                .and_modify(|to_clear| {
                                    to_clear
                                        .entry(pos)
                                        .and_modify(|flags| *flags |= flags_to_clear)
                                        .or_insert(flags_to_clear);
                                })
                                .or_insert_with(|| BTreeMap::from([(pos, flags_to_clear)]));
                        };

                        // The checkpoint's own leaf.
                        if let TreeState::AtPosition(pos) = checkpoint.tree_state() {
                            mark_at(pos, RetentionFlags::CHECKPOINT)
                        }
                        // The leaves whose marks this checkpoint removed.
                        for unmark_pos in checkpoint.marks_removed().iter() {
                            mark_at(*unmark_pos, RetentionFlags::MARKED)
                        }
                    }

                    Ok(())
                })
                .map_err(ShardTreeError::Storage)?;

            let deleted = checkpoints_to_delete
                .iter()
                .cloned()
                .collect::<BTreeSet<C>>();
            self.store
                .with_checkpoints(checkpoint_count, |cid, checkpoint| {
                    // Second pass: unmark flags that a surviving checkpoint still needs, so a
                    // removed checkpoint cannot clear a leaf preserved by one at the same
                    // position.
                    if !deleted.contains(cid) {
                        // Unmark flags that were marked for clearing above but which we now
                        // know we need to preserve.
                        let mut unmark_at = |pos, flags_to_clear: RetentionFlags| {
                            let subtree_addr = Self::subtree_addr(pos);
                            if let Some(to_clear) = clear_positions.get_mut(&subtree_addr) {
                                if let Some(flags) = to_clear.get_mut(&pos) {
                                    *flags &= !flags_to_clear;
                                }
                            }
                        };

                        // The checkpoint's own leaf.
                        if let TreeState::AtPosition(pos) = checkpoint.tree_state() {
                            unmark_at(pos, RetentionFlags::CHECKPOINT)
                        }
                        // The leaves whose marks this checkpoint removed.
                        for unmark_pos in checkpoint.marks_removed().iter() {
                            unmark_at(*unmark_pos, RetentionFlags::MARKED)
                        }
                    }

                    Ok(())
                })
                .map_err(ShardTreeError::Storage)?;

            // Remove any nodes that are fully preserved by later checkpoints.
            clear_positions.retain(|_, to_clear| {
                to_clear.retain(|_, flags| !flags.is_empty());
                !to_clear.is_empty()
            });

            trace!(
                "Removing checkpoints {:?}, pruning subtrees {:?}",
                checkpoints_to_delete,
                clear_positions,
            );

            // Prune each affected subtree
            for (subtree_addr, positions) in clear_positions.into_iter() {
                let to_clear = self
                    .store
                    .get_shard(subtree_addr)
                    .map_err(ShardTreeError::Storage)?;

                if let Some(to_clear) = to_clear {
                    let cleared = to_clear.clear_flags(positions);
                    self.store
                        .put_shard(cleared)
                        .map_err(ShardTreeError::Storage)?;
                }
            }

            // Now that the leaves have been pruned, actually remove the checkpoints
            for c in checkpoints_to_delete {
                self.store
                    .remove_checkpoint(&c)
                    .map_err(ShardTreeError::Storage)?;
            }
        }

        Ok(())
    }

    /// Truncates the tree, discarding all information after the checkpoint at the specified
    /// checkpoint depth.
    ///
    /// Returns `true` if the truncation succeeds or has no effect, or `false` if no checkpoint
    /// exists at the specified depth. Depth 0 refers to the most recent checkpoint in the tree;
    ///
    /// ## Parameters
    /// - `checkpoint_depth`: A zero-based index over the checkpoints that have been added to the
    ///   tree, in reverse checkpoint identifier order.
    pub fn truncate_to_checkpoint_depth(
        &mut self,
        checkpoint_depth: usize,
    ) -> Result<bool, ShardTreeError<S::Error>> {
        if let Some((checkpoint_id, c)) = self
            .store
            .get_checkpoint_at_depth(checkpoint_depth)
            .map_err(ShardTreeError::Storage)?
        {
            self.truncate_to_checkpoint_internal(&checkpoint_id, &c)?;
            Ok(true)
        } else {
            Ok(false)
        }
    }

    /// Truncates the tree, discarding all information after the specified checkpoint.
    ///
    /// Returns `true` if the truncation succeeds or has no effect, or `false` if no checkpoint
    /// exists for the specified checkpoint identifier.
    pub fn truncate_to_checkpoint(
        &mut self,
        checkpoint_id: &C,
    ) -> Result<bool, ShardTreeError<S::Error>> {
        if let Some(c) = self
            .store
            .get_checkpoint(checkpoint_id)
            .map_err(ShardTreeError::Storage)?
        {
            self.truncate_to_checkpoint_internal(checkpoint_id, &c)?;
            Ok(true)
        } else {
            Ok(false)
        }
    }

    fn truncate_to_checkpoint_internal(
        &mut self,
        checkpoint_id: &C,
        checkpoint: &Checkpoint,
    ) -> Result<(), ShardTreeError<S::Error>> {
        match checkpoint.tree_state() {
            TreeState::Empty => {
                self.store
                    .truncate_shards(0)
                    .map_err(ShardTreeError::Storage)?;
                self.store
                    .truncate_checkpoints_retaining(checkpoint_id)
                    .map_err(ShardTreeError::Storage)?;
                self.store
                    .put_cap(Tree::empty())
                    .map_err(ShardTreeError::Storage)?;
            }
            TreeState::AtPosition(position) => {
                let subtree_addr = Self::subtree_addr(position);
                let replacement = self
                    .store
                    .get_shard(subtree_addr)
                    .map_err(ShardTreeError::Storage)?
                    .and_then(|s| s.truncate_to_position(position));

                let cap_tree = LocatedTree {
                    root_addr: Self::root_addr(),
                    root: self.store.get_cap().map_err(ShardTreeError::Storage)?,
                };

                if let Some(truncated_cap) = cap_tree.truncate_to_position(position) {
                    self.store
                        .put_cap(truncated_cap.root)
                        .map_err(ShardTreeError::Storage)?;
                };

                if let Some(truncated) = replacement {
                    self.store
                        .truncate_shards(subtree_addr.index())
                        .map_err(ShardTreeError::Storage)?;
                    self.store
                        .put_shard(truncated)
                        .map_err(ShardTreeError::Storage)?;
                    self.store
                        .truncate_checkpoints_retaining(checkpoint_id)
                        .map_err(ShardTreeError::Storage)?;
                }
            }
        }

        Ok(())
    }

    /// Computes the Merkle root of the subtree rooted at `address`, as if the
    /// tree were truncated at `truncate_at`.
    ///
    /// This does not necessarily compute the root of the overall tree: it
    /// returns the root hash of the node identified by `address`, which may be
    /// any node in the tree, not just the top. To get the overall root, pass
    /// [`Self::root_addr`] as `address`, or use
    /// [`Self::root_at_checkpoint_depth`].
    ///
    /// # Arguments
    ///
    /// * `address` - The address of the node whose subtree root is computed.
    ///   It may be at any level from 0 up to (and including) the level of the
    ///   tree's overall root; the result is the root hash of that node's
    ///   subtree. Must be contained within [`Self::root_addr`] (see Panics).
    /// * `truncate_at` - An inclusive lower bound on positions to treat as
    ///   empty: every leaf at a position `>= truncate_at`, and every parent
    ///   whose value depends only on such leaves, is replaced by the empty
    ///   root for its level. This yields the root as of a prefix of the tree.
    ///   Pass a position at or beyond the tree's extent to include all
    ///   inserted leaves. This is how the checkpoint queries (e.g.
    ///   [`Self::root_at_checkpoint_depth`] and the witness methods) compute a
    ///   root or witness as of a past checkpoint, ignoring later leaves.
    ///
    /// # Panics
    ///
    /// Panics if `address` is not contained within [`Self::root_addr`], i.e.
    /// if it lies outside this tree.
    pub fn root(
        &self,
        address: Address,
        truncate_at: Position,
    ) -> Result<H, ShardTreeError<S::Error>> {
        assert!(Self::root_addr().contains(&address));

        // traverse the cap from root to leaf depth-first, either returning an existing
        // cached value for the node or inserting the computed value into the cache
        let (root, _) = self.root_internal(
            &LocatedPrunableTree {
                root_addr: Self::root_addr(),
                root: self.store.get_cap().map_err(ShardTreeError::Storage)?,
            },
            address,
            truncate_at,
        )?;
        Ok(root)
    }

    /// Like [`Self::root`], but caches the intermediate node hashes computed
    /// during the traversal.
    ///
    /// The recomputed cap is written back to the store via
    /// [`ShardStore::put_cap`]. Only hashes that do not incorporate
    /// empty/truncated nodes are cached.
    ///
    /// # Arguments
    ///
    /// * `address` - The address at which we want to compute the root hash.
    /// * `truncate_at` - An inclusive lower bound on positions to treat as
    ///   empty: every leaf at a position `>= truncate_at` is replaced by the
    ///   empty root for its level.
    pub fn root_caching(
        &mut self,
        address: Address,
        truncate_at: Position,
    ) -> Result<H, ShardTreeError<S::Error>> {
        let (root, updated_cap) = self.root_internal(
            &LocatedPrunableTree {
                root_addr: Self::root_addr(),
                root: self.store.get_cap().map_err(ShardTreeError::Storage)?,
            },
            address,
            truncate_at,
        )?;
        if let Some(updated_cap) = updated_cap {
            self.store
                .put_cap(updated_cap)
                .map_err(ShardTreeError::Storage)?;
        }
        Ok(root)
    }

    /// Computes the Merkle root of the subtree at `target_addr` by walking the
    /// cap downward from `cap`, returning it together with an optional rebuilt
    /// subtree the caller may write back to cache the hashes computed here
    /// (used by [`Self::root_caching`]; [`Self::root`] discards it).
    ///
    /// The walk stays inside the cap, whose nodes are never below the shard
    /// level, so `cap.root_addr.level()` is always `>= SHARD_HEIGHT`. Once it
    /// reaches the shard level it hands off to [`Self::root_from_shards`],
    /// which reads the actual shard data from the store. (`cap.root` being the
    /// tree located at `cap.root_addr` is assumed by construction of a
    /// [`LocatedPrunableTree`].)
    ///
    /// # Arguments
    ///
    /// * `cap` - The located subtree currently being processed: the cap node
    ///   at `cap.root_addr` paired with that address. At the entry point this
    ///   is the whole stored cap located at [`Self::root_addr`]; recursive
    ///   calls pass each child subtree paired with its child address.
    /// * `target_addr` - The address of the node whose root hash is computed.
    ///   It must be contained within `cap.root_addr`: at the same level or
    ///   deeper, with its range inside `cap.root_addr`'s; it is never an
    ///   ancestor of `cap.root_addr`.
    /// * `truncate_at` - An inclusive lower bound on positions to treat as
    ///   empty: every leaf at a position `>= truncate_at` is replaced by the
    ///   empty root for its level.
    ///
    /// `target_addr` may in principle be either a cap node (level
    /// `>= SHARD_HEIGHT`) or a node inside a shard (level `< SHARD_HEIGHT`),
    /// the latter resolved by [`Self::root_from_shards`]. In practice every
    /// in-crate caller passes a cap node: `root_at_checkpoint_*` use
    /// [`Self::root_addr`] (level `DEPTH`), and the frontier/witness path uses
    /// sibling addresses at levels `SHARD_HEIGHT..=DEPTH`. The sub-shard target
    /// path is supported but currently unused internally (still reachable via
    /// the public
    /// [`Self::root`] / [`Self::root_caching`] wrappers, which do not restrict
    /// the level).
    #[allow(clippy::type_complexity)]
    fn root_internal(
        &self,
        cap: &LocatedPrunableTree<S::H>,
        target_addr: Address,
        truncate_at: Position,
    ) -> Result<(H, Option<PrunableTree<H>>), ShardTreeError<S::Error>> {
        let cacheable = truncate_at >= cap.root_addr.position_range_end();
        let target_contains = target_addr.contains(&cap.root_addr);

        // Phase 1: Fast path — if a cached value is available and no truncation is needed,
        // return it immediately. `node_value()` returns the annotation for Parent nodes or
        // the hash for Leaf nodes.
        if cacheable && target_contains {
            if let Some(v) = cap.root.node_value() {
                return Ok((v.clone(), None));
            }
        }

        // Phase 2: Base case — at shard level, or a Nil node at the target address. In
        // both cases there is no cached subtree to preserve, so compute the root directly
        // from shard data. A Leaf at the target address is intentionally excluded here:
        // when it needs truncation it is expanded and reannotated by the Phase 3 descent,
        // preserving its cached hash as the replacement Parent's annotation (a cacheable
        // Leaf at the target is already served by the Phase 1 fast path).
        let at_shard_level =
            cap.root_addr.level() == ShardTree::<S, DEPTH, SHARD_HEIGHT>::subtree_level();
        if at_shard_level || (cap.root_addr == target_addr && matches!(&cap.root.0, Node::Nil)) {
            let addr = if target_contains {
                cap.root_addr
            } else {
                target_addr
            };
            let root = self.root_from_shards(addr, truncate_at)?;
            return Ok((
                root.clone(),
                if cacheable {
                    Some(Tree::leaf((root, RetentionFlags::EPHEMERAL)))
                } else {
                    None
                },
            ));
        }

        // Phase 3: Descent — recurse into children and combine results.

        // Save the original leaf value so we can re-annotate the replacement Parent with it,
        // preserving cached values when a Leaf is expanded.
        let orig_leaf_value = match &cap.root.0 {
            Node::Leaf { value } => Some(value.0.clone()),
            _ => None,
        };

        // Get children: real children for Parent nodes, empty children for Leaf/Nil.
        let (orig_left, orig_right) = match &cap.root.0 {
            Node::Parent { left, right, .. } => (left.clone(), right.clone()),
            _ => (Arc::new(Tree::empty()), Arc::new(Tree::empty())),
        };

        let (l_addr, r_addr) = cap
            .root_addr
            .children()
            .expect("descent is only reached above shard level");

        // Recurse into children. We skip computation in any subtree that will not
        // contribute data to the final result based on target_addr containment.
        let l_result = if r_addr.contains(&target_addr) {
            None
        } else {
            Some(self.root_internal(
                &LocatedPrunableTree {
                    root_addr: l_addr,
                    root: (*orig_left).clone(),
                },
                if l_addr.contains(&target_addr) {
                    target_addr
                } else {
                    l_addr
                },
                truncate_at,
            )?)
        };
        let r_result = if l_addr.contains(&target_addr) {
            None
        } else {
            Some(self.root_internal(
                &LocatedPrunableTree {
                    root_addr: r_addr,
                    root: (*orig_right).clone(),
                },
                if r_addr.contains(&target_addr) {
                    target_addr
                } else {
                    r_addr
                },
                truncate_at,
            )?)
        };

        // Compute the root value based on the child roots; these may contain the
        // hashes of empty/truncated nodes.
        let (root, new_left, new_right) = match (l_result, r_result) {
            (Some((l_root, new_left)), Some((r_root, new_right))) => (
                S::H::combine(l_addr.level(), &l_root, &r_root),
                new_left,
                new_right,
            ),
            (Some((l_root, new_left)), None) => (l_root, new_left, None),
            (None, Some((r_root, new_right))) => (r_root, None, new_right),
            (None, None) => unreachable!(),
        };

        // We don't use the `Tree::parent` constructor here, because it
        // creates `Arc`s for the child nodes internally, but if we don't
        // have a new child then we want to use the `Arc` for the existing
        // child.
        let new_parent = Tree(Node::Parent {
            ann: new_left
                .as_ref()
                .and_then(|l| l.node_value())
                .zip(new_right.as_ref().and_then(|r| r.node_value()))
                .map(|(l, r)| {
                    // Child node values are guaranteed to be non-truncated:
                    // the base case only caches a Leaf when `cacheable` is true
                    // (i.e. `truncate_at >= range_end`), and Parent annotations
                    // are built from these leaves recursively via this same
                    // `.zip().map()` chain, so the invariant holds at every level.
                    Arc::new(S::H::combine(l_addr.level(), l, r))
                }),
            left: new_left.map_or_else(|| orig_left, Arc::new),
            right: new_right.map_or_else(|| orig_right, Arc::new),
        });

        // If the original node was a Leaf, preserve its hash as the Parent annotation
        // so that future non-truncated lookups can use it via the fast-path.
        let replacement = match orig_leaf_value {
            Some(h) => new_parent.reannotate_root(Some(Arc::new(h))),
            None => new_parent,
        };

        Ok((root, Some(replacement)))
    }

    fn root_from_shards(
        &self,
        address: Address,
        truncate_at: Position,
    ) -> Result<H, ShardTreeError<S::Error>> {
        match address.context(Self::subtree_level()) {
            Either::Left(subtree_addr) => {
                // The requested root address is fully contained within one of the subtrees.
                Ok(if truncate_at <= address.position_range_start() {
                    H::empty_root(address.level())
                } else {
                    // get the child of the subtree with its root at `address`
                    self.store
                        .get_shard(subtree_addr)
                        .map_err(ShardTreeError::Storage)?
                        .ok_or_else(|| vec![subtree_addr])
                        .and_then(|subtree| {
                            subtree.subtree(address).map_or_else(
                                || Err(vec![address]),
                                |child| child.root_hash(truncate_at),
                            )
                        })
                        .map_err(QueryError::TreeIncomplete)?
                })
            }
            Either::Right(subtree_range) => {
                // The requested root requires hashing together the roots of several subtrees.
                let mut root_stack = vec![];
                let mut incomplete = vec![];

                for subtree_idx in subtree_range {
                    let subtree_addr = Address::from_parts(Self::subtree_level(), subtree_idx);
                    if truncate_at <= subtree_addr.position_range_start() {
                        break;
                    }

                    let subtree_root = self
                        .store
                        .get_shard(subtree_addr)
                        .map_err(ShardTreeError::Storage)?
                        .ok_or_else(|| vec![subtree_addr])
                        .and_then(|s| s.root_hash(truncate_at));

                    match subtree_root {
                        Ok(mut cur_hash) => {
                            if subtree_addr.index() % 2 == 0 {
                                root_stack.push((subtree_addr, cur_hash))
                            } else {
                                let mut cur_addr = subtree_addr;
                                while let Some((addr, hash)) = root_stack.pop() {
                                    if addr.parent() == cur_addr.parent() {
                                        cur_hash = H::combine(cur_addr.level(), &hash, &cur_hash);
                                        cur_addr = cur_addr.parent();
                                    } else {
                                        root_stack.push((addr, hash));
                                        break;
                                    }
                                }
                                root_stack.push((cur_addr, cur_hash));
                            }
                        }
                        Err(mut new_incomplete) => {
                            // Accumulate incomplete root information and continue, so that we can
                            // return the complete set of incomplete results.
                            incomplete.append(&mut new_incomplete);
                        }
                    }
                }

                if !incomplete.is_empty() {
                    return Err(ShardTreeError::Query(QueryError::TreeIncomplete(
                        incomplete,
                    )));
                }

                // Now hash with empty roots to obtain the root at maximum height
                if let Some((mut cur_addr, mut cur_hash)) = root_stack.pop() {
                    while let Some((addr, hash)) = root_stack.pop() {
                        while addr.level() > cur_addr.level() {
                            cur_hash = H::combine(
                                cur_addr.level(),
                                &cur_hash,
                                &H::empty_root(cur_addr.level()),
                            );
                            cur_addr = cur_addr.parent();
                        }
                        cur_hash = H::combine(cur_addr.level(), &hash, &cur_hash);
                        cur_addr = cur_addr.parent();
                    }

                    while cur_addr.level() < address.level() {
                        cur_hash = H::combine(
                            cur_addr.level(),
                            &cur_hash,
                            &H::empty_root(cur_addr.level()),
                        );
                        cur_addr = cur_addr.parent();
                    }

                    Ok(cur_hash)
                } else {
                    // if the stack is empty, we just return the default root at max height
                    Ok(H::empty_root(address.level()))
                }
            }
        }
    }

    /// Returns the position of the rightmost leaf as of the checkpoint at the given depth.
    ///
    /// Returns either the position of the checkpoint at the requested depth, the maximum leaf
    /// position in the tree if no checkpoint depth is provided, or `Ok(None)` if the tree is
    /// empty.
    ///
    /// Returns `ShardTreeError::Query(QueryError::CheckpointPruned)` if
    /// `checkpoint_depth.is_some()` and no checkpoint exists at the requested depth.
    ///
    /// ## Parameters
    /// - `checkpoint_depth`: A zero-based index over the checkpoints that have been added to the
    ///   tree, in reverse checkpoint identifier order, or `None` to request the overall maximum
    ///   leaf position in the tree.
    pub fn max_leaf_position(
        &self,
        checkpoint_depth: Option<usize>,
    ) -> Result<Option<Position>, ShardTreeError<S::Error>> {
        match checkpoint_depth {
            None => {
                // TODO: This relies on the invariant that the last shard in the subtrees vector is
                // never created without a leaf then being added to it. However, this may be a
                // difficult invariant to maintain when adding empty roots, so perhaps we need a
                // better way of tracking the actual max position of the tree; we might want to
                // just store it directly.
                Ok(self
                    .store
                    .last_shard()
                    .map_err(ShardTreeError::Storage)?
                    .and_then(|t| t.max_position()))
            }
            Some(depth) => self
                .store
                .get_checkpoint_at_depth(depth)
                .map_err(ShardTreeError::Storage)?
                .map(|(_, c)| c.position())
                .ok_or(ShardTreeError::Query(QueryError::CheckpointPruned)),
        }
    }

    /// Computes the root of the tree as of the checkpointed position having the specified
    /// checkpoint id.
    ///
    /// Returns `Ok(None)` if no checkpoint matches the specified ID.
    pub fn root_at_checkpoint_id(
        &self,
        checkpoint: &C,
    ) -> Result<Option<H>, ShardTreeError<S::Error>> {
        self.store
            .get_checkpoint(checkpoint)
            .map_err(ShardTreeError::Storage)?
            .map(|c| {
                c.position().map_or_else(
                    || Ok(H::empty_root(Self::root_addr().level())),
                    |pos| self.root(Self::root_addr(), pos + 1),
                )
            })
            .transpose()
    }

    /// Computes the root of the tree as of the checkpointed position having the specified
    /// checkpoint id, caching intermediate values produced while computing the root.
    ///
    /// Returns `Ok(None)` if no checkpoint matches the specified ID.
    pub fn root_at_checkpoint_id_caching(
        &mut self,
        checkpoint: &C,
    ) -> Result<Option<H>, ShardTreeError<S::Error>> {
        self.store
            .get_checkpoint(checkpoint)
            .map_err(ShardTreeError::Storage)?
            .map(|c| {
                c.position().map_or_else(
                    || Ok(H::empty_root(Self::root_addr().level())),
                    |pos| self.root_caching(Self::root_addr(), pos + 1),
                )
            })
            .transpose()
    }

    /// Computes the root of the tree as of the checkpointed position at the specified depth.
    ///
    /// Returns `Ok(None)` if no checkpoint exists at the requested depth.
    ///
    /// ## Parameters
    /// - `checkpoint_depth`: A zero-based index over the checkpoints that have been added to the
    ///   tree, in reverse checkpoint identifier order, or `None` to request the root computed over
    ///   all of the leaves in the tree.
    pub fn root_at_checkpoint_depth(
        &self,
        checkpoint_depth: Option<usize>,
    ) -> Result<Option<H>, ShardTreeError<S::Error>> {
        self.max_leaf_position(checkpoint_depth).map_or_else(
            |err| match err {
                ShardTreeError::Query(QueryError::CheckpointPruned) => Ok(None),
                err => Err(err),
            },
            |position| {
                position
                    .map_or_else(
                        || Ok(H::empty_root(Self::root_addr().level())),
                        |pos| self.root(Self::root_addr(), pos + 1),
                    )
                    .map(Some)
            },
        )
    }

    /// Computes the root of the tree as of the checkpointed position at the specified depth,
    /// caching intermediate values produced while computing the root.
    ///
    /// Returns `Ok(None)` if no checkpoint exists at the requested depth.
    ///
    /// ## Parameters
    /// - `checkpoint_depth`: A zero-based index over the checkpoints that have been added to the
    ///   tree, in reverse checkpoint identifier order, or `None` to request the root computed over
    ///   all of the leaves in the tree.
    pub fn root_at_checkpoint_depth_caching(
        &mut self,
        checkpoint_depth: Option<usize>,
    ) -> Result<Option<H>, ShardTreeError<S::Error>> {
        self.max_leaf_position(checkpoint_depth).map_or_else(
            |err| match err {
                ShardTreeError::Query(QueryError::CheckpointPruned) => Ok(None),
                err => Err(err),
            },
            |position| {
                position
                    .map_or_else(
                        || Ok(H::empty_root(Self::root_addr().level())),
                        |pos| self.root_caching(Self::root_addr(), pos + 1),
                    )
                    .map(Some)
            },
        )
    }

    fn witness_helper<Ctx>(
        mut ctx: Ctx,
        position: Position,
        as_of: Position,
        get_shard: impl Fn(&Ctx, Address) -> Result<Option<LocatedPrunableTree<H>>, S::Error>,
        mut root: impl FnMut(&mut Ctx, Address, Position) -> Result<H, ShardTreeError<S::Error>>,
    ) -> Result<MerklePath<H, DEPTH>, ShardTreeError<S::Error>> {
        let subtree_addr = Self::subtree_addr(position);

        // compute the witness for the specified position up to the subtree root
        let mut witness = get_shard(&ctx, subtree_addr)
            .map_err(ShardTreeError::Storage)?
            .map_or_else(
                || Err(QueryError::TreeIncomplete(vec![subtree_addr])),
                |subtree| subtree.witness(position, as_of + 1),
            )?;

        // compute the remaining parts of the witness up to the root
        let root_addr = Self::root_addr();
        let mut cur_addr = subtree_addr;
        while cur_addr != root_addr {
            witness.push(root(&mut ctx, cur_addr.sibling(), as_of + 1)?);
            cur_addr = cur_addr.parent();
        }

        Ok(MerklePath::from_parts(witness, position)
            .expect("witness has length DEPTH because we extended it to the root"))
    }

    fn witness_internal(
        &self,
        position: Position,
        as_of: Position,
    ) -> Result<MerklePath<H, DEPTH>, ShardTreeError<S::Error>> {
        Self::witness_helper(
            self,
            position,
            as_of,
            |ctx, shard_root| ctx.store.get_shard(shard_root),
            |ctx, address, truncate_at| ctx.root(address, truncate_at),
        )
    }

    fn witness_internal_caching(
        &mut self,
        position: Position,
        as_of: Position,
    ) -> Result<MerklePath<H, DEPTH>, ShardTreeError<S::Error>> {
        Self::witness_helper(
            self,
            position,
            as_of,
            |ctx, shard_root| ctx.store.get_shard(shard_root),
            |ctx, address, truncate_at| ctx.root_caching(address, truncate_at),
        )
    }

    /// Computes the witness for the leaf at the specified position, as of the given checkpoint
    /// depth.
    ///
    /// Returns `ShardTreeError::Query(QueryError::CheckpointPruned)` if no checkpoint exists at
    /// the requested depth.
    pub fn witness_at_checkpoint_depth(
        &self,
        position: Position,
        checkpoint_depth: usize,
    ) -> Result<Option<MerklePath<H, DEPTH>>, ShardTreeError<S::Error>> {
        let checkpoint = self
            .store
            .get_checkpoint_at_depth(checkpoint_depth)
            .map_err(ShardTreeError::Storage)?;

        match checkpoint {
            None => Ok(None),
            Some((_, c)) => {
                let as_of = c.position().filter(|p| position <= *p).ok_or_else(|| {
                    QueryError::NotContained(Address::from_parts(Level::from(0), position.into()))
                })?;

                self.witness_internal(position, as_of).map(Some)
            }
        }
    }

    /// Computes the witness for the leaf at the specified position, as of the given checkpoint
    /// depth.
    ///
    /// This implementation will mutate the tree to cache intermediate root (ommer) values that are
    /// computed in the process of constructing the witness, so as to avoid the need to recompute
    /// those values from potentially large numbers of subtree roots in the future.
    ///
    /// Returns `ShardTreeError::Query(QueryError::CheckpointPruned)` if no checkpoint exists at
    /// the requested depth. It is not possible to
    pub fn witness_at_checkpoint_depth_caching(
        &mut self,
        position: Position,
        checkpoint_depth: usize,
    ) -> Result<Option<MerklePath<H, DEPTH>>, ShardTreeError<S::Error>> {
        let checkpoint = self
            .store
            .get_checkpoint_at_depth(checkpoint_depth)
            .map_err(ShardTreeError::Storage)?;

        match checkpoint {
            None => Ok(None),
            Some((_, c)) => {
                let as_of = c.position().filter(|p| position <= *p).ok_or_else(|| {
                    QueryError::NotContained(Address::from_parts(Level::from(0), position.into()))
                })?;

                self.witness_internal_caching(position, as_of).map(Some)
            }
        }
    }

    /// Computes the witness for the leaf at the specified position, as of the given checkpoint.
    ///
    /// Returns Ok(None) if no such checkpoint exists.
    pub fn witness_at_checkpoint_id(
        &self,
        position: Position,
        checkpoint_id: &C,
    ) -> Result<Option<MerklePath<H, DEPTH>>, ShardTreeError<S::Error>> {
        self.store
            .get_checkpoint(checkpoint_id)
            .map_err(ShardTreeError::Storage)?
            .map(|checkpoint| {
                let as_of = checkpoint
                    .position()
                    .filter(|p| position <= *p)
                    .ok_or_else(|| {
                        QueryError::NotContained(Address::from_parts(
                            Level::from(0),
                            position.into(),
                        ))
                    })?;

                self.witness_internal(position, as_of)
            })
            .transpose()
    }

    /// Computes the witness for the leaf at the specified position, as of the given checkpoint.
    ///
    /// This implementation will mutate the tree to cache intermediate root (ommer) values that are
    /// computed in the process of constructing the witness, so as to avoid the need to recompute
    /// those values from potentially large numbers of subtree roots in the future.
    ///
    /// Returns Ok(None) if no such checkpoint exists.
    pub fn witness_at_checkpoint_id_caching(
        &mut self,
        position: Position,
        checkpoint_id: &C,
    ) -> Result<Option<MerklePath<H, DEPTH>>, ShardTreeError<S::Error>> {
        self.store
            .get_checkpoint(checkpoint_id)
            .map_err(ShardTreeError::Storage)?
            .map(|checkpoint| {
                let as_of = checkpoint
                    .position()
                    .filter(|p| position <= *p)
                    .ok_or_else(|| {
                        QueryError::NotContained(Address::from_parts(
                            Level::from(0),
                            position.into(),
                        ))
                    })?;

                self.witness_internal_caching(position, as_of)
            })
            .transpose()
    }

    /// Make a marked leaf at a position eligible to be pruned.
    ///
    /// If the checkpoint associated with the specified identifier does not exist because the
    /// corresponding checkpoint would have been more than `max_checkpoints` deep, the removal is
    /// recorded as of the first existing checkpoint and the associated leaves will be pruned when
    /// that checkpoint is subsequently removed.
    ///
    /// Returns `Ok(true)` if a mark was successfully removed from the leaf at the specified
    /// position, `Ok(false)` if the tree does not contain a leaf at the specified position or is
    /// not marked, or an error if one is produced by the underlying data store.
    pub fn remove_mark(
        &mut self,
        position: Position,
        as_of_checkpoint: Option<&C>,
    ) -> Result<bool, ShardTreeError<S::Error>> {
        match self
            .store
            .get_shard(Self::subtree_addr(position))
            .map_err(ShardTreeError::Storage)?
        {
            Some(shard)
                if shard
                    .value_at_position(position)
                    .iter()
                    .any(|(_, r)| r.is_marked()) =>
            {
                match as_of_checkpoint {
                    Some(cid)
                        if Some(cid)
                            >= self
                                .store
                                .min_checkpoint_id()
                                .map_err(ShardTreeError::Storage)?
                                .as_ref() =>
                    {
                        self.store
                            .update_checkpoint_with(cid, |checkpoint| {
                                checkpoint.mark_removed(position);
                                Ok(())
                            })
                            .map_err(ShardTreeError::Storage)
                    }
                    _ => {
                        // if no checkpoint was provided, or if the checkpoint is too far in the past,
                        // remove the mark directly.
                        self.store
                            .put_shard(
                                shard.clear_flags(BTreeMap::from([(
                                    position,
                                    RetentionFlags::MARKED,
                                )])),
                            )
                            .map_err(ShardTreeError::Storage)?;
                        Ok(true)
                    }
                }
            }
            _ => Ok(false),
        }
    }
}

#[cfg(test)]
mod tests {
    use std::convert::Infallible;

    use assert_matches::assert_matches;
    use proptest::prelude::*;

    use incrementalmerkletree::{
        frontier::{Frontier, NonEmptyFrontier},
        Address, Hashable, Level, Marking, MerklePath, Position, Retention,
    };
    use incrementalmerkletree_testing::{
        arb_operation, check_append, check_checkpoint_rewind, check_operations, check_remove_mark,
        check_rewind_remove_mark, check_root_hashes, check_witness_consistency, check_witnesses,
        complete_tree::CompleteTree, compute_root_from_witness, CombinedTree, SipHashable,
    };

    use crate::{
        error::{QueryError, ShardTreeError},
        store::{memory::MemoryShardStore, ShardStore},
        testing::{
            arb_char_str, arb_shard_layout, arb_shardtree_sized, check_shard_sizes,
            check_shardtree_insertion, check_witness_with_pruned_subtrees,
        },
        InsertionError, LocatedPrunableTree, LocatedTree, ShardTree,
    };

    #[test]
    fn shardtree_insertion() {
        let tree = empty_tree::<String, 4, 3>();

        check_shardtree_insertion(tree)
    }

    #[test]
    fn shard_sizes() {
        let tree = empty_tree::<String, 4, 2>();

        check_shard_sizes(tree)
    }

    #[test]
    fn witness_with_pruned_subtrees() {
        let tree = empty_tree::<String, 6, 3>();

        check_witness_with_pruned_subtrees(tree)
    }

    fn new_tree(m: usize) -> ShardTree<MemoryShardStore<String, usize>, 4, 3> {
        ShardTree::new(MemoryShardStore::empty(), m)
    }

    /// An empty in-memory tree of the given depth and shard height, retaining up
    /// to 100 checkpoints. Generic over the hash type so it serves both the
    /// `String` and `SipHashable` test trees.
    fn empty_tree<H, const DEPTH: u8, const SHARD_HEIGHT: u8>(
    ) -> ShardTree<MemoryShardStore<H, u32>, DEPTH, SHARD_HEIGHT>
    where
        H: Hashable + Clone + PartialEq,
    {
        ShardTree::new(MemoryShardStore::empty(), 100)
    }

    #[test]
    fn append() {
        check_append(new_tree);
    }

    #[test]
    fn root_hashes() {
        check_root_hashes(new_tree);
    }

    #[test]
    fn witnesses() {
        check_witnesses(new_tree);
    }

    #[test]
    fn witness_consistency() {
        check_witness_consistency(new_tree);
    }

    #[test]
    fn checkpoint_rewind() {
        check_checkpoint_rewind(new_tree);
    }

    #[test]
    fn remove_mark() {
        check_remove_mark(new_tree);
    }

    #[test]
    fn rewind_remove_mark() {
        check_rewind_remove_mark(new_tree);
    }

    #[test]
    fn frontier_empty_tree() {
        let tree = empty_tree::<String, 4, 3>();
        assert_eq!(tree.frontier().unwrap(), Frontier::empty());
    }

    #[test]
    fn frontier_single_leaf() {
        let mut tree = empty_tree::<String, 4, 3>();
        tree.append("a".to_string(), Retention::Ephemeral).unwrap();

        let frontier = tree.frontier().unwrap();
        assert_eq!(
            frontier,
            Frontier::from_parts(Position::from(0), "a".to_string(), vec![]).unwrap()
        );
    }

    #[test]
    fn frontier_within_single_shard() {
        // Insert a frontier at position 5 within the first shard (SHARD_HEIGHT=3).
        // Position 5 = binary 101: ommers at level 0 ("e") and level 2 ("abcd").
        let original = NonEmptyFrontier::from_parts(
            Position::from(5),
            "f".to_string(),
            vec!["e".to_string(), "abcd".to_string()],
        )
        .unwrap();

        let mut tree = empty_tree::<String, 4, 3>();
        tree.insert_frontier_nodes(original.clone(), Retention::Ephemeral)
            .unwrap();

        let frontier = tree.frontier().unwrap();
        assert_eq!(frontier.value(), Some(&original));
    }

    #[test]
    fn frontier_multi_shard_roundtrip() {
        // Insert a frontier at position 9, which is in the second shard (SHARD_HEIGHT=3).
        // Position 9 = binary 1001: ommers at level 0 ("i") and level 3 ("abcdefgh").
        // Level 0 ommer "i" is within the shard; level 3 ommer "abcdefgh" is the
        // sibling shard's root above the shard level.
        let original = NonEmptyFrontier::from_parts(
            Position::from(9),
            "j".to_string(),
            vec!["i".to_string(), "abcdefgh".to_string()],
        )
        .unwrap();

        let mut tree = empty_tree::<String, 4, 3>();
        tree.insert_frontier_nodes(original.clone(), Retention::Ephemeral)
            .unwrap();

        let frontier = tree.frontier().unwrap();
        assert_eq!(frontier.value(), Some(&original));
    }

    #[test]
    fn frontier_from_appended_leaves() {
        // Append leaves with the last one marked, so it isn't pruned away.
        let mut tree = empty_tree::<String, 4, 3>();
        for c in 'a'..='i' {
            tree.append(c.to_string(), Retention::Ephemeral).unwrap();
        }
        tree.append("j".to_string(), Retention::Marked).unwrap();

        let frontier = tree.frontier().unwrap();
        // Position 9 = binary 1001: ommers at level 0 ("i") and level 3 ("abcdefgh")
        let expected = NonEmptyFrontier::from_parts(
            Position::from(9),
            "j".to_string(),
            vec!["i".to_string(), "abcdefgh".to_string()],
        )
        .unwrap();
        assert_eq!(frontier.value(), Some(&expected));
    }

    #[test]
    fn frontier_with_append() {
        // Insert a frontier at position 9, which is in the second shard (SHARD_HEIGHT=3).
        // Position 9 = binary 1001: ommers at level 0 ("i") and level 3 ("abcdefgh").
        // Level 0 ommer "i" is within the shard; level 3 ommer "abcdefgh" is the
        // sibling shard's root above the shard level.
        let original = NonEmptyFrontier::from_parts(
            Position::from(9),
            "j".to_string(),
            vec!["i".to_string(), "abcdefgh".to_string()],
        )
        .unwrap();

        let mut tree = empty_tree::<String, 4, 3>();
        tree.insert_frontier_nodes(original, Retention::Ephemeral)
            .unwrap();

        // Now, append leaves to the inserted frontier state.
        for c in 'k'..='n' {
            tree.append(c.to_string(), Retention::Ephemeral).unwrap();
        }
        tree.append("o".to_string(), Retention::Marked).unwrap();

        let frontier = tree.frontier().unwrap();
        let expected = NonEmptyFrontier::from_parts(
            Position::from(14),
            "o".to_string(),
            vec!["mn".to_string(), "ijkl".to_string(), "abcdefgh".to_string()],
        )
        .unwrap();
        assert_eq!(frontier.value(), Some(&expected));
    }

    #[test]
    fn checkpoint_pruning_repeated() {
        // Create a tree with some leaves.
        let mut tree = new_tree(10);
        for c in 'a'..='c' {
            tree.append(c.to_string(), Retention::Ephemeral).unwrap();
        }

        // Repeatedly checkpoint the tree at the same position until the checkpoint cache
        // is full (creating a sequence of checkpoints in between which no new leaves were
        // appended to the tree).
        for i in 0..10 {
            assert_eq!(tree.checkpoint(i), Ok(true));
        }

        // Create one more checkpoint at the same position, causing the oldest in the
        // cache to be pruned.
        assert_eq!(tree.checkpoint(10), Ok(true));

        // Append a leaf to the tree and checkpoint it, causing the next oldest in the
        // cache to be pruned.
        assert_eq!(
            tree.append(
                'd'.to_string(),
                Retention::Checkpoint {
                    id: 11,
                    marking: Marking::None
                },
            ),
            Ok(()),
        );
    }

    #[test]
    fn checkpoint_pruning_with_interleaved_retained() {
        // Regression test: a retained anchor interleaved between prunable checkpoints at
        // the same tree position. Pruning checkpoints from both sides of the anchor must
        // not corrupt the anchor's leaf retention flags, and repeated pruning against a
        // non-growing tree must not panic in `clear_flags`.
        let mut tree = new_tree(10);

        // Growth phase: leaves appended with periodic checkpoints, one retained anchor.
        let mut checkpoint_id = 0usize;
        for c in 'a'..='h' {
            tree.append(
                c.to_string(),
                Retention::Checkpoint {
                    id: checkpoint_id,
                    marking: if c == 'c' {
                        Marking::Marked
                    } else {
                        Marking::None
                    },
                },
            )
            .unwrap();
            if checkpoint_id % 5 == 0 {
                tree.ensure_retained(checkpoint_id).unwrap();
            }
            checkpoint_id += 1;
        }

        // Stall phase: the tree stops growing; checkpoints pile up at the same position
        // with retained anchors interleaved among them.
        for i in 0..40 {
            assert_eq!(tree.checkpoint(checkpoint_id), Ok(true));
            if i % 7 == 0 {
                tree.ensure_retained(checkpoint_id).unwrap();
            }
            checkpoint_id += 1;
        }

        // Resume growth so folded regions and live flags interact across prunes.
        for c in 'i'..='p' {
            tree.append(
                c.to_string(),
                Retention::Checkpoint {
                    id: checkpoint_id,
                    marking: Marking::None,
                },
            )
            .unwrap();
            checkpoint_id += 1;
        }

        // Another stall on top of the grown tree.
        for _ in 0..40 {
            assert_eq!(tree.checkpoint(checkpoint_id), Ok(true));
            checkpoint_id += 1;
        }

        // Every retained anchor's root must still be computable.
        for retained in [0usize, 5] {
            assert!(
                tree.root_at_checkpoint_id(&retained).unwrap().is_some(),
                "retained anchor {retained} lost its root"
            );
        }
    }

    #[test]
    fn checkpoint_pruning_with_retained_random_ops() {
        // Deterministic pseudo-random soak of pruning against retained anchors: appends
        // (ephemeral, marked, and checkpointing), same-position checkpoint runs, periodic
        // retained anchors, released anchors, and mark removals recorded in checkpoints.
        // Guards the `prune_excess_checkpoints` clear/preserve bookkeeping against
        // interleaved retained checkpoints; a bookkeeping error surfaces as a
        // "Tree state inconsistent with checkpoints" panic in `clear_flags`.
        //
        // The two tests above pin the exact bugs deterministically, so this stays a light
        // fuzz by default. Set `SHARDTREE_SOAK_SEEDS` to widen the sweep locally.
        let seeds: u64 = std::env::var("SHARDTREE_SOAK_SEEDS")
            .ok()
            .and_then(|raw| raw.parse().ok())
            .unwrap_or(8);
        for seed in 1_u64..=seeds {
            let mut state = seed.wrapping_mul(0x9E37_79B9_7F4A_7C15);
            let mut next = move || {
                state = state
                    .wrapping_mul(6364136223846793005)
                    .wrapping_add(1442695040888963407);
                (state >> 33) as u32
            };

            let mut tree = new_tree(10);
            let mut checkpoint_id = 0usize;
            let mut marked_positions: Vec<Position> = Vec::new();
            let mut retained_ids: Vec<usize> = Vec::new();
            let mut appended = 0u64;

            for _ in 0..4_000 {
                match next() % 10 {
                    // Append runs keep the tree growing in bursts.
                    0..=2 if appended < 14 => {
                        let marking = if next() % 4 == 0 {
                            Marking::Marked
                        } else {
                            Marking::None
                        };
                        if matches!(marking, Marking::Marked) {
                            marked_positions.push(Position::from(appended));
                        }
                        tree.append(
                            format!("l{appended}"),
                            Retention::Checkpoint {
                                id: checkpoint_id,
                                marking,
                            },
                        )
                        .unwrap();
                        appended += 1;
                        checkpoint_id += 1;
                    }
                    3..=4 if appended < 14 => {
                        tree.append(format!("l{appended}"), Retention::Ephemeral)
                            .unwrap();
                        appended += 1;
                    }
                    0..=4 => {
                        assert_eq!(tree.checkpoint(checkpoint_id), Ok(true));
                        checkpoint_id += 1;
                    }
                    // Same-position checkpoint runs (stalled chain).
                    5..=7 => {
                        let run = 1 + next() % 12;
                        for _ in 0..run {
                            assert_eq!(tree.checkpoint(checkpoint_id), Ok(true));
                            checkpoint_id += 1;
                        }
                    }
                    // Retain the newest checkpoint as a durable anchor.
                    8 => {
                        if checkpoint_id > 0 {
                            let id = checkpoint_id - 1;
                            tree.ensure_retained(id).unwrap();
                            retained_ids.push(id);
                            if retained_ids.len() > 6 {
                                let release = retained_ids.remove(0);
                                tree.remove_retained_checkpoint(&release).unwrap();
                            }
                        }
                    }
                    // Remove an old mark, recording marks_removed in the current checkpoint.
                    _ => {
                        if !marked_positions.is_empty() {
                            let idx = (next() as usize) % marked_positions.len();
                            let pos = marked_positions.swap_remove(idx);
                            let _ = tree.remove_mark(pos, None);
                        }
                    }
                }
            }
        }
    }

    #[test]
    fn regression_prune_excess_checkpoints_bounds_all_methods() {
        // Regression test: every method that may add a checkpoint must trigger
        // `prune_excess_checkpoints`, keeping the stored checkpoint count at or
        // below `max_checkpoints`. We push past the bound through each entry
        // point in isolation and assert the count settles at the maximum.
        const MAX: usize = 2;
        const N: usize = 5;

        // `checkpoint`: repeated checkpoints at the same position.
        let mut tree = new_tree(MAX);
        tree.append("a".to_string(), Retention::Ephemeral).unwrap();
        for i in 0..N {
            assert_eq!(tree.checkpoint(i), Ok(true));
        }
        assert_eq!(tree.store().checkpoint_count(), Ok(MAX));

        // `append` with `Checkpoint` retention.
        let mut tree = new_tree(MAX);
        for (i, c) in ('a'..='e').enumerate() {
            tree.append(
                c.to_string(),
                Retention::Checkpoint {
                    id: i,
                    marking: Marking::None,
                },
            )
            .unwrap();
        }
        assert_eq!(tree.store().checkpoint_count(), Ok(MAX));

        // `insert_frontier` with `Checkpoint` retention, inserting successively
        // longer frontiers so each call checkpoints a new position.
        let mut tree = new_tree(MAX);
        let mut frontier = Frontier::<String, 4>::empty();
        for (i, c) in ('a'..='e').enumerate() {
            frontier.append(c.to_string());
            tree.insert_frontier(
                frontier.clone(),
                Retention::Checkpoint {
                    id: i,
                    marking: Marking::None,
                },
            )
            .unwrap();
        }
        assert_eq!(tree.store().checkpoint_count(), Ok(MAX));

        // `batch_insert` with `Checkpoint` retention on every value.
        let mut tree = new_tree(MAX);
        let values = ('a'..='e').enumerate().map(|(i, c)| {
            (
                c.to_string(),
                Retention::Checkpoint {
                    id: i,
                    marking: Marking::None,
                },
            )
        });
        tree.batch_insert(Position::from(0), values).unwrap();
        assert_eq!(tree.store().checkpoint_count(), Ok(MAX));
    }

    #[test]
    fn avoid_pruning_reference() {
        fn test_with_marking(
            frontier_marking: Marking,
        ) -> Result<Option<MerklePath<String, 6>>, ShardTreeError<Infallible>> {
            let mut tree = ShardTree::<MemoryShardStore<String, usize>, 6, 3>::new(
                MemoryShardStore::empty(),
                5,
            );

            let frontier_end = Position::from((1 << 3) - 3);
            let mut f0 = Frontier::<String, 6>::empty();
            for c in 'a'..='f' {
                f0.append(c.to_string());
            }

            let frontier = Frontier::from_parts(
                frontier_end,
                "f".to_owned(),
                vec!["e".to_owned(), "abcd".to_owned()],
            )
            .unwrap();

            // Insert a frontier two leaves from the end of the first shard, checkpointed,
            // with the specified marking.
            tree.insert_frontier(
                frontier,
                Retention::Checkpoint {
                    id: 1,
                    marking: frontier_marking,
                },
            )
            .unwrap();

            // Insert a few leaves beginning at the subsequent position, so as to cross the shard
            // boundary.
            tree.batch_insert(
                frontier_end + 1,
                ('g'..='j').map(|c| (c.to_string(), Retention::Ephemeral)),
            )
            .unwrap();

            // Trigger pruning by adding 5 more checkpoints
            for i in 2..7 {
                tree.checkpoint(i).unwrap();
            }

            // Insert nodes that require the pruned nodes for witnessing
            tree.batch_insert(
                frontier_end - 1,
                ('e'..='f').map(|c| (c.to_string(), Retention::Marked)),
            )
            .unwrap();

            // Compute the witness
            tree.witness_at_checkpoint_id(frontier_end, &6)
        }

        // If we insert the frontier with Marking::None, the frontier nodes are treated
        // as ephemeral nodes and are pruned, leaving an incomplete tree.
        assert_matches!(
            test_with_marking(Marking::None),
            Err(ShardTreeError::Query(QueryError::TreeIncomplete(_)))
        );

        // If we insert the frontier with Marking::Reference, the frontier nodes will
        // not be pruned on completion of the subtree, and thus we'll be able to compute
        // the witness.
        let expected_witness = MerklePath::from_parts(
            [
                "e",
                "gh",
                "abcd",
                "ij______",
                "________________",
                "________________________________",
            ]
            .iter()
            .map(|s| s.to_string())
            .collect(),
            Position::from(5),
        )
        .unwrap();

        let witness = test_with_marking(Marking::Reference).unwrap();
        assert_eq!(witness, Some(expected_witness));
    }

    /// Builds a tree (`max_checkpoints == 3`) containing a `Marked` leaf at position 2 and an
    /// explicitly-retained "anchor" checkpoint (id 1) at position 5, then advances the tree across
    /// the shard boundary while adding more than `max_checkpoints` additional checkpoints, so that
    /// checkpoint 1 would ordinarily be pruned were it not retained.
    fn anchor_checkpoint_tree() -> ShardTree<MemoryShardStore<String, usize>, 6, 3> {
        let mut tree = ShardTree::new(MemoryShardStore::empty(), 3);

        // Append an initial series of leaves, marking the leaf at position 2 so that we can
        // later compute a witness for it.
        for c in 'a'..='e' {
            let retention = if c == 'c' {
                Retention::Marked
            } else {
                Retention::Ephemeral
            };
            tree.append(c.to_string(), retention).unwrap();
        }

        // Create a checkpoint (id 1) at position 5 and retain it explicitly so it becomes a
        // durable anchor.
        tree.append(
            "f".to_string(),
            Retention::Checkpoint {
                id: 1,
                marking: Marking::None,
            },
        )
        .unwrap();
        tree.ensure_retained(1).unwrap();

        // Advance the tree across the shard boundary, adding ordinary checkpoints (ids 2..=5)
        // such that checkpoint 1 ages more than `max_checkpoints` behind the tip.
        let mut next_id = 2;
        for c in 'g'..='n' {
            let retention = if matches!(c, 'h' | 'j' | 'l' | 'n') {
                let id = next_id;
                next_id += 1;
                Retention::Checkpoint {
                    id,
                    marking: Marking::None,
                }
            } else {
                Retention::Ephemeral
            };
            tree.append(c.to_string(), retention).unwrap();
        }

        tree
    }

    #[test]
    fn anchor_checkpoint_survives_pruning() {
        let tree = anchor_checkpoint_tree();

        // Three more recent ordinary checkpoints exist and `max_checkpoints` is 3, so an
        // un-retained checkpoint in checkpoint 1's position would have been pruned. Because
        // checkpoint 1 was explicitly retained, its record must be preserved.
        assert_matches!(tree.store().get_checkpoint(&1), Ok(Some(_)));
    }

    #[test]
    fn witness_marked_leaf_as_of_anchor_checkpoint() {
        let tree = anchor_checkpoint_tree();

        // The witness for the `Marked` leaf at position 2, as of the anchor checkpoint (id 1) at
        // position 5, must remain computable even though checkpoint 1 has aged more than
        // `max_checkpoints` behind the tip of the tree.
        let expected_path: Vec<String> = [
            "d",
            "ab",
            "ef__",
            "________",
            "________________",
            "________________________________",
        ]
        .iter()
        .map(|s| s.to_string())
        .collect();

        let witness = tree
            .witness_at_checkpoint_id(Position::from(2), &1)
            .unwrap()
            .expect("a witness can be computed as of the anchored checkpoint");
        assert_eq!(witness.path_elems(), expected_path.as_slice());

        // The root as of the anchor checkpoint must also be computable, and must be consistent
        // with the witness for the marked leaf.
        let root = tree
            .root_at_checkpoint_id(&1)
            .unwrap()
            .expect("a root can be computed as of the anchored checkpoint");
        assert_eq!(
            root,
            compute_root_from_witness("c".to_string(), Position::from(2), &expected_path)
        );
    }

    #[test]
    fn released_anchor_checkpoint_is_pruned() {
        let mut tree = anchor_checkpoint_tree();

        // The anchor is retained, so it survives so far.
        assert_matches!(tree.store().get_checkpoint(&1), Ok(Some(_)));

        // Release the retention, then trigger pruning by adding another checkpoint. Checkpoint 1
        // is now an ordinary checkpoint well beyond the `max_checkpoints` window, so it must be
        // pruned.
        tree.remove_retained_checkpoint(&1).unwrap();
        tree.append(
            "o".to_string(),
            Retention::Checkpoint {
                id: 6,
                marking: Marking::None,
            },
        )
        .unwrap();

        assert_matches!(tree.store().get_checkpoint(&1), Ok(None));
    }

    // Combined tree tests
    #[allow(clippy::type_complexity)]
    fn new_combined_tree<H>(
        max_checkpoints: usize,
    ) -> CombinedTree<
        H,
        usize,
        CompleteTree<H, usize, 4>,
        ShardTree<MemoryShardStore<H, usize>, 4, 3>,
    >
    where
        H: Hashable + Ord + Clone + core::fmt::Debug,
    {
        CombinedTree::new(
            CompleteTree::new(max_checkpoints),
            ShardTree::new(MemoryShardStore::empty(), max_checkpoints),
        )
    }

    #[test]
    fn combined_append() {
        check_append::<String, usize, _, _>(new_combined_tree);
    }

    #[test]
    fn combined_rewind_remove_mark() {
        check_rewind_remove_mark(new_combined_tree);
    }

    proptest! {
        #![proptest_config(ProptestConfig::with_cases(100000))]

        #[test]
        fn check_randomized_u64_ops(
            ops in proptest::collection::vec(
                arb_operation(
                    (0..32u64).prop_map(SipHashable),
                    (0u64..100).prop_map(Position::from)
                ),
                1..100
            )
        ) {
            let tree = new_combined_tree(100);
            let indexed_ops = ops.iter().enumerate().map(|(i, op)| op.map_checkpoint_id(|_| i)).collect::<Vec<_>>();
            check_operations(tree, &indexed_ops)?;
        }

        #[test]
        fn check_randomized_str_ops(
            ops in proptest::collection::vec(
                arb_operation(
                    (97u8..123).prop_map(|c| char::from(c).to_string()),
                    (0u64..100).prop_map(Position::from)
                ),
                1..100
            )
        ) {
            let tree = new_combined_tree(100);
            let indexed_ops = ops.iter().enumerate().map(|(i, op)| op.map_checkpoint_id(|_| i)).collect::<Vec<_>>();
            check_operations(tree, &indexed_ops)?;
        }
    }

    fn check_caching<const DEPTH: u8, const SHARD_HEIGHT: u8>(
        mut tree: ShardTree<MemoryShardStore<String, usize>, DEPTH, SHARD_HEIGHT>,
        marked_positions: Vec<Position>,
    ) {
        if let Some(max_leaf_pos) = tree.max_leaf_position(None).unwrap() {
            let max_complete_addr =
                Address::above_position(max_leaf_pos.root_level(), max_leaf_pos);
            let root = tree.root(max_complete_addr, max_leaf_pos + 1);
            let caching_root = tree.root_caching(max_complete_addr, max_leaf_pos + 1);
            assert_matches!(root, Ok(_));
            assert_eq!(root, caching_root);
        }

        if let Ok(Some(checkpoint_position)) = tree.max_leaf_position(Some(0)) {
            for pos in marked_positions {
                let witness = tree.witness_at_checkpoint_depth(pos, 0);
                let caching_witness = tree.witness_at_checkpoint_depth_caching(pos, 0);
                if pos <= checkpoint_position {
                    assert_matches!(witness, Ok(_));
                }
                assert_eq!(witness, caching_witness);
            }
        }
    }

    proptest! {
        #![proptest_config(ProptestConfig::with_cases(100))]

        #[test]
        fn check_shardtree_caching(
            (tree, _, marked_positions) in arb_shardtree_sized::<_, 6, 3>(arb_char_str())
        ) {
            check_caching(tree, marked_positions);
        }

        #[test]
        fn check_shardtree_caching_small_shards(
            (tree, _, marked_positions) in arb_shardtree_sized::<_, 4, 2>(arb_char_str())
        ) {
            check_caching(tree, marked_positions);
        }
    }

    proptest! {
        #[test]
        fn marked_positions_matches_shard_layout(
            layout in arb_shard_layout(arb_char_str(), 2, 8, 4)
        ) {
            let mut tree = empty_tree::<String, 4, 2>();
            for shard in &layout.shards {
                tree.store.put_shard(shard.clone()).unwrap();
            }
            prop_assert_eq!(tree.marked_positions().unwrap(), layout.marked_positions);
        }

        // Regression test: the dense MemoryShardStore back-fills empty
        // placeholder shards, so get_shard_roots reports a contiguous
        // 0..=max_populated_index range of roots.
        #[test]
        fn get_shard_roots_is_dense_up_to_max_index(
            layout in arb_shard_layout(arb_char_str(), 2, 8, 4)
        ) {
            let mut tree = empty_tree::<String, 4, 2>();
            for shard in &layout.shards {
                tree.store.put_shard(shard.clone()).unwrap();
            }
            let max_index = layout.shards.iter().map(|s| s.root_addr.index()).max().unwrap();
            let expected: Vec<Address> = (0..=max_index)
                .map(|i| Address::from_parts(Level::from(2), i))
                .collect();
            prop_assert_eq!(tree.store.get_shard_roots().unwrap(), expected);
        }
    }

    #[test]
    fn insert_frontier_nodes() {
        let mut frontier = NonEmptyFrontier::new("a".to_string());
        for c in 'b'..'z' {
            frontier.append(c.to_string());
        }

        let root_addr = Address::from_parts(Level::from(4), 1);
        let tree = LocatedPrunableTree::empty(root_addr);
        let result = tree.insert_frontier_nodes::<()>(frontier.clone(), &Retention::Ephemeral);
        assert_matches!(result, Ok(_));

        let mut tree1 = LocatedPrunableTree::empty(root_addr);
        for c in 'q'..'z' {
            let (t, _, _) = tree1
                .append::<()>(c.to_string(), Retention::Ephemeral)
                .unwrap();
            tree1 = t;
        }
        assert_matches!(
            tree1.insert_frontier_nodes::<()>(frontier.clone(), &Retention::Ephemeral),
            Ok(t) if t == result.unwrap()
        );

        let mut tree2 = LocatedPrunableTree::empty(root_addr);
        for c in 'a'..'i' {
            let (t, _, _) = tree2
                .append::<()>(c.to_string(), Retention::Ephemeral)
                .unwrap();
            tree2 = t;
        }
        assert_matches!(
            tree2.insert_frontier_nodes::<()>(frontier, &Retention::Ephemeral),
            Err(InsertionError::Conflict(_))
        );
    }

    #[test]
    fn cached_parent_annotation_does_not_short_circuit_truncation() {
        // Regression test: the Parent handler's annotation fast-path in root_internal
        // must not return a cached (non-truncated) hash when truncation is required.
        //
        // The scenario:
        // 1. Fill all positions so the full root can be cached as a Leaf in the cap.
        // 2. root_caching with no truncation -> cap = Leaf(full_hash)
        // 3. root_caching with truncation -> Leaf handler's third branch splits the
        //    Leaf into Parent(None, Nil, Nil) and recurses. On return, reannotate_root
        //    sets the original Leaf value (full_hash) as the Parent annotation.
        //    Cap = Parent(Some(full_hash), Nil, Nil)
        // 4. root with truncation -> Parent handler's fast-path sees the annotation
        //    and incorrectly returns full_hash instead of recomputing with truncation.
        //
        // DEPTH=4, SHARD_HEIGHT=2: 4 shards of 4 positions each, 16 total positions.
        let mut tree = empty_tree::<String, 4, 2>();

        // Fill all 16 positions (shards 0-3).
        for (i, c) in ('a'..='p').enumerate() {
            tree.append(
                c.to_string(),
                Retention::Checkpoint {
                    id: (i + 1) as u32,
                    marking: Marking::None,
                },
            )
            .unwrap();
        }

        let root_addr = ShardTree::<MemoryShardStore<String, u32>, 4, 2>::root_addr();

        // Step 1: Cache the full root. The cap becomes Leaf("abcdefghijklmnop").
        let full_root = tree.root_caching(root_addr, Position::from(16)).unwrap();
        assert_eq!(full_root, "abcdefghijklmnop");

        // Step 2: Call root_caching with truncation at position 4. This correctly
        // returns the truncated root, but the Leaf handler's reannotate_root sets
        // the full root hash as the annotation on the replacement Parent node in
        // the cap: Parent(Some("abcdefghijklmnop"), Nil, Nil).
        let truncated_root = tree.root_caching(root_addr, Position::from(4)).unwrap();
        assert_eq!(truncated_root, "abcd____________");

        // Step 3: Call root again with the same truncation. The cap now has
        // Parent(Some("abcdefghijklmnop"), ...). The bug causes the fast-path to
        // return "abcdefghijklmnop" instead of recomputing with truncation.
        let truncated_root_again = tree.root(root_addr, Position::from(4)).unwrap();

        assert_eq!(
            truncated_root, truncated_root_again,
            "Truncated root must not be affected by cached annotation on Parent node"
        );
    }

    #[test]
    fn root_caching_does_not_corrupt_cap_with_sub_shard_nodes() {
        // Regression test: root_caching must not split cached shard-level Leaf nodes
        // into Parent nodes with children below the shard level. Such sub-shard Parent
        // nodes corrupt the cap, causing deserialization failures when the cap is later
        // read with a level-shifted root address (as zcash_client_sqlite does).
        //
        // DEPTH=4, SHARD_HEIGHT=2: 4 shards of 4 positions each.
        // The cap covers levels 2..4 (2 internal levels above the shard leaves).
        let mut tree = empty_tree::<String, 4, 2>();

        // Fill shard 0 (positions 0-3) and start shard 1 (position 4).
        tree.append(
            "a".to_string(),
            Retention::Checkpoint {
                id: 1,
                marking: Marking::None,
            },
        )
        .unwrap();
        tree.append(
            "b".to_string(),
            Retention::Checkpoint {
                id: 2,
                marking: Marking::None,
            },
        )
        .unwrap();
        tree.append(
            "c".to_string(),
            Retention::Checkpoint {
                id: 3,
                marking: Marking::None,
            },
        )
        .unwrap();
        tree.append(
            "d".to_string(),
            Retention::Checkpoint {
                id: 4,
                marking: Marking::None,
            },
        )
        .unwrap();
        tree.append(
            "e".to_string(),
            Retention::Checkpoint {
                id: 5,
                marking: Marking::None,
            },
        )
        .unwrap();

        // Insert shard 0's root hash into the cap at the shard level. This is what
        // put_subtree_roots does in zcash_client_sqlite: it inserts known subtree root
        // hashes so they can be used for witness/root computation without reading shard data.
        //
        // Shard 0 root = combine(1, combine(0, "a", "b"), combine(0, "c", "d")) = "abcd"
        tree.insert(Address::from_parts(Level::from(2), 0), "abcd".to_string())
            .unwrap();

        // Compute the root with truncation within shard 0. This triggers the bug:
        // the Leaf handler in root_internal splits the shard-level Leaf("abcd") into a
        // Parent with children at level 1 (below shard level 2).
        let _root = tree.root_caching(
            ShardTree::<MemoryShardStore<String, u32>, 4, 2>::root_addr(),
            Position::from(2), // truncate within shard 0
        );

        // Validate the cap's structural integrity. In zcash_client_sqlite, the cap is read
        // back and located at a level-shifted root address: (DEPTH - SHARD_HEIGHT, 0).
        // In the shifted coordinate system, shard-level nodes are at level 0, where Parent
        // nodes are forbidden (because level-0 addresses have no children).
        //
        // If root_caching split the shard-level Leaf, the cap will have a Parent at what
        // becomes level 0 in the shifted space, and from_parts will return Err.
        let cap = tree.store().get_cap().unwrap();
        let shifted_root_addr = Address::from_parts(Level::from(4 - 2), 0);
        assert!(
            LocatedTree::from_parts(shifted_root_addr, cap).is_ok(),
            "Cap must not contain Parent nodes at or below shard level"
        );
    }

    #[test]
    fn insert_frontier_nodes_sub_shard_height() {
        let mut frontier = NonEmptyFrontier::new("a".to_string());
        for c in 'b'..='c' {
            frontier.append(c.to_string());
        }

        let root_addr = Address::from_parts(Level::from(3), 0);
        let tree = LocatedPrunableTree::empty(root_addr);
        let result = tree.insert_frontier_nodes::<()>(frontier.clone(), &Retention::Ephemeral);
        assert_matches!(result, Ok((ref _t, None)));

        if let Ok((t, None)) = result {
            // verify that the leaf at the tip is included
            assert_eq!(
                t.root.root_hash(root_addr, Position::from(3)),
                Ok("abc_____".to_string())
            );
        }
    }

    // -----------------------------------------------------------------------
    // Regression tests for `root_internal`.
    //
    // These exercise every branch of the three-phase `root_internal` flow and
    // its `root_from_shards` delegate, by calling `root_internal` directly with
    // a hand-built cap node, a target address, and a `truncate_at` bound chosen
    // to hit a specific branch. Each test compares a hand-written
    // `expected_root_hash` against the `computed_root_hash` returned by the
    // method.
    //
    // Fixture: `DEPTH = 4`, `SHARD_HEIGHT = 2`. With the `String` test hash,
    // `combine` is concatenation and `empty_root(level n)` is `2^n`
    // underscores: `_`, `__`, `____`, `________`, then 16 for level 4. Shards
    // are rooted at level 2 (4 leaves each); the cap spans levels 2..=4.
    // -----------------------------------------------------------------------
    mod root_internal {
        use std::sync::Arc;

        use assert_matches::assert_matches;
        use incrementalmerkletree::{Address, Hashable, Level, Position};
        use incrementalmerkletree_testing::SipHashable;

        use crate::{
            error::{QueryError, ShardTreeError},
            store::{memory::MemoryShardStore, ShardStore},
            LocatedPrunableTree, LocatedTree, PrunableTree, RetentionFlags, ShardTree, Tree,
        };

        use super::empty_tree;

        type TestTree = ShardTree<MemoryShardStore<String, u32>, 4, 2>;

        fn addr(level: u8, index: u64) -> Address {
            Address::from_parts(Level::from(level), index)
        }

        // A `String` leaf with the given content and EPHEMERAL retention.
        fn pleaf(value: &str) -> PrunableTree<String> {
            Tree::leaf((value.to_string(), RetentionFlags::EPHEMERAL))
        }

        // A `String` parent with an optional cached-hash annotation.
        fn pparent(
            ann: Option<&str>,
            left: PrunableTree<String>,
            right: PrunableTree<String>,
        ) -> PrunableTree<String> {
            Tree::parent(ann.map(|s| Arc::new(s.to_string())), left, right)
        }

        fn pnil() -> PrunableTree<String> {
            Tree::empty()
        }

        // A located cap node: the tree paired with its absolute address.
        fn cap_at(
            level: u8,
            index: u64,
            root: PrunableTree<String>,
        ) -> LocatedPrunableTree<String> {
            LocatedTree {
                root_addr: addr(level, index),
                root,
            }
        }

        // A complete shard at level 2 (root address `(2, index)`) holding the
        // four given leaves. Its root hash is the concatenation `abcd`.
        fn shard(index: u64, a: &str, b: &str, c: &str, d: &str) -> LocatedPrunableTree<String> {
            LocatedTree {
                root_addr: addr(2, index),
                root: pparent(
                    None,
                    pparent(None, pleaf(a), pleaf(b)),
                    pparent(None, pleaf(c), pleaf(d)),
                ),
            }
        }

        fn tree_with_shards(shards: &[LocatedPrunableTree<String>]) -> TestTree {
            let mut tree = empty_tree::<String, 4, 2>();
            for s in shards {
                tree.store.put_shard(s.clone()).unwrap();
            }
            tree
        }

        // ---- Phase 1: fast path (cached value returned immediately) ---------

        #[test]
        fn fast_path_parent_cached() {
            // A Parent carrying a cached annotation, queried at its own address
            // with no truncation: the annotation is returned without touching
            // any shard data (the store is empty on purpose).
            let tree = empty_tree::<String, 4, 2>();
            let cap = cap_at(4, 0, pparent(Some("abcdefghijklmnop"), pnil(), pnil()));

            let expected_root_hash = "abcdefghijklmnop".to_string();
            let target_addr = addr(4, 0);
            let truncate_at = Position::from(16);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // The fast path serves an existing cached value; it never produces
            // a write-back node.
            assert!(updated_cap.is_none());
        }

        #[test]
        fn fast_path_leaf_cached() {
            // A Leaf node carries its hash directly; querying it at its own
            // address with no truncation returns that hash via the fast path.
            let tree = empty_tree::<String, 4, 2>();
            let cap = cap_at(2, 0, pleaf("abcd"));

            let expected_root_hash = "abcd".to_string();
            let target_addr = addr(2, 0);
            let truncate_at = Position::from(4);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // The fast path serves an existing cached value; it never produces
            // a write-back node.
            assert!(updated_cap.is_none());
        }

        // ---- Phase 2: base case via `root_from_shards` ----------------------

        #[test]
        fn base_shard_level_single_present() {
            // Trigger C: the cap node sits at the shard level, so the root is
            // read directly from the single backing shard.
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(2, 0, pnil());

            let expected_root_hash = "abcd".to_string();
            let target_addr = addr(2, 0);
            let truncate_at = Position::from(4);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // The result is untruncated, so it is written back as a cached leaf.
            assert_eq!(updated_cap, Some(pleaf(&expected_root_hash)));
        }

        #[test]
        fn base_shard_truncated_to_empty() {
            // `truncate_at` at the start of the shard's range makes the whole
            // shard empty, so the empty root for level 2 is returned.
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(2, 0, pnil());

            let expected_root_hash = "____".to_string();
            let target_addr = addr(2, 0);
            let truncate_at = Position::from(0);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // A truncated root must never be cached.
            assert!(updated_cap.is_none());
        }

        #[test]
        fn base_shard_missing_errors() {
            // The requested shard was never stored: report it incomplete.
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(2, 1, pnil());

            let target_addr = addr(2, 1);
            let truncate_at = Position::from(8);
            let err = tree
                .root_internal(&cap, target_addr, truncate_at)
                .unwrap_err();

            assert_matches!(
                err,
                ShardTreeError::Query(QueryError::TreeIncomplete(addrs))
                    if addrs == vec![addr(2, 1)]
            );
        }

        #[test]
        fn base_shard_node_not_retained_errors() {
            // The shard exists but a node required for the root was pruned
            // away (its left child is Nil): report that node's address.
            let pruned = LocatedTree {
                root_addr: addr(2, 2),
                root: pparent(None, pnil(), pparent(None, pleaf("k"), pleaf("l"))),
            };
            let tree = tree_with_shards(&[pruned]);
            let cap = cap_at(2, 2, pnil());

            let target_addr = addr(2, 2);
            let truncate_at = Position::from(12);
            let err = tree
                .root_internal(&cap, target_addr, truncate_at)
                .unwrap_err();

            assert_matches!(
                err,
                ShardTreeError::Query(QueryError::TreeIncomplete(addrs))
                    if addrs == vec![addr(1, 4)]
            );
        }

        #[test]
        fn base_target_nil_above_shard() {
            // Trigger D: a non-Parent (Nil) cap node above the shard level,
            // queried at its own address, folds the shards it spans.
            let shard0 = shard(0, "a", "b", "c", "d");
            let shard1 = shard(1, "e", "f", "g", "h");
            let tree = tree_with_shards(&[shard0, shard1]);
            let cap = cap_at(3, 0, pnil());

            let expected_root_hash = "abcdefgh".to_string();
            let target_addr = addr(3, 0);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // The result is untruncated, so it is written back as a cached leaf.
            assert_eq!(updated_cap, Some(pleaf(&expected_root_hash)));
        }

        #[test]
        fn base_shard_level_leaf_truncated() {
            // A cached shard-level Leaf, queried with truncation, must not return
            // its cached hash: the base case recomputes the (truncated) root from
            // the backing shard and does not re-cache it. This is the shard-level
            // Leaf entry into Phase 2, which the `base_shard_*` tests reach only
            // with a Nil node.
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(2, 0, pleaf("abcd"));

            // truncate_at = 2 keeps positions 0,1 and empties 2,3.
            let expected_root_hash = "ab__".to_string();
            let target_addr = addr(2, 0);
            let truncate_at = Position::from(2);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // A truncated root must never be cached.
            assert!(updated_cap.is_none());
        }

        #[test]
        fn base_leaf_at_target_truncated() {
            // A Leaf above the shard level, queried at its own address with
            // truncation. Pre-#143 this is the Leaf handler's
            // recurse-and-reannotate branch. #143 reproduces that branch by
            // excluding a Leaf-at-target from the Phase 2 base case (which is
            // restricted to Nil-at-target), so the Phase 3 descent expands the
            // Leaf, computes the truncated root, and reannotates the resulting
            // Parent with the original leaf value.
            //
            // The reannotated annotation is the original un-truncated hash
            // ("abcdefgh"), never the truncated root, so the write-back stays a
            // valid cache entry: a later non-truncated lookup served from it via
            // the Phase 1 fast path returns the correct hash.
            let shard0 = shard(0, "a", "b", "c", "d");
            let shard1 = shard(1, "e", "f", "g", "h");
            let tree = tree_with_shards(&[shard0, shard1]);
            let cap = cap_at(3, 0, pleaf("abcdefgh"));

            // truncate_at = 4 keeps shard 0 and empties shard 1.
            let expected_root_hash = "abcd____".to_string();
            let target_addr = addr(3, 0);
            let truncate_at = Position::from(4);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // The expanded Parent keeps shard 0's cached leaf, an empty right
            // child, and the original leaf hash as its annotation.
            assert_eq!(
                updated_cap,
                Some(pparent(Some("abcdefgh"), pleaf("abcd"), pnil()))
            );
        }

        #[test]
        fn base_target_below_shard_level() {
            // A target address strictly below the shard level: the base case uses
            // `target_addr` (not `cap.root_addr`) for the shard lookup, and
            // `root_from_shards` resolves it via the proper-descendant path
            // `subtree.subtree(address)` rather than the whole-subtree root.
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(2, 0, pnil());

            // (1, 0) spans positions 0,1; its root is `combine(0, "a", "b")`.
            let expected_root_hash = "ab".to_string();
            let target_addr = addr(1, 0);
            let truncate_at = Position::from(4);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // Untruncated, so the sub-shard root is written back as a cached leaf.
            assert_eq!(updated_cap, Some(pleaf(&expected_root_hash)));
        }

        // ---- Phase 2: `root_from_shards` multi-shard peak fold --------------

        #[test]
        fn shards_fold_two_present() {
            let shard0 = shard(0, "a", "b", "c", "d");
            let shard1 = shard(1, "e", "f", "g", "h");
            let tree = tree_with_shards(&[shard0, shard1]);
            let cap = cap_at(3, 0, pnil());

            let expected_root_hash = "abcdefgh".to_string();
            let target_addr = addr(3, 0);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // Untruncated, so the folded root is written back as a cached leaf.
            assert_eq!(updated_cap, Some(pleaf(&expected_root_hash)));
        }

        #[test]
        fn shards_fold_with_truncation() {
            // The left half (shards 0,1) is present; everything from position 8
            // on is truncated, so the right peak is padded with `empty_root(3)`.
            let shard0 = shard(0, "a", "b", "c", "d");
            let shard1 = shard(1, "e", "f", "g", "h");
            let shard2 = shard(2, "i", "j", "k", "l");
            let shard3 = shard(3, "m", "n", "o", "p");
            let tree = tree_with_shards(&[shard0, shard1, shard2, shard3]);
            let cap = cap_at(4, 0, pnil());

            let expected_root_hash = "abcdefgh________".to_string();
            let target_addr = addr(4, 0);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // A truncated root must never be cached.
            assert!(updated_cap.is_none());
        }

        #[test]
        fn shards_fold_unaligned_present_count() {
            // Three shards present, the fourth truncated away. The peak stack
            // holds two entries at different levels ((3,0) and (2,2)), so the
            // final fold runs its inner `while addr.level() > cur_addr.level()`
            // loop to pad the lone shard 2 up to level 3 with an empty sibling
            // before combining. No single- or empty-stack fold reaches that loop.
            let shard0 = shard(0, "a", "b", "c", "d");
            let shard1 = shard(1, "e", "f", "g", "h");
            let shard2 = shard(2, "i", "j", "k", "l");
            let tree = tree_with_shards(&[shard0, shard1, shard2]);
            let cap = cap_at(4, 0, pnil());

            // shards 0,1 fold to "abcdefgh"; shard 2 is padded to "ijkl____";
            // shard 3 (positions 12..16) is truncated.
            let expected_root_hash = "abcdefghijkl____".to_string();
            let target_addr = addr(4, 0);
            let truncate_at = Position::from(12);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // A truncated root must never be cached.
            assert!(updated_cap.is_none());
        }

        #[test]
        fn shards_fold_missing_errors() {
            // A shard required for the multi-shard fold is missing.
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(3, 0, pnil());

            let target_addr = addr(3, 0);
            let truncate_at = Position::from(8);
            let err = tree
                .root_internal(&cap, target_addr, truncate_at)
                .unwrap_err();

            assert_matches!(
                err,
                ShardTreeError::Query(QueryError::TreeIncomplete(addrs))
                    if addrs == vec![addr(2, 1)]
            );
        }

        #[test]
        fn shards_fold_multiple_missing_errors() {
            // Several shards required for the multi-shard fold are missing. The
            // incomplete addresses accumulate (`incomplete.append`) and are all
            // reported together, not just the first one.
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(4, 0, pnil());

            let target_addr = addr(4, 0);
            let truncate_at = Position::from(16);
            let err = tree
                .root_internal(&cap, target_addr, truncate_at)
                .unwrap_err();

            assert_matches!(
                err,
                ShardTreeError::Query(QueryError::TreeIncomplete(addrs))
                    if addrs == vec![addr(2, 1), addr(2, 2), addr(2, 3)]
            );
        }

        #[test]
        fn full_truncation_empty_root() {
            // Truncating at position 0 empties the whole tree; the empty-stack
            // branch of the fold returns `empty_root(4)` (16 underscores).
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(4, 0, pnil());

            let expected_root_hash = "________________".to_string();
            let target_addr = addr(4, 0);
            let truncate_at = Position::from(0);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // A truncated root must never be cached.
            assert!(updated_cap.is_none());
        }

        // ---- Phase 3: descent (recurse into children, combine) --------------

        #[test]
        fn descent_parent_both_children() {
            // Querying a Parent at its own address recurses into both children
            // (each a cached leaf) and combines them: the `(Some, Some)` arm.
            let tree = empty_tree::<String, 4, 2>();
            let cap = cap_at(3, 0, pparent(None, pleaf("abcd"), pleaf("efgh")));

            let expected_root_hash = "abcdefgh".to_string();
            let target_addr = addr(3, 0);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // Both children hit the fast path and produce no write-back, so the
            // rebuilt Parent keeps the original (cached) children and stays
            // unannotated.
            assert_eq!(
                updated_cap,
                Some(pparent(None, pleaf("abcd"), pleaf("efgh")))
            );
        }

        #[test]
        fn descent_target_in_left_child() {
            // Target is inside the left child only: the right subtree is
            // skipped and the left root is returned (the `(Some, None)` arm).
            let tree = empty_tree::<String, 4, 2>();
            let cap = cap_at(3, 0, pparent(None, pleaf("abcd"), pleaf("efgh")));

            let expected_root_hash = "abcd".to_string();
            let target_addr = addr(2, 0);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // The right subtree is skipped; the rebuilt Parent keeps both
            // original children unchanged and stays unannotated.
            assert_eq!(
                updated_cap,
                Some(pparent(None, pleaf("abcd"), pleaf("efgh")))
            );
        }

        #[test]
        fn descent_target_in_right_child() {
            // Target is inside the right child only: the `(None, Some)` arm.
            let tree = empty_tree::<String, 4, 2>();
            let cap = cap_at(3, 0, pparent(None, pleaf("abcd"), pleaf("efgh")));

            let expected_root_hash = "efgh".to_string();
            let target_addr = addr(2, 1);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // The left subtree is skipped; the rebuilt Parent keeps both
            // original children unchanged and stays unannotated.
            assert_eq!(
                updated_cap,
                Some(pparent(None, pleaf("abcd"), pleaf("efgh")))
            );
        }

        #[test]
        fn descent_expand_cached_leaf() {
            // A cached Leaf above the shard level, queried below itself, is
            // expanded: `orig_leaf_value` is `Some`, so the rebuilt Parent is
            // re-annotated with the original leaf hash for future fast-paths.
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(3, 0, pleaf("abcdefgh"));

            let expected_root_hash = "abcd".to_string();
            let target_addr = addr(2, 0);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // The rebuilt node retains the original leaf hash as its annotation,
            // with shard 0's cached leaf on the left and an empty right child.
            assert_eq!(
                updated_cap,
                Some(pparent(Some("abcdefgh"), pleaf("abcd"), pnil()))
            );
        }

        #[test]
        fn descent_through_nil() {
            // A Nil cap node above the shard level, queried below itself,
            // descends into empty children down to the backing shard.
            let shard0 = shard(0, "a", "b", "c", "d");
            let shard1 = shard(1, "e", "f", "g", "h");
            let tree = tree_with_shards(&[shard0, shard1]);
            let cap = cap_at(3, 0, pnil());

            let expected_root_hash = "efgh".to_string();
            let target_addr = addr(2, 1);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // The left subtree is skipped (empty); the right child becomes
            // shard 1's cached leaf. No annotation (left child not cacheable).
            assert_eq!(
                updated_cap,
                Some(pparent(None, pnil(), pleaf(&expected_root_hash)))
            );
        }

        // ---- Annotation propagation and caching write-back ------------------

        #[test]
        fn descent_caches_annotation_when_untruncated() {
            // When both children are recomputed from shards (not short-circuited
            // by the fast path) and neither is truncated, each returns a cached
            // leaf, so the rebuilt Parent gets a cached annotation equal to the
            // combined hash. (Note: already-cached Leaf children hit the fast
            // path, which returns no write-back node, so the parent would NOT
            // be annotated in that case; see `descent_parent_both_children`.)
            let shard0 = shard(0, "a", "b", "c", "d");
            let shard1 = shard(1, "e", "f", "g", "h");
            let tree = tree_with_shards(&[shard0, shard1]);
            let cap = cap_at(3, 0, pparent(None, pnil(), pnil()));

            let target_addr = addr(3, 0);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, "abcdefgh".to_string());
            // Both children are recomputed into cached leaves, so the rebuilt
            // Parent is annotated with the combined hash.
            assert_eq!(
                updated_cap,
                Some(pparent(Some("abcdefgh"), pleaf("abcd"), pleaf("efgh")))
            );
        }

        #[test]
        fn descent_no_annotation_when_child_truncated() {
            // Left child is fully present (and cached), the right child is
            // truncated to empty. The hash still combines, but the rebuilt
            // Parent carries no annotation because the right child was not
            // cacheable.
            let shard0 = shard(0, "a", "b", "c", "d");
            let tree = tree_with_shards(&[shard0]);
            let cap = cap_at(3, 0, pparent(None, pnil(), pnil()));

            let expected_root_hash = "abcd____".to_string();
            let target_addr = addr(3, 0);
            let truncate_at = Position::from(4);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // Left child becomes a cached leaf; the truncated right child yields
            // no write-back, so the rebuilt Parent keeps the empty right child
            // and carries no annotation.
            assert_eq!(updated_cap, Some(pparent(None, pleaf("abcd"), pnil())));
        }

        #[test]
        fn caching_then_fast_path() {
            // First compute folds the shards and returns a cacheable rebuilt
            // node; feeding that node back lets a second query answer from the
            // cache via the fast path, even after the shards are dropped.
            let shard0 = shard(0, "a", "b", "c", "d");
            let shard1 = shard(1, "e", "f", "g", "h");
            let mut tree = tree_with_shards(&[shard0, shard1]);
            let cap = cap_at(3, 0, pnil());

            let target_addr = addr(3, 0);
            let truncate_at = Position::from(8);
            let (first_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();
            assert_eq!(first_root_hash, "abcdefgh".to_string());
            // The fold writes the untruncated root back as a cached leaf.
            assert_eq!(updated_cap, Some(pleaf("abcdefgh")));
            let updated_cap = updated_cap.expect("a rebuilt node is returned");

            // Drop all shard data, then answer the same query from the rebuilt
            // (cached) node, reusing `target_addr` and `truncate_at`.
            tree.store.truncate_shards(0).unwrap();
            let cached_cap = cap_at(3, 0, updated_cap);
            let (computed_root_hash, _) = tree
                .root_internal(&cached_cap, target_addr, truncate_at)
                .unwrap();

            assert_eq!(computed_root_hash, "abcdefgh".to_string());
        }

        // ---- Sanity check with a non-concatenating hash ---------------------

        #[test]
        fn descent_parent_both_children_siphash() {
            // Same descent as `descent_parent_both_children`, but with the
            // SipHash test hash, to confirm the result is not an artifact of
            // string concatenation. `expected_root_hash` is built directly from
            // the hash primitive; `computed_root_hash` comes from the method.
            let tree = empty_tree::<SipHashable, 4, 2>();

            let left = SipHashable(0xAA);
            let right = SipHashable(0xBB);
            let cap = LocatedTree {
                root_addr: addr(3, 0),
                root: Tree::parent(
                    None,
                    Tree::leaf((left.clone(), RetentionFlags::EPHEMERAL)),
                    Tree::leaf((right.clone(), RetentionFlags::EPHEMERAL)),
                ),
            };

            // Children sit at level 2; the combine happens at that level.
            let expected_root_hash = SipHashable::combine(Level::from(2), &left, &right);
            let target_addr = addr(3, 0);
            let truncate_at = Position::from(8);
            let (computed_root_hash, updated_cap) =
                tree.root_internal(&cap, target_addr, truncate_at).unwrap();

            assert_eq!(computed_root_hash, expected_root_hash);
            // Both children hit the fast path; the rebuilt Parent keeps the
            // original cached leaves and stays unannotated.
            assert_eq!(
                updated_cap,
                Some(Tree::parent(
                    None,
                    Tree::leaf((left, RetentionFlags::EPHEMERAL)),
                    Tree::leaf((right, RetentionFlags::EPHEMERAL)),
                ))
            );
        }
    }
}
