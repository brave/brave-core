//! Helpers for working with trees that support pruning unneeded leaves and branches.

use std::collections::{BTreeMap, BTreeSet};
use std::fmt;
use std::sync::Arc;

use bitflags::bitflags;
use incrementalmerkletree::Marking;
use incrementalmerkletree::{
    frontier::NonEmptyFrontier, Address, Hashable, Level, Position, Retention,
};
use tracing::{trace, warn};

use crate::error::{InsertionError, QueryError};
use crate::{LocatedTree, Node, Tree};

bitflags! {
    /// Flags storing the [`Retention`] state of a leaf.
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct RetentionFlags: u8 {
        /// An leaf with `EPHEMERAL` retention can be pruned as soon as we are certain that it is not part
        /// of the witness for a leaf with [`CHECKPOINT`] or [`MARKED`] retention.
        ///
        /// [`CHECKPOINT`]: RetentionFlags::CHECKPOINT
        /// [`MARKED`]: RetentionFlags::MARKED
        const EPHEMERAL = 0b00000000;

        /// A leaf with `CHECKPOINT` retention can be pruned when there are more than `max_checkpoints`
        /// additional checkpoint leaves, if it is not also a marked leaf.
        const CHECKPOINT = 0b00000001;

        /// A leaf with `MARKED` retention can be pruned only as a consequence of an explicit deletion
        /// action.
        const MARKED = 0b00000010;

        /// A leaf with `REFERENCE` retention will not be considered prunable until the `REFERENCE`
        /// flag is removed from the leaf. The `REFERENCE` flag will be removed at any point that
        /// the leaf is overwritten without `REFERENCE` retention, and `REFERENCE` retention cannot
        /// be added to an existing leaf.
        const REFERENCE = 0b00000100;
    }
}

impl RetentionFlags {
    pub fn is_checkpoint(&self) -> bool {
        (*self & RetentionFlags::CHECKPOINT) == RetentionFlags::CHECKPOINT
    }

    pub fn is_marked(&self) -> bool {
        (*self & RetentionFlags::MARKED) == RetentionFlags::MARKED
    }
}

impl<'a, C> From<&'a Retention<C>> for RetentionFlags {
    fn from(retention: &'a Retention<C>) -> Self {
        match retention {
            Retention::Ephemeral => RetentionFlags::EPHEMERAL,
            Retention::Checkpoint {
                marking: Marking::Marked,
                ..
            } => RetentionFlags::CHECKPOINT | RetentionFlags::MARKED,
            Retention::Checkpoint {
                marking: Marking::Reference,
                ..
            } => RetentionFlags::CHECKPOINT | RetentionFlags::REFERENCE,
            Retention::Checkpoint { .. } => RetentionFlags::CHECKPOINT,
            Retention::Marked => RetentionFlags::MARKED,
            Retention::Reference => RetentionFlags::REFERENCE,
        }
    }
}

impl<C> From<Retention<C>> for RetentionFlags {
    fn from(retention: Retention<C>) -> Self {
        RetentionFlags::from(&retention)
    }
}

/// A [`Tree`] annotated with Merkle hashes.
pub type PrunableTree<H> = Tree<Option<Arc<H>>, (H, RetentionFlags)>;

/// Errors that can occur when merging trees.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum MergeError {
    Conflict(Address),
    TreeMalformed(Address),
}

impl From<MergeError> for InsertionError {
    fn from(value: MergeError) -> Self {
        match value {
            MergeError::Conflict(addr) => InsertionError::Conflict(addr),
            MergeError::TreeMalformed(addr) => InsertionError::InputMalformed(addr),
        }
    }
}

impl fmt::Display for MergeError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match &self {
            MergeError::Conflict(addr) => write!(
                f,
                "Inserted root conflicts with existing root at address {:?}",
                addr
            ),
            MergeError::TreeMalformed(addr) => {
                write!(f, "Merge input malformed at address {:?}", addr)
            }
        }
    }
}

impl<H: Hashable + Clone + PartialEq> PrunableTree<H> {
    /// Returns the the value if this is a leaf.
    pub fn leaf_value(&self) -> Option<&H> {
        self.0.leaf_value().map(|(h, _)| h)
    }

    /// Returns the cached root value with which the tree has been annotated for this node if it is
    /// available, otherwise return the value if this is a leaf.
    pub fn node_value(&self) -> Option<&H> {
        self.0.annotation().map_or_else(
            || self.leaf_value(),
            |rc_opt| rc_opt.as_ref().map(|rc| rc.as_ref()),
        )
    }

    /// Returns whether or not this tree is a leaf with `Marked` retention.
    pub fn is_marked_leaf(&self) -> bool {
        self.0
            .leaf_value()
            .map_or(false, |(_, retention)| retention.is_marked())
    }

    /// Returns `true` if it is possible to compute or retrieve the Merkle root of this
    /// tree.
    pub fn has_computable_root(&self) -> bool {
        match &self.0 {
            Node::Parent { ann, left, right } => {
                ann.is_some()
                    || (left.as_ref().has_computable_root() && right.as_ref().has_computable_root())
            }
            Node::Leaf { .. } => true,
            Node::Nil => false,
        }
    }

    /// Returns `true` if no additional leaves can be appended to the right hand side of this tree
    /// without over-filling it given its current depth.
    pub fn is_full(&self) -> bool {
        match &self.0 {
            Node::Nil => false,
            Node::Leaf { .. } => true,
            Node::Parent { ann, right, .. } => ann.is_some() || right.is_full(),
        }
    }

    /// Determines whether a tree has any [`Retention::Marked`] nodes.
    pub fn contains_marked(&self) -> bool {
        match &self.0 {
            Node::Parent { left, right, .. } => left.contains_marked() || right.contains_marked(),
            Node::Leaf { value: (_, r) } => r.is_marked(),
            Node::Nil => false,
        }
    }

    /// Returns the Merkle root of this tree, given the address of the root node, or
    /// a vector of the addresses of `Nil` nodes that inhibited the computation of
    /// such a root.
    ///
    /// # Parameters:
    /// * `truncate_at` An inclusive lower bound on positions in the tree beyond which all leaf
    ///   values will be treated as `Nil`.
    pub fn root_hash(&self, root_addr: Address, truncate_at: Position) -> Result<H, Vec<Address>> {
        if truncate_at <= root_addr.position_range_start() {
            // we are in the part of the tree where we're generating empty roots,
            // so no need to inspect the tree
            Ok(H::empty_root(root_addr.level()))
        } else {
            match &self.0 {
                Node::Parent { ann, left, right } => ann
                    .as_ref()
                    .filter(|_| truncate_at >= root_addr.position_range_end())
                    .map_or_else(
                        || {
                            // Compute the roots of the left and right children and hash them
                            // together.
                            let (l_addr, r_addr) = root_addr
                                .children()
                                .expect("The root address of a parent node must have children.");
                            accumulate_result_with(
                                left.root_hash(l_addr, truncate_at),
                                right.root_hash(r_addr, truncate_at),
                                |left_root, right_root| {
                                    H::combine(l_addr.level(), &left_root, &right_root)
                                },
                            )
                        },
                        |rc| {
                            // Since we have an annotation on the root, and we are not truncating
                            // within this subtree, we can just use the cached value.
                            Ok(rc.as_ref().clone())
                        },
                    ),
                Node::Leaf { value } => {
                    if truncate_at >= root_addr.position_range_end() {
                        // no truncation of this leaf is necessary, just use it
                        Ok(value.0.clone())
                    } else {
                        // we have a leaf value that is a subtree root created by hashing together
                        // the roots of child subtrees, but truncation would require that that leaf
                        // value be "split" into its constituent parts, which we can't do so we
                        // return an error
                        Err(vec![root_addr])
                    }
                }
                Node::Nil => Err(vec![root_addr]),
            }
        }
    }

