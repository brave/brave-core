//! `shardtree` is a space-efficient fixed-depth Merkle tree structure that is densely
//! filled from the left. It supports:
//!
//! - *Out-of-order insertion*: leaves and nodes may be inserted into the tree in
//!   arbitrary order. The structure will keep track of the right-most filled position as
//!   the frontier of the tree; any unfilled leaves to the left of this position are
//!   considered "missing", while any unfilled leaves to the right of this position are
//!   considered "empty".
//! - *Witnessing*: Individual leaves of the Merkle tree may be marked such that witnesses
//!   will be maintained for the marked leaves as additional nodes are inserted into the
//!   tree, but leaf and node data not specifically required to maintain these witnesses
//!   is not retained, for space efficiency.
//! - *Checkpointing*: the tree may be reset to a previously checkpointed state, up to a
//!   fixed number of checkpoints.
//!
//! Due to its structure (described in the [`store`] module), witnesses for marked leaves
//! can be advanced up to recent checkpoints or the latest state of the tree, without
//! having to insert each intermediate leaf individually. Instead, only the roots of all
//! complete shards between the one containing the marked leaf and the tree frontier need
//! to be inserted, along with the necessary nodes to build a path from the marked leaf to
//! the root of the shard containing it.

#![cfg_attr(docsrs, feature(doc_cfg))]

use core::fmt::Debug;
use either::Either;
use incrementalmerkletree::frontier::Frontier;
use std::collections::{BTreeMap, BTreeSet};
use std::sync::Arc;
use tracing::trace;

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

#[cfg(any(bench, test, feature = "test-dependencies"))]
pub mod testing;

#[cfg(feature = "legacy-api")]
#[cfg_attr(docsrs, doc(cfg(feature = "legacy-api")))]
mod legacy;

/// A sparse binary Merkle tree of the specified depth, represented as an ordered collection of
/// subtrees (shards) of a given maximum height.
///
/// This tree maintains a collection of "checkpoints" which represent positions, usually near the
/// front of the tree, that are maintained such that it's possible to truncate nodes to the right
/// of the specified position.
#[derive(Debug)]
pub struct ShardTree<S: ShardStore, const DEPTH: u8, const SHARD_HEIGHT: u8> {
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

