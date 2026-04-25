//! Helpers for working with trees that support pruning unneeded leaves and branches.

use std::collections::{BTreeMap, BTreeSet};
use std::sync::Arc;

use bitflags::bitflags;
use incrementalmerkletree::{
    frontier::NonEmptyFrontier, Address, Hashable, Level, Position, Retention,
};
use tracing::trace;

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
            Retention::Checkpoint { is_marked, .. } => {
                if *is_marked {
                    RetentionFlags::CHECKPOINT | RetentionFlags::MARKED
                } else {
                    RetentionFlags::CHECKPOINT
                }
            }
            Retention::Marked => RetentionFlags::MARKED,
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
    ///    values will be treated as `Nil`.
    pub fn root_hash(&self, root_addr: Address, truncate_at: Position) -> Result<H, Vec<Address>> {
        if truncate_at <= root_addr.position_range_start() {
            // we are in the part of the tree where we're generating empty roots,
            // so no need to inspect the tree
            Ok(H::empty_root(root_addr.level()))
        } else {
            match self {
                Tree(Node::Parent { ann, left, right }) => ann
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
                Tree(Node::Leaf { value }) => {
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
                Tree(Node::Nil) => Err(vec![root_addr]),
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
    pub fn merge_checked(self, root_addr: Address, other: Self) -> Result<Self, Address> {
        /// Pre-condition: `root_addr` must be the address of `t0` and `t1`.
        #[allow(clippy::type_complexity)]
        fn go<H: Hashable + Clone + PartialEq>(
            addr: Address,
            t0: PrunableTree<H>,
            t1: PrunableTree<H>,
        ) -> Result<PrunableTree<H>, Address> {
            // Require that any roots the we compute will not be default-filled by picking
            // a starting valid fill point that is outside the range of leaf positions.
            let no_default_fill = addr.position_range_end();
            match (t0, t1) {
                (Tree(Node::Nil), other) | (other, Tree(Node::Nil)) => Ok(other),
                (Tree(Node::Leaf { value: vl }), Tree(Node::Leaf { value: vr })) => {
                    if vl.0 == vr.0 {
                        // Merge the flags together.
                        Ok(Tree(Node::Leaf {
                            value: (vl.0, vl.1 | vr.1),
                        }))
                    } else {
                        trace!(left = ?vl.0, right = ?vr.0, "Merge conflict for leaves");
                        Err(addr)
                    }
                }
                (Tree(Node::Leaf { value }), parent @ Tree(Node::Parent { .. }))
                | (parent @ Tree(Node::Parent { .. }), Tree(Node::Leaf { value })) => {
                    let parent_hash = parent.root_hash(addr, no_default_fill);
                    if parent_hash.iter().all(|r| r == &value.0) {
                        Ok(parent.reannotate_root(Some(Arc::new(value.0))))
                    } else {
                        trace!(leaf = ?value, node = ?parent_hash, "Merge conflict for leaf into node");
                        Err(addr)
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
                            let (l_addr, r_addr) = addr
                                .children()
                                .expect("The root address of a parent node must have children.");
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
                        Err(addr)
                    }
                }
            }
        }

        trace!(this = ?self, other = ?other, "Merging subtrees");
        go(root_addr, self, other)
    }

    /// Unite two nodes by either constructing a new parent node, or, if both nodes are ephemeral
    /// leaves or Nil, constructing a replacement root by hashing leaf values together (or a
    /// replacement `Nil` value).
    ///
    /// `level` must be the level of the two nodes that are being joined.
    pub(crate) fn unite(level: Level, ann: Option<Arc<H>>, left: Self, right: Self) -> Self {
        match (left, right) {
            (Tree(Node::Nil), Tree(Node::Nil)) => Tree(Node::Nil),
            (Tree(Node::Leaf { value: lv }), Tree(Node::Leaf { value: rv }))
                // we can prune right-hand leaves that are not marked; if a leaf
                // is a checkpoint then that information will be propagated to
                // the replacement leaf
                if lv.1 == RetentionFlags::EPHEMERAL && (rv.1 & RetentionFlags::MARKED) == RetentionFlags::EPHEMERAL =>
            {
                Tree(
                    Node::Leaf {
                        value: (H::combine(level, &lv.0, &rv.0), rv.1),
                    },
                )
            }
            (left, right) => Tree(
                Node::Parent {
                    ann,
                    left: Arc::new(left),
                    right: Arc::new(right),
                },
            ),
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

impl<H: Hashable + Clone + PartialEq> LocatedPrunableTree<H> {
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
        self.root_hash(
            self.max_position()
                .map_or_else(|| self.root_addr.position_range_start(), |pos| pos + 1),
        )
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
                            Tree::unite(l_child.level(), ann.clone(), left, Tree(Node::Nil))
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
            is_complete: bool,
            contains_marked: bool,
        ) -> Result<(PrunableTree<H>, Vec<IncompleteAt>), InsertionError> {
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
                    node = LocatedTree {
                        root_addr: node.root_addr.parent(),
                        root: if node.root_addr.is_right_child() {
                            Tree(Node::Parent {
                                ann: None,
                                left: Arc::new(Tree(Node::Nil)),
                                right: Arc::new(node.root),
                            })
                        } else {
                            Tree(Node::Parent {
                                ann: None,
                                left: Arc::new(node.root),
                                right: Arc::new(Tree(Node::Nil)),
                            })
                        },
                    };
                }
                (node.root.reannotate_root(ann), incomplete)
            };

            trace!(
                "Node at {:?} contains subtree at {:?}",
                root_addr,
                subtree.root_addr(),
            );
            match into {
                Tree(Node::Nil) => Ok(replacement(None, subtree)),
                Tree(Node::Leaf { value: (value, _) }) => {
                    if root_addr == subtree.root_addr {
                        if is_complete {
                            // It is safe to replace the existing root unannotated, because we
                            // can always recompute the root from a complete subtree.
                            Ok((subtree.root, vec![]))
                        } else if subtree.root.node_value().iter().all(|v| v == &value) {
                            Ok((
                                // at this point we statically know the root to be a parent
                                subtree.root.reannotate_root(Some(Arc::new(value.clone()))),
                                vec![],
                            ))
                        } else {
                            trace!(
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
                        .map_err(InsertionError::Conflict)
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
                            go(l_addr, left.as_ref(), subtree, is_complete, contains_marked)?;
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
                        let (new_right, incomplete) = go(
                            r_addr,
                            right.as_ref(),
                            subtree,
                            is_complete,
                            contains_marked,
                        )?;
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

        let LocatedTree { root_addr, root } = self;
        if root_addr.contains(&subtree.root_addr) {
            let complete = subtree.root.is_complete();
            go(*root_addr, root, subtree, complete, contains_marked).map(|(root, incomplete)| {
                (
                    LocatedTree {
                        root_addr: *root_addr,
                        root,
                    },
                    incomplete,
                )
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
                    Ok((
                        r.subtree,
                        r.max_insert_position
                            .expect("Batch insertion result position is never initialized to None"),
                        checkpoint_id,
                    ))
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
        let mut subtree = Tree(Node::Leaf {
            value: (leaf, leaf_retention.into()),
        });

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
                match root {
                    Tree(Node::Parent { ann, left, right }) => {
                        let (l_addr, r_addr) = root_addr
                            .children()
                            .expect("has children because we checked `root` is a parent");

                        let p = to_clear.partition_point(|(p, _)| p < &l_addr.position_range_end());
                        trace!(
                            "In {:?}, partitioned: {:?} {:?}",
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
                    Tree(Node::Leaf { value: (h, r) }) => {
                        trace!("In {:?}, clearing {:?}", root_addr, to_clear);
                        // When we reach a leaf, we should be down to just a single position
                        // which should correspond to the last level-0 child of the address's
                        // subtree range; if it's a checkpoint this will always be the case for
                        // a partially-pruned branch, and if it's a marked node then it will
                        // be a level-0 leaf.
                        match to_clear {
                            [(pos, flags)] => {
                                assert_eq!(*pos, root_addr.max_position());
                                Tree(Node::Leaf {
                                    value: (h.clone(), *r & !*flags),
                                })
                            }
                            _ => {
                                panic!("Tree state inconsistent with checkpoints.");
                            }
                        }
                    }
                    Tree(Node::Nil) => Tree(Node::Nil),
                }
            }
        }

        let to_clear = to_clear.into_iter().collect::<Vec<_>>();
        Self {
            root_addr: self.root_addr,
            root: go(&to_clear, self.root_addr, &self.root),
        }
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
    use std::collections::BTreeSet;

    use incrementalmerkletree::{Address, Level, Position};

    use super::{LocatedPrunableTree, PrunableTree, RetentionFlags};
    use crate::{
        error::QueryError,
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
            Err(Address::from_parts(0.into(), 0))
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
    fn located_insert_subtree_leaf_overwrites() {
        let t: LocatedPrunableTree<String> = LocatedTree {
            root_addr: Address::from_parts(2.into(), 1),
            root: parent(leaf(("a".to_string(), RetentionFlags::MARKED)), nil()),
        };

        assert_eq!(
            t.insert_subtree(
                LocatedTree {
                    root_addr: Address::from_parts(1.into(), 2),
                    root: leaf(("b".to_string(), RetentionFlags::EPHEMERAL)),
                },
                false,
            ),
            Ok((
                LocatedTree {
                    root_addr: Address::from_parts(2.into(), 1),
                    root: parent(leaf(("b".to_string(), RetentionFlags::EPHEMERAL)), nil()),
                },
                vec![],
            )),
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
}