    /// Returns a vector of the positions of [`Node::Leaf`] values in the tree having
    /// [`MARKED`](RetentionFlags::MARKED) retention.
    ///
    /// Computing the set of marked positions requires a full traversal of the tree, and so should
    /// be considered to be a somewhat expensive operation.
    pub fn marked_positions(&self, root_addr: Address) -> BTreeSet<Position> {
        match &self.0 {
            Node::Parent { left, right, .. } => {
                // We should never construct parent nodes where both children are Nil.
                // While we could handle that here, if we encountered that case it would
                // be indicative of a programming error elsewhere and so we assert instead.
                assert!(!(left.0.is_nil() && right.0.is_nil()));
                let (left_root, right_root) = root_addr
                    .children()
                    .expect("A parent node cannot appear at level 0");

                let mut left_incomplete = left.marked_positions(left_root);
                let mut right_incomplete = right.marked_positions(right_root);
                left_incomplete.append(&mut right_incomplete);
                left_incomplete
            }
            Node::Leaf {
                value: (_, retention),
            } => {
                let mut result = BTreeSet::new();
                if root_addr.level() == 0.into() && retention.is_marked() {
                    result.insert(Position::from(root_addr.index()));
                }
                result
            }
            Node::Nil => BTreeSet::new(),
        }
    }

    /// Prunes the tree by hashing together ephemeral sibling nodes.
    ///
    /// `level` must be the level of the root of the node being pruned.
    pub fn prune(self, level: Level) -> Self {
        match self {
            Tree(Node::Parent { ann, left, right }) => Tree::unite(
                level,
                ann,
                left.as_ref().clone().prune(level - 1),
                right.as_ref().clone().prune(level - 1),
            ),
            other => other,
        }
    }

    /// Merge two subtrees having the same root address.
    ///
    /// The merge operation is checked to be strictly additive and returns an error if merging
    /// would cause information loss or if a conflict between root hashes occurs at a node. The
    /// returned error contains the address of the node where such a conflict occurred.
    #[tracing::instrument()]
    pub fn merge_checked(self, root_addr: Address, other: Self) -> Result<Self, MergeError> {
        /// Pre-condition: `root_addr` must be the address of `t0` and `t1`.
        #[allow(clippy::type_complexity)]
        fn go<H: Hashable + Clone + PartialEq>(
            addr: Address,
            t0: PrunableTree<H>,
            t1: PrunableTree<H>,
        ) -> Result<PrunableTree<H>, MergeError> {
            // Require that any roots the we compute will not be default-filled by picking
            // a starting valid fill point that is outside the range of leaf positions.
            let no_default_fill = addr.position_range_end();
            match (t0, t1) {
                (Tree(Node::Nil), other) | (other, Tree(Node::Nil)) => Ok(other),
                (Tree(Node::Leaf { value: vl }), Tree(Node::Leaf { value: vr })) => {
                    if vl.0 == vr.0 {
                        // Merge the flags together.
                        Ok(Tree::leaf((vl.0, vl.1 | vr.1)))
                    } else {
                        trace!(left = ?vl.0, right = ?vr.0, "Merge conflict for leaves");
                        Err(MergeError::Conflict(addr))
                    }
                }
                (Tree(Node::Leaf { value }), parent @ Tree(Node::Parent { .. }))
                | (parent @ Tree(Node::Parent { .. }), Tree(Node::Leaf { value })) => {
                    let parent_hash = parent.root_hash(addr, no_default_fill);
                    if parent_hash.iter().all(|r| r == &value.0) {
                        Ok(parent.reannotate_root(Some(Arc::new(value.0))))
                    } else {
                        trace!(leaf = ?value, node = ?parent_hash, "Merge conflict for leaf into node");
                        Err(MergeError::Conflict(addr))
                    }
                }
                (lparent, rparent) => {
                    let lroot = lparent.root_hash(addr, no_default_fill).ok();
                    let rroot = rparent.root_hash(addr, no_default_fill).ok();
                    // If both parents share the same root hash (or if one of them is absent),
                    // they can be merged
                    if lroot.iter().zip(&rroot).all(|(l, r)| l == r) {
                        // using `if let` here to bind variables; we need to borrow the trees for
                        // root hash calculation but binding the children of the parent node
                        // interferes with binding a reference to the parent.
                        if let (
                            Tree(Node::Parent {
                                ann: lann,
                                left: ll,
                                right: lr,
                            }),
                            Tree(Node::Parent {
                                ann: rann,
                                left: rl,
                                right: rr,
                            }),
                        ) = (lparent, rparent)
                        {
                            let (l_addr, r_addr) =
                                addr.children().ok_or(MergeError::TreeMalformed(addr))?;
                            Ok(Tree::unite(
                                addr.level() - 1,
                                lann.or(rann),
                                go(l_addr, ll.as_ref().clone(), rl.as_ref().clone())?,
                                go(r_addr, lr.as_ref().clone(), rr.as_ref().clone())?,
                            ))
                        } else {
                            unreachable!()
                        }
                    } else {
                        trace!(left = ?lroot, right = ?rroot, "Merge conflict for nodes");
                        Err(MergeError::Conflict(addr))
                    }
                }
            }
        }

        go(root_addr, self, other)
    }

    /// Unite two nodes by either constructing a new parent node, or, if both nodes are ephemeral
    /// leaves or Nil, constructing a replacement parent by hashing leaf values together (or a
    /// replacement `Nil` value).
    ///
    /// `level` must be the level of the two nodes that are being joined.
    pub(crate) fn unite(level: Level, ann: Option<Arc<H>>, left: Self, right: Self) -> Self {
        match (left, right) {
            (Tree(Node::Nil), Tree(Node::Nil)) if ann.is_none() => Tree::empty(),
            (Tree(Node::Leaf { value: lv }), Tree(Node::Leaf { value: rv }))
                // we can prune right-hand leaves that are not marked or reference leaves; if a
                // leaf is a checkpoint then that information will be propagated to the replacement
                // leaf
                if lv.1 == RetentionFlags::EPHEMERAL &&
                    (rv.1 & (RetentionFlags::MARKED | RetentionFlags::REFERENCE)) == RetentionFlags::EPHEMERAL =>
            {
                Tree::leaf((H::combine(level, &lv.0, &rv.0), rv.1))
            }
            (left, right) => Tree::parent(ann, left, right),
        }
    }
}

/// A [`LocatedTree`] annotated with Merkle hashes.
pub type LocatedPrunableTree<H> = LocatedTree<Option<Arc<H>>, (H, RetentionFlags)>;