    /// Append a single value at the first available position in the tree.
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
                if subtree.root.is_complete() {
                    let addr = subtree.root_addr;

                    if addr.index() < Self::max_subtree_index() {
                        LocatedTree::empty(addr.next_at_level()).append(value, retention)?
                    } else {
                        return Err(InsertionError::TreeFull.into());
                    }
                } else {
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
    pub fn insert_frontier(
        &mut self,
        frontier: Frontier<H, DEPTH>,
        leaf_retention: Retention<C>,
    ) -> Result<(), ShardTreeError<S::Error>> {
        if let Some(nonempty_frontier) = frontier.take() {
            self.insert_frontier_nodes(nonempty_frontier, leaf_retention)
        } else {
            match leaf_retention {
                Retention::Ephemeral => Ok(()),
                Retention::Checkpoint {
                    id,
                    is_marked: false,
                } => self
                    .store
                    .add_checkpoint(id, Checkpoint::tree_empty())
                    .map_err(ShardTreeError::Storage),
                Retention::Checkpoint {
                    is_marked: true, ..
                }
                | Retention::Marked => Err(ShardTreeError::Insert(
                    InsertionError::MarkedRetentionInvalid,
                )),
            }
        }
    }

    /// Add the leaf and ommers of the provided frontier as nodes within the subtree corresponding
    /// to the frontier's position, and update the cap to include the ommer nodes at levels greater
    /// than or equal to the shard height.
    pub fn insert_frontier_nodes(
        &mut self,
        frontier: NonEmptyFrontier<H>,
        leaf_retention: Retention<C>,
    ) -> Result<(), ShardTreeError<S::Error>> {
        let leaf_position = frontier.position();
        let subtree_root_addr = Address::above_position(Self::subtree_level(), leaf_position);
        trace!("Subtree containing nodes: {:?}", subtree_root_addr);

        let (updated_subtree, supertree) = self
            .store
            .get_shard(subtree_root_addr)
            .map_err(ShardTreeError::Storage)?
            .unwrap_or_else(|| LocatedTree::empty(subtree_root_addr))
            .insert_frontier_nodes(frontier, &leaf_retention)?;

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

        if let Retention::Checkpoint { id, is_marked: _ } = leaf_retention {
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
                continue;
            }

            // `LocatedTree::decompose_to_level` will return the tree as-is if it is
            // smaller than a shard, so we can't assume that the address of `subtree` is a
            // valid shard address.
            let root_addr = Self::subtree_addr(subtree.root_addr.position_range_start());

            let contains_marked = subtree.root.contains_marked();
            let (new_subtree, mut incomplete) = self
                .store
                .get_shard(root_addr)
                .map_err(ShardTreeError::Storage)?
                .unwrap_or_else(|| LocatedTree::empty(root_addr))
                .insert_subtree(subtree, contains_marked)?;
            self.store
                .put_shard(new_subtree)
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
    pub fn checkpoint(&mut self, checkpoint_id: C) -> Result<bool, ShardTreeError<S::Error>> {
        fn go<H: Hashable + Clone + PartialEq>(
            root_addr: Address,
            root: &PrunableTree<H>,
        ) -> Option<(PrunableTree<H>, Position)> {
            match root {
                Tree(Node::Parent { ann, left, right }) => {
                    let (l_addr, r_addr) = root_addr.children().unwrap();
                    go(r_addr, right).map_or_else(
                        || {
                            go(l_addr, left).map(|(new_left, pos)| {
                                (
                                    Tree::unite(
                                        l_addr.level(),
                                        ann.clone(),
                                        new_left,
                                        Tree(Node::Nil),
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
                Tree(Node::Leaf { value: (h, r) }) => Some((
                    Tree(Node::Leaf {
                        value: (h.clone(), *r | RetentionFlags::CHECKPOINT),
                    }),
                    root_addr.max_position(),
                )),
                Tree(Node::Nil) => None,
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

    fn prune_excess_checkpoints(&mut self) -> Result<(), ShardTreeError<S::Error>> {
        let checkpoint_count = self
            .store
            .checkpoint_count()
            .map_err(ShardTreeError::Storage)?;
        trace!(
            "Tree has {} checkpoints, max is {}",
            checkpoint_count,
            self.max_checkpoints,
        );
        if checkpoint_count > self.max_checkpoints {
            // Batch removals by subtree & create a list of the checkpoint identifiers that
            // will be removed from the checkpoints map.
            let remove_count = checkpoint_count - self.max_checkpoints;
            let mut checkpoints_to_delete = vec![];
            let mut clear_positions: BTreeMap<Address, BTreeMap<Position, RetentionFlags>> =
                BTreeMap::new();
            self.store
                .with_checkpoints(checkpoint_count, |cid, checkpoint| {
                    // When removing is true, we are iterating through the range of
                    // checkpoints being removed. When remove is false, we are
                    // iterating through the range of checkpoints that are being
                    // retained.
                    let removing = checkpoints_to_delete.len() < remove_count;

                    if removing {
                        checkpoints_to_delete.push(cid.clone());
                    }

                    let mut clear_at = |pos, flags_to_clear| {
                        let subtree_addr = Self::subtree_addr(pos);
                        if removing {
                            // Mark flags to be cleared from the given position.
                            clear_positions
                                .entry(subtree_addr)
                                .and_modify(|to_clear| {
                                    to_clear
                                        .entry(pos)
                                        .and_modify(|flags| *flags |= flags_to_clear)
                                        .or_insert(flags_to_clear);
                                })
                                .or_insert_with(|| BTreeMap::from([(pos, flags_to_clear)]));
                        } else {
                            // Unmark flags that might have been marked for clearing above
                            // but which we now know we need to preserve.
                            if let Some(to_clear) = clear_positions.get_mut(&subtree_addr) {
                                if let Some(flags) = to_clear.get_mut(&pos) {
                                    *flags &= !flags_to_clear;
                                }
                            }
                        }
                    };

                    // Clear or preserve the checkpoint leaf.
                    if let TreeState::AtPosition(pos) = checkpoint.tree_state() {
                        clear_at(pos, RetentionFlags::CHECKPOINT)
                    }

                    // Clear or preserve the leaves that have been marked for removal.
                    for unmark_pos in checkpoint.marks_removed().iter() {
                        clear_at(*unmark_pos, RetentionFlags::MARKED)
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
                let cleared = self
                    .store
                    .get_shard(subtree_addr)
                    .map_err(ShardTreeError::Storage)?
                    .map(|subtree| subtree.clear_flags(positions));
                if let Some(cleared) = cleared {
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

    /// Truncates the tree, discarding all information after the checkpoint at the specified depth.
    ///
    /// This will also discard all checkpoints with depth less than or equal to the specified depth.
    /// Returns `true` if the truncation succeeds or has no effect, or `false` if no checkpoint
    /// exists at the specified depth.
    pub fn truncate_to_depth(
        &mut self,
        checkpoint_depth: usize,
    ) -> Result<bool, ShardTreeError<S::Error>> {
        if checkpoint_depth == 0 {
            Ok(true)
        } else if let Some((checkpoint_id, c)) = self
            .store
            .get_checkpoint_at_depth(checkpoint_depth)
            .map_err(ShardTreeError::Storage)?
        {
            self.truncate_removing_checkpoint_internal(&checkpoint_id, &c)?;
            Ok(true)
        } else {
            Ok(false)
        }
    }

    /// Truncates the tree, discarding all information after the specified checkpoint.
    ///
    /// This will also discard all checkpoints with depth less than or equal to the specified depth.
    /// Returns `true` if the truncation succeeds or has no effect, or `false` if no checkpoint
    /// exists for the specified checkpoint identifier.
    pub fn truncate_removing_checkpoint(
        &mut self,
        checkpoint_id: &C,
    ) -> Result<bool, ShardTreeError<S::Error>> {
        if let Some(c) = self
            .store
            .get_checkpoint(checkpoint_id)
            .map_err(ShardTreeError::Storage)?
        {
            self.truncate_removing_checkpoint_internal(checkpoint_id, &c)?;
            Ok(true)
        } else {
            Ok(false)
        }
    }

    fn truncate_removing_checkpoint_internal(
        &mut self,
        checkpoint_id: &C,
        checkpoint: &Checkpoint,
    ) -> Result<(), ShardTreeError<S::Error>> {
        match checkpoint.tree_state() {
            TreeState::Empty => {
                self.store
                    .truncate(Address::from_parts(Self::subtree_level(), 0))
                    .map_err(ShardTreeError::Storage)?;
                self.store
                    .truncate_checkpoints(checkpoint_id)
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

                if let Some(truncated) = cap_tree.truncate_to_position(position) {
                    self.store
                        .put_cap(truncated.root)
                        .map_err(ShardTreeError::Storage)?;
                };

                if let Some(truncated) = replacement {
                    self.store
                        .truncate(subtree_addr)
                        .map_err(ShardTreeError::Storage)?;
                    self.store
                        .put_shard(truncated)
                        .map_err(ShardTreeError::Storage)?;
                    self.store
                        .truncate_checkpoints(checkpoint_id)
                        .map_err(ShardTreeError::Storage)?;
                }
            }
        }

        Ok(())
    }

    /// Computes the root of any subtree of this tree rooted at the given address, with the overall
    /// tree truncated to the specified position.
    ///
    /// The specified address is not required to be at any particular level, though it cannot
    /// exceed the level corresponding to the maximum depth of the tree. Nodes to the right of the
    /// given position, and parents of such nodes, will be replaced by the empty root for the
    /// associated level.
    ///
    /// Use [`Self::root_at_checkpoint_depth`] to obtain the root of the overall tree.
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

    // compute the root, along with an optional update to the cap
    #[allow(clippy::type_complexity)]
    fn root_internal(
        &self,
        cap: &LocatedPrunableTree<S::H>,
        // The address at which we want to compute the root hash
        target_addr: Address,
        // An inclusive lower bound for positions whose leaf values will be replaced by empty
        // roots.
        truncate_at: Position,
    ) -> Result<(H, Option<PrunableTree<H>>), ShardTreeError<S::Error>> {
        match &cap.root {
            Tree(Node::Parent { ann, left, right }) => {
                match ann {
                    Some(cached_root) if target_addr.contains(&cap.root_addr) => {
                        Ok((cached_root.as_ref().clone(), None))
                    }
                    _ => {
                        // Compute the roots of the left and right children and hash them together.
                        // We skip computation in any subtrees that will not have data included in
                        // the final result.
                        let (l_addr, r_addr) = cap.root_addr.children().unwrap();
                        let l_result = if r_addr.contains(&target_addr) {
                            None
                        } else {
                            Some(self.root_internal(
                                &LocatedPrunableTree {
                                    root_addr: l_addr,
                                    root: left.as_ref().clone(),
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
                                    root: right.as_ref().clone(),
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

                        let new_parent = Tree(Node::Parent {
                            ann: new_left
                                .as_ref()
                                .and_then(|l| l.node_value())
                                .zip(new_right.as_ref().and_then(|r| r.node_value()))
                                .map(|(l, r)| {
                                    // the node values of child nodes cannot contain the hashes of
                                    // empty nodes or nodes with positions greater than the
                                    Arc::new(S::H::combine(l_addr.level(), l, r))
                                }),
                            left: new_left.map_or_else(|| left.clone(), Arc::new),
                            right: new_right.map_or_else(|| right.clone(), Arc::new),
                        });

                        Ok((root, Some(new_parent)))
                    }
                }
            }
            Tree(Node::Leaf { value }) => {
                if truncate_at >= cap.root_addr.position_range_end()
                    && target_addr.contains(&cap.root_addr)
                {
                    // no truncation or computation of child subtrees of this leaf is necessary, just use
                    // the cached leaf value
                    Ok((value.0.clone(), None))
                } else {
                    // since the tree was truncated below this level, recursively call with an
                    // empty parent node to trigger the continued traversal
                    let (root, replacement) = self.root_internal(
                        &LocatedPrunableTree {
                            root_addr: cap.root_addr(),
                            root: Tree::parent(None, Tree::empty(), Tree::empty()),
                        },
                        target_addr,
                        truncate_at,
                    )?;

                    Ok((
                        root,
                        replacement.map(|r| r.reannotate_root(Some(Arc::new(value.0.clone())))),
                    ))
                }
            }
            Tree(Node::Nil) => {
                if cap.root_addr == target_addr
                    || cap.root_addr.level() == ShardTree::<S, DEPTH, SHARD_HEIGHT>::subtree_level()
                {
                    // We are at the leaf level or the target address; compute the root hash and
                    // return it as cacheable if it is not truncated.
                    let root = self.root_from_shards(target_addr, truncate_at)?;
                    Ok((
                        root.clone(),
                        if truncate_at >= cap.root_addr.position_range_end() {
                            // return the compute root as a new leaf to be cached if it contains no
                            // empty hashes due to truncation
                            Some(Tree::leaf((root, RetentionFlags::EPHEMERAL)))
                        } else {
                            None
                        },
                    ))
                } else {
                    // Compute the result by recursively walking down the tree. By replacing
                    // the current node with a parent node, the `Parent` handler will take care
                    // of the branching recursive calls.
                    self.root_internal(
                        &LocatedPrunableTree {
                            root_addr: cap.root_addr,
                            root: Tree::parent(None, Tree::empty(), Tree::empty()),
                        },
                        target_addr,
                        truncate_at,
                    )
                }
            }
        }
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

    /// Returns the position of the rightmost leaf inserted as of the given checkpoint.
    ///
    /// Returns the maximum leaf position if `checkpoint_depth == 0` (or `Ok(None)` in this
    /// case if the tree is empty) or an error if the checkpointed position cannot be restored
    /// because it has been pruned. Note that no actual level-0 leaf may exist at this position.
    pub fn max_leaf_position(
        &self,
        checkpoint_depth: usize,
    ) -> Result<Option<Position>, ShardTreeError<S::Error>> {
        Ok(if checkpoint_depth == 0 {
            // TODO: This relies on the invariant that the last shard in the subtrees vector is
            // never created without a leaf then being added to it. However, this may be a
            // difficult invariant to maintain when adding empty roots, so perhaps we need a
            // better way of tracking the actual max position of the tree; we might want to
            // just store it directly.
            self.store
                .last_shard()
                .map_err(ShardTreeError::Storage)?
                .and_then(|t| t.max_position())
        } else {
            match self
                .store
                .get_checkpoint_at_depth(checkpoint_depth)
                .map_err(ShardTreeError::Storage)?
            {
                Some((_, c)) => Ok(c.position()),
                None => {
                    // There is no checkpoint at the specified depth, so we report it as pruned.
                    Err(QueryError::CheckpointPruned)
                }
            }?
        })
    }

    /// Computes the root of the tree as of the checkpointed position having the specified
    /// checkpoint id.
    pub fn root_at_checkpoint_id(&self, checkpoint: &C) -> Result<H, ShardTreeError<S::Error>> {
        let position = self
            .store
            .get_checkpoint(checkpoint)
            .map_err(ShardTreeError::Storage)?
            .map(|c| c.position())
            .ok_or(QueryError::CheckpointPruned)?;

        position.map_or_else(
            || Ok(H::empty_root(Self::root_addr().level())),
            |pos| self.root(Self::root_addr(), pos + 1),
        )
    }

    /// Computes the root of the tree as of the checkpointed position having the specified
    /// checkpoint id, caching intermediate values produced while computing the root.
    pub fn root_at_checkpoint_id_caching(
        &mut self,
        checkpoint: &C,
    ) -> Result<H, ShardTreeError<S::Error>> {
        let position = self
            .store
            .get_checkpoint(checkpoint)
            .map_err(ShardTreeError::Storage)?
            .map(|c| c.position())
            .ok_or(QueryError::CheckpointPruned)?;

        position.map_or_else(
            || Ok(H::empty_root(Self::root_addr().level())),
            |pos| self.root_caching(Self::root_addr(), pos + 1),
        )
    }

    /// Computes the root of the tree as of the checkpointed position at the specified depth.
    ///
    /// Returns the root as of the most recently appended leaf if `checkpoint_depth == 0`. Note
    /// that if the most recently appended leaf is also a checkpoint, this will return the same
    /// result as `checkpoint_depth == 1`.
    pub fn root_at_checkpoint_depth(
        &self,
        checkpoint_depth: usize,
    ) -> Result<H, ShardTreeError<S::Error>> {
        self.max_leaf_position(checkpoint_depth)?.map_or_else(
            || Ok(H::empty_root(Self::root_addr().level())),
            |pos| self.root(Self::root_addr(), pos + 1),
        )
    }

    /// Computes the root of the tree as of the checkpointed position at the specified depth,
    /// caching intermediate values produced while computing the root.
    pub fn root_at_checkpoint_depth_caching(
        &mut self,
        checkpoint_depth: usize,
    ) -> Result<H, ShardTreeError<S::Error>> {
        self.max_leaf_position(checkpoint_depth)?.map_or_else(
            || Ok(H::empty_root(Self::root_addr().level())),
            |pos| self.root_caching(Self::root_addr(), pos + 1),
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

        Ok(MerklePath::from_parts(witness, position).unwrap())
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
    /// Returns the witness as of the most recently appended leaf if `checkpoint_depth == 0`. Note
    /// that if the most recently appended leaf is also a checkpoint, this will return the same
    /// result as `checkpoint_depth == 1`.
    pub fn witness_at_checkpoint_depth(
        &self,
        position: Position,
        checkpoint_depth: usize,
    ) -> Result<MerklePath<H, DEPTH>, ShardTreeError<S::Error>> {
        let as_of = self.max_leaf_position(checkpoint_depth).and_then(|v| {
            v.ok_or_else(|| QueryError::TreeIncomplete(vec![Self::root_addr()]).into())
        })?;

        if position > as_of {
            Err(
                QueryError::NotContained(Address::from_parts(Level::from(0), position.into()))
                    .into(),
            )
        } else {
            self.witness_internal(position, as_of)
        }
    }

    /// Computes the witness for the leaf at the specified position, as of the given checkpoint
    /// depth.
    ///
    /// This implementation will mutate the tree to cache intermediate root (ommer) values that are
    /// computed in the process of constructing the witness, so as to avoid the need to recompute
    /// those values from potentially large numbers of subtree roots in the future.
    pub fn witness_at_checkpoint_depth_caching(
        &mut self,
        position: Position,
        checkpoint_depth: usize,
    ) -> Result<MerklePath<H, DEPTH>, ShardTreeError<S::Error>> {
        let as_of = self.max_leaf_position(checkpoint_depth).and_then(|v| {
            v.ok_or_else(|| QueryError::TreeIncomplete(vec![Self::root_addr()]).into())
        })?;

        if position > as_of {
            Err(
                QueryError::NotContained(Address::from_parts(Level::from(0), position.into()))
                    .into(),
            )
        } else {
            self.witness_internal_caching(position, as_of)
        }
    }

    /// Computes the witness for the leaf at the specified position, as of the given checkpoint.
    pub fn witness_at_checkpoint_id(
        &self,
        position: Position,
        checkpoint_id: &C,
    ) -> Result<MerklePath<H, DEPTH>, ShardTreeError<S::Error>> {
        let as_of = self
            .store
            .get_checkpoint(checkpoint_id)
            .map_err(ShardTreeError::Storage)?
            .and_then(|c| c.position())
            .ok_or(QueryError::CheckpointPruned)?;

        self.witness_internal(position, as_of)
    }

    /// Computes the witness for the leaf at the specified position, as of the given checkpoint.
    ///
    /// This implementation will mutate the tree to cache intermediate root (ommer) values that are
    /// computed in the process of constructing the witness, so as to avoid the need to recompute
    /// those values from potentially large numbers of subtree roots in the future.
    pub fn witness_at_checkpoint_id_caching(
        &mut self,
        position: Position,
        checkpoint_id: &C,
    ) -> Result<MerklePath<H, DEPTH>, ShardTreeError<S::Error>> {
        let as_of = self
            .store
            .get_checkpoint(checkpoint_id)
            .map_err(ShardTreeError::Storage)?
            .and_then(|c| c.position())
            .ok_or(QueryError::CheckpointPruned)?;

        self.witness_internal_caching(position, as_of)
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
    use assert_matches::assert_matches;
    use proptest::prelude::*;

    use incrementalmerkletree::{
        frontier::NonEmptyFrontier,
        testing::{
            arb_operation, check_append, check_checkpoint_rewind, check_operations,
            check_remove_mark, check_rewind_remove_mark, check_root_hashes,
            check_witness_consistency, check_witnesses, complete_tree::CompleteTree, CombinedTree,
            SipHashable,
        },
        Address, Hashable, Level, Position, Retention,
    };

    use crate::{
        store::memory::MemoryShardStore,
        testing::{
            arb_char_str, arb_shardtree, check_shard_sizes, check_shardtree_insertion,
            check_witness_with_pruned_subtrees,
        },
        InsertionError, LocatedPrunableTree, ShardTree,
    };

    #[test]
    fn shardtree_insertion() {
        let tree: ShardTree<MemoryShardStore<String, u32>, 4, 3> =
            ShardTree::new(MemoryShardStore::empty(), 100);

        check_shardtree_insertion(tree)
    }

    #[test]
    fn shard_sizes() {
        let tree: ShardTree<MemoryShardStore<String, u32>, 4, 2> =
            ShardTree::new(MemoryShardStore::empty(), 100);

        check_shard_sizes(tree)
    }

    #[test]
    fn witness_with_pruned_subtrees() {
        let tree: ShardTree<MemoryShardStore<String, u32>, 6, 3> =
            ShardTree::new(MemoryShardStore::empty(), 100);

        check_witness_with_pruned_subtrees(tree)
    }

    fn new_tree(m: usize) -> ShardTree<MemoryShardStore<String, usize>, 4, 3> {
        ShardTree::new(MemoryShardStore::empty(), m)
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
                    is_marked: false
                },
            ),
            Ok(()),
        );
    }

    // Combined tree tests
    #[allow(clippy::type_complexity)]
    fn new_combined_tree<H: Hashable + Ord + Clone + core::fmt::Debug>(
        max_checkpoints: usize,
    ) -> CombinedTree<
        H,
        usize,
        CompleteTree<H, usize, 4>,
        ShardTree<MemoryShardStore<H, usize>, 4, 3>,
    > {
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

    proptest! {
        #![proptest_config(ProptestConfig::with_cases(100))]

        #[test]
        fn check_shardtree_caching(
            (mut tree, _, marked_positions) in arb_shardtree(arb_char_str())
        ) {
            if let Some(max_leaf_pos) = tree.max_leaf_position(0).unwrap() {
                let max_complete_addr = Address::above_position(max_leaf_pos.root_level(), max_leaf_pos);
                let root = tree.root(max_complete_addr, max_leaf_pos + 1);
                let caching_root = tree.root_caching(max_complete_addr, max_leaf_pos + 1);
                assert_matches!(root, Ok(_));
                assert_eq!(root, caching_root);

                for pos in marked_positions {
                    let witness = tree.witness_at_checkpoint_depth(pos, 0);
                    let caching_witness = tree.witness_at_checkpoint_depth_caching(pos, 0);
                    assert_matches!(witness, Ok(_));
                    assert_eq!(witness, caching_witness);
                }
            }
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
}