/// A data structure describing the nature of a [`Node::Nil`] node in the tree that was introduced
/// as the consequence of an insertion.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct IncompleteAt {
    /// The address of the empty node.
    pub address: Address,
    /// A flag identifying whether or not the missing node is required in order to construct a
    /// witness for a node with [`MARKED`] retention.
    ///
    /// [`MARKED`]: RetentionFlags::MARKED
    pub required_for_witness: bool,
}

/// Validation errors that can occur during reconstruction of a Merkle frontier from
/// the state of a [`LocatedPrunableTree`].
#[derive(Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum FrontierError {
    /// An error representing that the number of ommers provided in frontier construction does not
    /// the expected length of the ommers list given the position.
    PositionMismatch { expected_ommers: u8 },
    /// An error representing that the position and/or list of ommers provided to frontier
    /// construction would result in a frontier that exceeds the maximum statically allowed depth
    /// of the tree. `depth` is the minimum tree depth that would be required in order for that
    /// tree to contain the position in question.
    MaxDepthExceeded { depth: u8 },
    /// The tree from which the frontier was being extracted is incomplete, and consequently no
    /// frontier can be computed.
    TreeIncomplete { address: Address },
    /// The tree from which the frontier was being extracted is corrupt; an invalid node was found
    /// at the given address.
    CorruptedData { address: Address },
}

impl From<incrementalmerkletree::frontier::FrontierError> for FrontierError {
    fn from(value: incrementalmerkletree::frontier::FrontierError) -> Self {
        use incrementalmerkletree::frontier::FrontierError::*;
        match value {
            PositionMismatch { expected_ommers } => {
                FrontierError::PositionMismatch { expected_ommers }
            }
            MaxDepthExceeded { depth } => FrontierError::MaxDepthExceeded { depth },
        }
    }
}

impl<H: Hashable + Clone + PartialEq> LocatedPrunableTree<H> {
    /// Returns the maximum position at which a non-`Nil` leaf has been observed in the tree.
    ///
    /// Note that no actual leaf value may exist at this position, as it may have previously been
    /// pruned.
    pub fn max_position(&self) -> Option<Position> {
        /// Pre-condition: `addr` must be the address of `root`.
        fn go<H>(
            addr: Address,
            root: &Tree<Option<Arc<H>>, (H, RetentionFlags)>,
        ) -> Option<Position> {
            match &root.0 {
                Node::Nil => None,
                Node::Leaf { .. } => Some(addr.position_range_end() - 1),
                Node::Parent { ann, left, right } => {
                    if ann.is_some() {
                        Some(addr.max_position())
                    } else {
                        let (l_addr, r_addr) = addr
                            .children()
                            .expect("has children because we checked `root` is a parent");
                        go(r_addr, right.as_ref()).or_else(|| go(l_addr, left.as_ref()))
                    }
                }
            }
        }

        go(self.root_addr, &self.root)
    }

    /// Computes the root hash of this tree, truncated to the given position.
    ///
    /// If the tree contains any [`Node::Nil`] nodes corresponding to positions less than
    /// `truncate_at`, this will return an error containing the addresses of those nodes within the
    /// tree.
    pub fn root_hash(&self, truncate_at: Position) -> Result<H, Vec<Address>> {
        self.root.root_hash(self.root_addr, truncate_at)
    }

    /// Compute the root hash of this subtree, filling empty nodes along the rightmost path of the
    /// subtree with the empty root value for the given level.
    ///
    /// This should only be used for computing roots when it is known that no successor trees
    /// exist.
    ///
    /// If the tree contains any [`Node::Nil`] nodes that are to the left of filled nodes in the
    /// tree, this will return an error containing the addresses of those nodes.
    pub fn right_filled_root(&self) -> Result<H, Vec<Address>> {
        let truncate_at = self
            .max_position()
            .map_or_else(|| self.root_addr.position_range_start(), |pos| pos + 1);

        self.root_hash(truncate_at)
    }

    /// Returns the positions of marked leaves in the tree.
    pub fn marked_positions(&self) -> BTreeSet<Position> {
        /// Pre-condition: `root_addr` must be the address of `root`.
        fn go<H: Hashable + Clone + PartialEq>(
            root_addr: Address,
            root: &PrunableTree<H>,
            acc: &mut BTreeSet<Position>,
        ) {
            match &root.0 {
                Node::Parent { left, right, .. } => {
                    let (l_addr, r_addr) = root_addr
                        .children()
                        .expect("has children because we checked `root` is a parent");
                    go(l_addr, left.as_ref(), acc);
                    go(r_addr, right.as_ref(), acc);
                }
                Node::Leaf { value } => {
                    if value.1.is_marked() && root_addr.level() == 0.into() {
                        acc.insert(Position::from(root_addr.index()));
                    }
                }
                _ => {}
            }
        }

        let mut result = BTreeSet::new();
        go(self.root_addr, &self.root, &mut result);
        result
    }

    /// Compute the witness for the leaf at the specified position.
    ///
    /// This tree will be truncated to the `truncate_at` position, and then empty roots
    /// corresponding to later positions will be filled by the [`Hashable::empty_root`]
    /// implementation for `H`.
    ///
    /// Returns either the witness for the leaf at the specified position, or an error that
    /// describes the causes of failure.
    pub fn witness(&self, position: Position, truncate_at: Position) -> Result<Vec<H>, QueryError> {
        /// Traverse down to the desired leaf position, and then construct
        /// the authentication path on the way back up.
        //
        /// Pre-condition: `root_addr` must be the address of `root`.
        fn go<H: Hashable + Clone + PartialEq>(
            root: &PrunableTree<H>,
            root_addr: Address,
            position: Position,
            truncate_at: Position,
        ) -> Result<Vec<H>, Vec<Address>> {
            match &root.0 {
                Node::Parent { left, right, .. } => {
                    let (l_addr, r_addr) = root_addr
                        .children()
                        .expect("has children because we checked `root` is a parent");
                    if root_addr.level() > 1.into() {
                        let r_start = r_addr.position_range_start();
                        if position < r_start {
                            accumulate_result_with(
                                go(left.as_ref(), l_addr, position, truncate_at),
                                right.as_ref().root_hash(r_addr, truncate_at),
                                |mut witness, sibling_root| {
                                    witness.push(sibling_root);
                                    witness
                                },
                            )
                        } else {
                            // if the position we're witnessing is down the right-hand branch then
                            // we always set the truncation bound outside the range of leaves on the
                            // left, because we don't allow any empty nodes to the left
                            accumulate_result_with(
                                left.as_ref().root_hash(l_addr, r_start),
                                go(right.as_ref(), r_addr, position, truncate_at),
                                |sibling_root, mut witness| {
                                    witness.push(sibling_root);
                                    witness
                                },
                            )
                        }
                    } else {
                        // we handle the level 0 leaves here by adding the sibling of our desired
                        // leaf to the witness
                        if position.is_right_child() {
                            if right.is_marked_leaf() {
                                left.leaf_value()
                                    .map(|v| vec![v.clone()])
                                    .ok_or_else(|| vec![l_addr])
                            } else {
                                Err(vec![l_addr])
                            }
                        } else if left.is_marked_leaf() {
                            // If we have the left-hand leaf and the right-hand leaf is empty, we
                            // can fill it with the empty leaf, but only if we are truncating at
                            // a position to the left of the current position
                            if truncate_at <= position + 1 {
                                Ok(vec![H::empty_leaf()])
                            } else {
                                right
                                    .leaf_value()
                                    .map_or_else(|| Err(vec![r_addr]), |v| Ok(vec![v.clone()]))
                            }
                        } else {
                            Err(vec![r_addr])
                        }
                    }
                }
                _ => {
                    // if we encounter a nil or leaf node, we were unable to descend
                    // to the leaf at the desired position.
                    Err(vec![root_addr])
                }
            }
        }

        if self.root_addr.position_range().contains(&position) {
            go(&self.root, self.root_addr, position, truncate_at)
                .map_err(QueryError::TreeIncomplete)
        } else {
            Err(QueryError::NotContained(self.root_addr))
        }
    }

    /// Prunes this tree by replacing all nodes that are right-hand children along the path
    /// to the specified position with [`Node::Nil`].
    ///
    /// The leaf at the specified position is retained. Returns the truncated tree if a leaf or
    /// subtree root with the specified position as its maximum position exists, or `None`
    /// otherwise.
    pub fn truncate_to_position(&self, position: Position) -> Option<Self> {
        /// Pre-condition: `root_addr` must be the address of `root`.
        fn go<H: Hashable + Clone + PartialEq>(
            position: Position,
            root_addr: Address,
            root: &PrunableTree<H>,
        ) -> Option<PrunableTree<H>> {
            match &root.0 {
                Node::Parent { ann, left, right } => {
                    let (l_child, r_child) = root_addr
                        .children()
                        .expect("has children because we checked `root` is a parent");
                    if position < r_child.position_range_start() {
                        // we are truncating within the range of the left node, so recurse
                        // to the left to truncate the left child and then reconstruct the
                        // node with `Nil` as the right sibling
                        go(position, l_child, left.as_ref()).map(|left| {
                            Tree::unite(l_child.level(), ann.clone(), left, Tree::empty())
                        })
                    } else {
                        // we are truncating within the range of the right node, so recurse
                        // to the right to truncate the right child and then reconstruct the
                        // node with the left sibling unchanged
                        go(position, r_child, right.as_ref()).map(|right| {
                            Tree::unite(r_child.level(), ann.clone(), left.as_ref().clone(), right)
                        })
                    }
                }
                Node::Leaf { .. } => {
                    if root_addr.max_position() <= position {
                        Some(root.clone())
                    } else {
                        None
                    }
                }
                Node::Nil => None,
            }
        }

        if self.root_addr.position_range().contains(&position) {
            go(position, self.root_addr, &self.root).map(|root| LocatedTree {
                root_addr: self.root_addr,
                root,
            })
        } else {
            None
        }
    }

    /// Inserts a descendant subtree into this subtree, creating empty sibling nodes as necessary
    /// to fill out the tree.
    ///
    /// In the case that a leaf node would be replaced by an incomplete subtree, the resulting
    /// parent node will be annotated with the existing leaf value.
    ///
    /// Returns the updated tree, along with the addresses of any [`Node::Nil`] nodes that were
    /// inserted in the process of creating the parent nodes down to the insertion point, or an
    /// error if the specified subtree's root address is not in the range of valid descendants of
    /// the root node of this tree or if the insertion would result in a conflict between computed
    /// root hashes of complete subtrees.
    pub fn insert_subtree(
        &self,
        subtree: Self,
        contains_marked: bool,
    ) -> Result<(Self, Vec<IncompleteAt>), InsertionError> {
        /// A function to recursively dig into the tree, creating a path downward and introducing
        /// empty nodes as necessary until we can insert the provided subtree.
        ///
        /// Pre-condition: `root_addr` must be the address of `into`.
        #[allow(clippy::type_complexity)]
        fn go<H: Hashable + Clone + PartialEq>(
            root_addr: Address,
            into: &PrunableTree<H>,
            subtree: LocatedPrunableTree<H>,
            contains_marked: bool,
        ) -> Result<(PrunableTree<H>, Vec<IncompleteAt>), InsertionError> {
            // An empty tree cannot replace any other type of tree.
            if subtree.root().is_nil() {
                Ok((into.clone(), vec![]))
            } else {
                // In the case that we are replacing a node entirely, we need to extend the
                // subtree up to the level of the node being replaced, adding Nil siblings
                // and recording the presence of those incomplete nodes when necessary
                let replacement = |ann: Option<Arc<H>>, mut node: LocatedPrunableTree<H>| {
                    // construct the replacement node bottom-up
                    let mut incomplete = vec![];
                    while node.root_addr.level() < root_addr.level() {
                        incomplete.push(IncompleteAt {
                            address: node.root_addr.sibling(),
                            required_for_witness: contains_marked,
                        });
                        let full = node.root;
                        node = LocatedTree {
                            root_addr: node.root_addr.parent(),
                            root: if node.root_addr.is_right_child() {
                                Tree::parent(None, Tree::empty(), full)
                            } else {
                                Tree::parent(None, full, Tree::empty())
                            },
                        };
                    }
                    (node.root.reannotate_root(ann), incomplete)
                };

                match into {
                    Tree(Node::Nil) => Ok(replacement(None, subtree)),
                    Tree(Node::Leaf {
                        value: (value, retention),
                    }) => {
                        if root_addr == subtree.root_addr {
                            // The current leaf is at the location we wish to transplant the root
                            // of the subtree being inserted, so we either replace the leaf
                            // entirely with the subtree, or reannotate the root so as to avoid
                            // discarding the existing leaf value.

                            if subtree.root.has_computable_root() {
                                Ok((
                                    if subtree.root.is_leaf() {
                                        // When replacing a leaf with a leaf, `REFERENCE` retention
                                        // will be discarded unless both leaves have `REFERENCE`
                                        // retention.
                                        subtree
                                            .root
                                            .try_map::<(H, RetentionFlags), InsertionError, _>(
                                                &|(v0, ret0)| {
                                                    if v0 == value {
                                                        let retention_result: RetentionFlags =
                                                            ((*retention | *ret0)
                                                                - RetentionFlags::REFERENCE)
                                                                | (RetentionFlags::REFERENCE
                                                                    & *retention
                                                                    & *ret0);
                                                        Ok((value.clone(), retention_result))
                                                    } else {
                                                        Err(InsertionError::Conflict(root_addr))
                                                    }
                                                },
                                            )?
                                    } else {
                                        // It is safe to replace the existing root unannotated, because we
                                        // can always recompute the root from the subtree.
                                        subtree.root
                                    },
                                    vec![],
                                ))
                            } else if subtree.root.node_value().iter().all(|v| v == &value) {
                                Ok((
                                    // at this point we statically know the root to be a parent
                                    subtree.root.reannotate_root(Some(Arc::new(value.clone()))),
                                    vec![],
                                ))
                            } else {
                                warn!(
                                    cur_root = ?value,
                                    new_root = ?subtree.root.node_value(),
                                    "Insertion conflict",
                                );
                                Err(InsertionError::Conflict(root_addr))
                            }
                        } else {
                            Ok(replacement(Some(Arc::new(value.clone())), subtree))
                        }
                    }
                    parent if root_addr == subtree.root_addr => {
                        // Merge the existing subtree with the subtree being inserted.
                        // A merge operation can't introduce any new incomplete roots.
                        parent
                            .clone()
                            .merge_checked(root_addr, subtree.root)
                            .map_err(InsertionError::from)
                            .map(|tree| (tree, vec![]))
                    }
                    Tree(Node::Parent { ann, left, right }) => {
                        // In this case, we have an existing parent but we need to dig down farther
                        // before we can insert the subtree that we're carrying for insertion.
                        let (l_addr, r_addr) = root_addr
                            .children()
                            .expect("has children because we checked `into` is a parent");
                        if l_addr.contains(&subtree.root_addr) {
                            let (new_left, incomplete) =
                                go(l_addr, left.as_ref(), subtree, contains_marked)?;
                            Ok((
                                Tree::unite(
                                    root_addr.level() - 1,
                                    ann.clone(),
                                    new_left,
                                    right.as_ref().clone(),
                                ),
                                incomplete,
                            ))
                        } else {
                            let (new_right, incomplete) =
                                go(r_addr, right.as_ref(), subtree, contains_marked)?;
                            Ok((
                                Tree::unite(
                                    root_addr.level() - 1,
                                    ann.clone(),
                                    left.as_ref().clone(),
                                    new_right,
                                ),
                                incomplete,
                            ))
                        }
                    }
                }
            }
        }

        let max_position = self.max_position();
        trace!(
            max_position = ?max_position,
            tree = ?self,
            to_insert = ?subtree,
            "Current shard"
        );
        let LocatedTree { root_addr, root } = self;
        if root_addr.contains(&subtree.root_addr) {
            go(*root_addr, root, subtree, contains_marked).map(|(root, incomplete)| {
                let new_tree = LocatedTree {
                    root_addr: *root_addr,
                    root,
                };
                assert!(new_tree.max_position() >= max_position);
                (new_tree, incomplete)
            })
        } else {
            Err(InsertionError::NotContained(subtree.root_addr))
        }
    }

    /// Append a single value at the first available position in the tree.
    ///
    /// Prefer to use [`Self::batch_append`] or [`Self::batch_insert`] when appending multiple
    /// values, as these operations require fewer traversals of the tree than are necessary when
    /// performing multiple sequential calls to [`Self::append`].
    pub fn append<C: Clone + Ord>(
        &self,
        value: H,
        retention: Retention<C>,
    ) -> Result<(Self, Position, Option<C>), InsertionError> {
        let checkpoint_id = if let Retention::Checkpoint { id, .. } = &retention {
            Some(id.clone())
        } else {
            None
        };

        self.batch_append(Some((value, retention)).into_iter())
            // We know that the max insert position will have been incremented by one.
            .and_then(|r| {
                let mut r = r.expect("We know the iterator to have been nonempty.");
                if r.remainder.next().is_some() {
                    Err(InsertionError::TreeFull)
                } else {
                    Ok((r.subtree, r.max_insert_position, checkpoint_id))
                }
            })
    }

    // Constructs a pair of trees that contain the leaf and ommers of the given frontier. The first
    // element of the result is a tree with its root at a level less than or equal to `split_at`;
    // the second element is a tree with its leaves at level `split_at` that is only returned if
    // the frontier contains sufficient data to fill the first tree to the `split_at` level.
    fn from_frontier<C>(
        frontier: NonEmptyFrontier<H>,
        leaf_retention: &Retention<C>,
        split_at: Level,
    ) -> (Self, Option<Self>) {
        let (position, leaf, ommers) = frontier.into_parts();
        Self::from_frontier_parts(position, leaf, ommers.into_iter(), leaf_retention, split_at)
    }

    // Permits construction of a subtree from legacy `CommitmentTree` data that may
    // have inaccurate position information (e.g. in the case that the tree is the
    // cursor for an `IncrementalWitness`).
    pub(crate) fn from_frontier_parts<C>(
        position: Position,
        leaf: H,
        mut ommers: impl Iterator<Item = H>,
        leaf_retention: &Retention<C>,
        split_at: Level,
    ) -> (Self, Option<Self>) {
        let mut addr = Address::from(position);
        let mut subtree = Tree::leaf((leaf, leaf_retention.into()));

        while addr.level() < split_at {
            if addr.is_left_child() {
                // the current address is a left child, so create a parent with
                // an empty right-hand tree
                subtree = Tree::parent(None, subtree, Tree::empty());
            } else if let Some(left) = ommers.next() {
                // the current address corresponds to a right child, so create a parent that
                // takes the left sibling to that child from the ommers
                subtree =
                    Tree::parent(None, Tree::leaf((left, RetentionFlags::EPHEMERAL)), subtree);
            } else {
                break;
            }

            addr = addr.parent();
        }

        let located_subtree = LocatedTree {
            root_addr: addr,
            root: subtree,
        };

        let located_supertree = if located_subtree.root_addr().level() == split_at {
            let mut addr = located_subtree.root_addr();
            let mut supertree = None;
            for left in ommers {
                // build up the left-biased tree until we get a right-hand node
                while addr.is_left_child() {
                    supertree = supertree.map(|t| Tree::parent(None, t, Tree::empty()));
                    addr = addr.parent();
                }

                // once we have a right-hand root, add a parent with the current ommer as the
                // left-hand sibling
                supertree = Some(Tree::parent(
                    None,
                    Tree::leaf((left, RetentionFlags::EPHEMERAL)),
                    supertree.unwrap_or_else(Tree::empty),
                ));
                addr = addr.parent();
            }

            supertree.map(|t| LocatedTree {
                root_addr: addr,
                root: t,
            })
        } else {
            // if there were not enough ommers available from the frontier to reach the address
            // of the root of this tree, there is no contribution to the cap
            None
        };

        (located_subtree, located_supertree)
    }

    /// Inserts leaves and subtree roots from the provided frontier into this tree, up to the level
    /// of this tree's root.
    ///
    /// Returns the updated tree, along with a [`LocatedPrunableTree`] containing only the remainder
    /// of the frontier's ommers that had addresses at levels greater than the root of this tree.
    ///
    /// Returns an error in the following cases:
    /// * the leaf node of `frontier` is at a position that is not contained within this tree's
    ///   position range
    /// * a conflict occurs where an ommer of the frontier being inserted does not match the
    ///   existing value for that node
    pub fn insert_frontier_nodes<C>(
        &self,
        frontier: NonEmptyFrontier<H>,
        leaf_retention: &Retention<C>,
    ) -> Result<(Self, Option<Self>), InsertionError> {
        let subtree_range = self.root_addr.position_range();
        if subtree_range.contains(&frontier.position()) {
            let leaf_is_marked = leaf_retention.is_marked();
            let (subtree, supertree) =
                Self::from_frontier(frontier, leaf_retention, self.root_addr.level());

            let subtree = self.insert_subtree(subtree, leaf_is_marked)?.0;

            Ok((subtree, supertree))
        } else {
            Err(InsertionError::OutOfRange(
                frontier.position(),
                subtree_range,
            ))
        }
    }

    /// Clears the specified retention flags at all positions specified, pruning any branches
    /// that no longer need to be retained.
    pub fn clear_flags(&self, to_clear: BTreeMap<Position, RetentionFlags>) -> Self {
        /// Pre-condition: `root_addr` must be the address of `root`.
        fn go<H: Hashable + Clone + PartialEq>(
            to_clear: &[(Position, RetentionFlags)],
            root_addr: Address,
            root: &PrunableTree<H>,
        ) -> PrunableTree<H> {
            if to_clear.is_empty() {
                // nothing to do, so we just return the root
                root.clone()
            } else {
                match &root.0 {
                    Node::Parent { ann, left, right } => {
                        let (l_addr, r_addr) = root_addr
                            .children()
                            .expect("has children because we checked `root` is a parent");

                        let p = to_clear.partition_point(|(p, _)| p < &l_addr.position_range_end());
                        trace!(
                            "Tree::unite at {:?}, partitioned: {:?} {:?}",
                            root_addr,
                            &to_clear[0..p],
                            &to_clear[p..],
                        );
                        Tree::unite(
                            l_addr.level(),
                            ann.clone(),
                            go(&to_clear[0..p], l_addr, left),
                            go(&to_clear[p..], r_addr, right),
                        )
                    }
                    Node::Leaf { value: (h, r) } => {
                        trace!("In {:?}, clearing {:?}", root_addr, to_clear);
                        // When we reach a leaf, we should be down to just a single position
                        // which should correspond to the last level-0 child of the address's
                        // subtree range; if it's a checkpoint this will always be the case for
                        // a partially-pruned branch, and if it's a marked node then it will
                        // be a level-0 leaf.
                        match to_clear {
                            [(_, flags)] => Tree::leaf((h.clone(), *r & !*flags)),
                            _ => {
                                panic!("Tree state inconsistent with checkpoints.");
                            }
                        }
                    }
                    Node::Nil => Tree::empty(),
                }
            }
        }

        let to_clear = to_clear.into_iter().collect::<Vec<_>>();
        Self {
            root_addr: self.root_addr,
            root: go(&to_clear, self.root_addr, &self.root),
        }
    }

    #[cfg(test)]
    pub(crate) fn flag_positions(&self) -> BTreeMap<Position, RetentionFlags> {
        fn go<H>(
            root: &PrunableTree<H>,
            root_addr: Address,
            acc: &mut BTreeMap<Position, RetentionFlags>,
        ) {
            match &root.0 {
                Node::Parent { left, right, .. } => {
                    let (l_addr, r_addr) = root_addr
                        .children()
                        .expect("A parent node cannot appear at level 0");
                    go(left, l_addr, acc);
                    go(right, r_addr, acc);
                }
                Node::Leaf { value } if value.1 != RetentionFlags::EPHEMERAL => {
                    acc.insert(root_addr.max_position(), value.1);
                }
                _ => (),
            }
        }

        let mut result = BTreeMap::new();
        go(&self.root, self.root_addr, &mut result);
        result
    }

    /// Returns the position, leaf hash, and ommers for the frontier of this tree, `Ok(None)` if
    /// the tree is empty, or `Err(())` if the frontier cannot be computed due to missing data.
    ///
    /// The returned ommers cover only the levels within this tree (from level 0 up to
    /// but not including this tree's root level). This is a building block for
    /// [`ShardTree::frontier`] which extends the ommers to the full tree depth.
    pub(crate) fn frontier_ommers(&self) -> Result<Option<(Position, H, Vec<H>)>, FrontierError> {
        /// Traverses the rightmost path of the tree, collecting the frontier leaf and ommers.
        /// Returns `(position, leaf_hash, ommers)` with ommers ordered from lowest to highest
        /// level, or `None` if the frontier cannot be extracted.
        ///
        /// Pre-condition: `addr` must be the address of `root`.
        fn go<H: Hashable + Clone + PartialEq>(
            address: Address,
            root: &PrunableTree<H>,
        ) -> Result<Option<(Position, H, Vec<H>)>, FrontierError> {
            match &root.0 {
                Node::Nil => Err(FrontierError::TreeIncomplete { address }),
                Node::Leaf { value: (h, _) } => {
                    if address.level() == Level::ZERO {
                        Ok(Some((Position::from(address.index()), h.clone(), vec![])))
                    } else {
                        // A leaf above level 0 is a pruned subtree; we cannot extract
                        // the frontier leaf or internal ommers.
                        Err(FrontierError::TreeIncomplete { address })
                    }
                }
                Node::Parent { left, right, .. } => {
                    let (l_addr, r_addr) = address
                        .children()
                        .ok_or(FrontierError::CorruptedData { address })?;

                    if right.0.is_nil() {
                        // Right subtree is empty; the frontier is in the left subtree
                        // and no ommer is needed at this level.
                        go(l_addr, left)
                    } else {
                        // Right subtree is populated; the frontier is within it.
                        // The left subtree's root hash becomes an ommer.
                        go(r_addr, right)?
                            .map(|(pos, leaf, mut ommers)| {
                                let left_hash = left
                                    .root_hash(l_addr, r_addr.position_range_start())
                                    .map_err(|_| FrontierError::TreeIncomplete {
                                        address: l_addr,
                                    })?;
                                ommers.push(left_hash);
                                Ok((pos, leaf, ommers))
                            })
                            .transpose()
                    }
                }
            }
        }

        if self.root.is_nil() {
            Ok(None)
        } else {
            go(self.root_addr, &self.root)
        }
    }

    /// Returns the Merkle frontier of the tree, if the tree is nonempty and has no `Nil` leaves
    /// prior to the leaf at the greatest position.
    pub fn frontier(&self) -> Result<Option<NonEmptyFrontier<H>>, FrontierError> {
        self.frontier_ommers()?
            .map(|(position, leaf, ommers)| NonEmptyFrontier::from_parts(position, leaf, ommers))
            .transpose()
            .map_err(FrontierError::from)
    }
}

// We need an applicative functor for Result for this function so that we can correctly
// accumulate errors, but we don't have one so we just write a special- cased version here.
fn accumulate_result_with<A, B, C>(
    left: Result<A, Vec<Address>>,
    right: Result<B, Vec<Address>>,
    combine_success: impl FnOnce(A, B) -> C,
) -> Result<C, Vec<Address>> {
    match (left, right) {
        (Ok(a), Ok(b)) => Ok(combine_success(a, b)),
        (Err(mut xs), Err(mut ys)) => {
            xs.append(&mut ys);
            Err(xs)
        }
        (Ok(_), Err(xs)) => Err(xs),
        (Err(xs), Ok(_)) => Err(xs),
    }
}

#[cfg(test)]
mod tests {
    use assert_matches::assert_matches;
    use std::collections::{BTreeMap, BTreeSet};

    use incrementalmerkletree::{frontier::NonEmptyFrontier, Address, Level, Position};
    use proptest::proptest;

    use super::{LocatedPrunableTree, PrunableTree, RetentionFlags};
    use crate::{
        error::{InsertionError, QueryError},
        prunable::{FrontierError, MergeError},
        testing::{arb_char_str, arb_prunable_tree},
        tree::{
            tests::{leaf, nil, parent},
            LocatedTree,
        },
    };

    #[test]
    fn root() {
        let t: PrunableTree<String> = parent(
            leaf(("a".to_string(), RetentionFlags::EPHEMERAL)),
            leaf(("b".to_string(), RetentionFlags::EPHEMERAL)),
        );

        assert_eq!(
            t.root_hash(Address::from_parts(Level::from(1), 0), Position::from(2)),
            Ok("ab".to_string())
        );

        let t0 = parent(nil(), t.clone());
        assert_eq!(
            t0.root_hash(Address::from_parts(Level::from(2), 0), Position::from(4)),
            Err(vec![Address::from_parts(Level::from(1), 0)])
        );

        // Check root computation with truncation
        let t1 = parent(t, nil());
        assert_eq!(
            t1.root_hash(Address::from_parts(Level::from(2), 0), Position::from(2)),
            Ok("ab__".to_string())
        );
        assert_eq!(
            t1.root_hash(Address::from_parts(Level::from(2), 0), Position::from(3)),
            Err(vec![Address::from_parts(Level::from(1), 1)])
        );
    }

    #[test]
    fn marked_positions() {
        let t: PrunableTree<String> = parent(
            leaf(("a".to_string(), RetentionFlags::EPHEMERAL)),
            leaf(("b".to_string(), RetentionFlags::MARKED)),
        );
        assert_eq!(
            t.marked_positions(Address::from_parts(Level::from(1), 0)),
            BTreeSet::from([Position::from(1)])
        );

        let t0 = parent(t.clone(), t);
        assert_eq!(
            t0.marked_positions(Address::from_parts(Level::from(2), 1)),
            BTreeSet::from([Position::from(5), Position::from(7)])
        );
    }

    #[test]
    fn prune() {
        let t: PrunableTree<String> = parent(
            leaf(("a".to_string(), RetentionFlags::EPHEMERAL)),
            leaf(("b".to_string(), RetentionFlags::EPHEMERAL)),
        );

        assert_eq!(
            t.clone().prune(Level::from(1)),
            leaf(("ab".to_string(), RetentionFlags::EPHEMERAL))
        );

        let t0 = parent(leaf(("c".to_string(), RetentionFlags::MARKED)), t);
        assert_eq!(
            t0.prune(Level::from(2)),
            parent(
                leaf(("c".to_string(), RetentionFlags::MARKED)),
                leaf(("ab".to_string(), RetentionFlags::EPHEMERAL))
            )
        );
    }

    #[test]
    fn merge_checked() {
        let t0: PrunableTree<String> =
            parent(leaf(("a".to_string(), RetentionFlags::EPHEMERAL)), nil());

        let t1: PrunableTree<String> =
            parent(nil(), leaf(("b".to_string(), RetentionFlags::EPHEMERAL)));

        assert_eq!(
            t0.clone()
                .merge_checked(Address::from_parts(1.into(), 0), t1.clone()),
            Ok(leaf(("ab".to_string(), RetentionFlags::EPHEMERAL)))
        );

        let t2: PrunableTree<String> =
            parent(leaf(("c".to_string(), RetentionFlags::EPHEMERAL)), nil());
        assert_eq!(
            t0.clone()
                .merge_checked(Address::from_parts(1.into(), 0), t2.clone()),
            Err(MergeError::Conflict(Address::from_parts(0.into(), 0)))
        );

        let t3: PrunableTree<String> = parent(t0, t2);
        let t4: PrunableTree<String> = parent(t1.clone(), t1);

        assert_eq!(
            t3.merge_checked(Address::from_parts(2.into(), 0), t4),
            Ok(leaf(("abcb".to_string(), RetentionFlags::EPHEMERAL)))
        );
    }

    #[test]
    fn merge_checked_flags() {
        let t0: PrunableTree<String> = leaf(("a".to_string(), RetentionFlags::EPHEMERAL));
        let t1: PrunableTree<String> = leaf(("a".to_string(), RetentionFlags::MARKED));
        let t2: PrunableTree<String> = leaf(("a".to_string(), RetentionFlags::CHECKPOINT));

        assert_eq!(
            t0.merge_checked(Address::from_parts(1.into(), 0), t1.clone()),
            Ok(t1.clone()),
        );

        assert_eq!(
            t1.merge_checked(Address::from_parts(1.into(), 0), t2),
            Ok(leaf((
                "a".to_string(),
                RetentionFlags::MARKED | RetentionFlags::CHECKPOINT,
            ))),
        );
    }

    #[test]
    fn located_insert_subtree() {
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(3.into(), 1),
            root: parent(
                leaf(("abcd".to_string(), RetentionFlags::EPHEMERAL)),
                parent(nil(), leaf(("gh".to_string(), RetentionFlags::EPHEMERAL))),
            ),
        };

        assert_eq!(
            t.insert_subtree(
                LocatedTree {
                    root_addr: Address::from_parts(1.into(), 6),
                    root: parent(leaf(("e".to_string(), RetentionFlags::MARKED)), nil())
                },
                true
            ),
            Ok((
                LocatedTree {
                    root_addr: Address::from_parts(3.into(), 1),
                    root: parent(
                        leaf(("abcd".to_string(), RetentionFlags::EPHEMERAL)),
                        parent(
                            parent(leaf(("e".to_string(), RetentionFlags::MARKED)), nil()),
                            leaf(("gh".to_string(), RetentionFlags::EPHEMERAL))
                        )
                    )
                },
                vec![]
            ))
        );
    }

    #[test]
    fn located_insert_subtree_prevents_leaf_overwrite_conflict() {
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(2.into(), 1),
            root: parent(leaf(("a".to_string(), RetentionFlags::MARKED)), nil()),
        };

        let conflict_addr = Address::from_parts(1.into(), 2);
        assert_eq!(
            t.insert_subtree(
                LocatedTree {
                    root_addr: conflict_addr,
                    root: leaf(("b".to_string(), RetentionFlags::EPHEMERAL)),
                },
                false,
            ),
            Err(InsertionError::Conflict(conflict_addr)),
        );
    }

    #[test]
    fn located_witness() {
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(3.into(), 0),
            root: parent(
                leaf(("abcd".to_string(), RetentionFlags::EPHEMERAL)),
                parent(
                    parent(
                        leaf(("e".to_string(), RetentionFlags::MARKED)),
                        leaf(("f".to_string(), RetentionFlags::EPHEMERAL)),
                    ),
                    leaf(("gh".to_string(), RetentionFlags::EPHEMERAL)),
                ),
            ),
        };

        assert_eq!(
            t.witness(4.into(), 8.into()),
            Ok(vec!["f", "gh", "abcd"]
                .into_iter()
                .map(|s| s.to_string())
                .collect())
        );
        assert_eq!(
            t.witness(4.into(), 6.into()),
            Ok(vec!["f", "__", "abcd"]
                .into_iter()
                .map(|s| s.to_string())
                .collect())
        );
        assert_eq!(
            t.witness(4.into(), 7.into()),
            Err(QueryError::TreeIncomplete(vec![Address::from_parts(
                1.into(),
                3
            )]))
        );
    }

    #[test]
    fn frontier_empty_tree() {
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(2.into(), 0),
            root: nil(),
        };
        assert_eq!(t.frontier(), Ok(None));
    }

    #[test]
    fn frontier_single_leaf() {
        // A single leaf at position 0
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(0.into(), 0),
            root: leaf(("a".to_string(), RetentionFlags::EPHEMERAL)),
        };
        assert_eq!(
            t.frontier(),
            Ok(Some(
                NonEmptyFrontier::from_parts(Position::from(0), "a".to_string(), vec![]).unwrap()
            ))
        );
    }

    #[test]
    fn frontier_two_leaves() {
        // Positions 0 and 1 populated
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(1.into(), 0),
            root: parent(
                leaf(("a".to_string(), RetentionFlags::EPHEMERAL)),
                leaf(("b".to_string(), RetentionFlags::EPHEMERAL)),
            ),
        };
        // Position 1 = binary 1: ommer at level 0 = "a"
        assert_eq!(
            t.frontier(),
            Ok(Some(
                NonEmptyFrontier::from_parts(
                    Position::from(1),
                    "b".to_string(),
                    vec!["a".to_string()]
                )
                .unwrap()
            ))
        );
    }

    #[test]
    fn frontier_left_only() {
        // Only left child populated (position 0), right is Nil
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(1.into(), 0),
            root: parent(leaf(("a".to_string(), RetentionFlags::EPHEMERAL)), nil()),
        };
        // Position 0, no ommers
        assert_eq!(
            t.frontier(),
            Ok(Some(
                NonEmptyFrontier::from_parts(Position::from(0), "a".to_string(), vec![]).unwrap()
            ))
        );
    }

    #[test]
    fn frontier_deeper_tree() {
        // Positions 0-5 populated at level 3
        //           root
        //          /    \
        //       (l2,0)  (l2,1)
        //       / \      / \
        //     ab   cd   ef  Nil
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(3.into(), 0),
            root: parent(
                parent(
                    parent(
                        leaf(("a".to_string(), RetentionFlags::EPHEMERAL)),
                        leaf(("b".to_string(), RetentionFlags::EPHEMERAL)),
                    ),
                    parent(
                        leaf(("c".to_string(), RetentionFlags::EPHEMERAL)),
                        leaf(("d".to_string(), RetentionFlags::EPHEMERAL)),
                    ),
                ),
                parent(
                    parent(
                        leaf(("e".to_string(), RetentionFlags::EPHEMERAL)),
                        leaf(("f".to_string(), RetentionFlags::EPHEMERAL)),
                    ),
                    nil(),
                ),
            ),
        };

        // Position 5 = binary 101: ommers at levels 0 and 2
        // Level 0 ommer: "e" (sibling of "f")
        // Level 2 ommer: hash of left subtree = "abcd"
        assert_eq!(
            t.frontier(),
            Ok(Some(
                NonEmptyFrontier::from_parts(
                    Position::from(5),
                    "f".to_string(),
                    vec!["e".to_string(), "abcd".to_string()]
                )
                .unwrap()
            ))
        );
    }

    #[test]
    fn frontier_with_pruned_left_sibling() {
        // Left subtree is a pruned leaf (at level > 0), right subtree has detail
        //        root (level 3)
        //       /        \
        //   "abcd"     (l2,1)
        //  (pruned)     / \
        //             ef   Nil
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(3.into(), 0),
            root: parent(
                leaf(("abcd".to_string(), RetentionFlags::EPHEMERAL)),
                parent(
                    parent(
                        leaf(("e".to_string(), RetentionFlags::EPHEMERAL)),
                        leaf(("f".to_string(), RetentionFlags::EPHEMERAL)),
                    ),
                    nil(),
                ),
            ),
        };

        // The pruned left sibling's hash "abcd" should be used as the level-2 ommer
        assert_eq!(
            t.frontier(),
            Ok(Some(
                NonEmptyFrontier::from_parts(
                    Position::from(5),
                    "f".to_string(),
                    vec!["e".to_string(), "abcd".to_string()]
                )
                .unwrap()
            ))
        );
    }

    #[test]
    fn frontier_with_nil_left_sibling() {
        // Left subtree is Nil (incomplete), right has the frontier
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(2.into(), 0),
            root: parent(
                nil(),
                parent(
                    leaf(("c".to_string(), RetentionFlags::EPHEMERAL)),
                    leaf(("d".to_string(), RetentionFlags::EPHEMERAL)),
                ),
            ),
        };

        // Should return None because the left sibling is Nil (incomplete)
        assert_matches!(t.frontier(), Err(FrontierError::TreeIncomplete { .. }));
    }

    #[test]
    fn frontier_pruned_rightmost_subtree() {
        // Rightmost path hits a pruned leaf at level > 0
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(2.into(), 0),
            root: parent(
                parent(
                    leaf(("a".to_string(), RetentionFlags::EPHEMERAL)),
                    leaf(("b".to_string(), RetentionFlags::EPHEMERAL)),
                ),
                leaf(("cd".to_string(), RetentionFlags::EPHEMERAL)),
            ),
        };

        // Right child is a leaf at level 1 (pruned subtree); can't extract frontier
        assert_matches!(t.frontier(), Err(FrontierError::TreeIncomplete { .. }));
    }

    #[test]
    fn frontier_nonzero_index_shard() {
        // A shard at index 1 (positions 4-7): the absolute position has bits
        // above the shard level, so NonEmptyFrontier::from_parts will fail.
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(2.into(), 1),
            root: parent(
                parent(
                    leaf(("a".to_string(), RetentionFlags::EPHEMERAL)),
                    leaf(("b".to_string(), RetentionFlags::EPHEMERAL)),
                ),
                nil(),
            ),
        };

        // Position 5 (binary 101) needs 2 ommers, but we only have 1 within the shard
        // (level 0 ommer "a"). The bit at level 2 is set in position 5, requiring an
        // ommer we don't have. from_parts rejects this.
        assert_matches!(t.frontier(), Err(FrontierError::PositionMismatch { .. }));
    }

    #[test]
    fn frontier_roundtrip_via_insert() {
        use incrementalmerkletree::Retention;

        // Build a frontier, insert it into an empty tree, then extract it back
        let frontier = NonEmptyFrontier::from_parts(
            Position::from(5),
            "f".to_string(),
            vec!["e".to_string(), "abcd".to_string()],
        )
        .unwrap();

        let empty_tree: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(3.into(), 0),
            root: nil(),
        };

        let (filled_tree, _) = empty_tree
            .insert_frontier_nodes(frontier.clone(), &Retention::<()>::Ephemeral)
            .unwrap();

        assert_eq!(filled_tree.frontier(), Ok(Some(frontier)));
    }

    proptest! {
        #[test]
        fn clear_flags(
            root in arb_prunable_tree(arb_char_str(), 8, 2^6)
        ) {
            let root_addr = Address::from_parts(Level::from(7), 0);
            let tree = LocatedTree::from_parts(root_addr, root).unwrap();

            let (to_clear, to_retain) = tree.flag_positions().into_iter().enumerate().fold(
                (BTreeMap::new(), BTreeMap::new()),
                |(mut to_clear, mut to_retain), (i, (pos, flags))| {
                    if i % 2 == 0 {
                        to_clear.insert(pos, flags);
                    } else {
                        to_retain.insert(pos, flags);
                    }
                    (to_clear, to_retain)
                }
            );

            let pre_clearing_max_position = tree.max_position();
            let cleared = tree.clear_flags(to_clear);

            // Clearing flags should not modify the max position of leaves represented
            // in the shard.
            assert!(cleared.max_position() == pre_clearing_max_position);
            assert_eq!(to_retain, cleared.flag_positions());
        }
    }
}
