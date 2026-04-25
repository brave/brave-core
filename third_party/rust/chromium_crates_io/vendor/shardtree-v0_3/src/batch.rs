//! Helpers for inserting many leaves into a tree at once.

use std::{collections::BTreeMap, fmt, ops::Range, sync::Arc};

use incrementalmerkletree::{Address, Hashable, Level, Position, Retention};
use tracing::trace;

use crate::{
    error::{InsertionError, ShardTreeError},
    store::{Checkpoint, ShardStore},
    IncompleteAt, LocatedPrunableTree, LocatedTree, Node, RetentionFlags, ShardTree, Tree,
};

impl<
        H: Hashable + Clone + PartialEq,
        C: Clone + fmt::Debug + Ord,
        S: ShardStore<H = H, CheckpointId = C>,
        const DEPTH: u8,
        const SHARD_HEIGHT: u8,
    > ShardTree<S, DEPTH, SHARD_HEIGHT>
{
    /// Put a range of values into the subtree to fill leaves starting from the given position.
    ///
    /// This operation will pad the tree until it contains enough subtrees to reach the starting
    /// position. It will fully consume the provided iterator, constructing successive subtrees
    /// until no more values are available. It aggressively prunes the tree as it goes, retaining
    /// only nodes that either have [`Retention::Marked`] retention, are required to construct a
    /// witness for such marked nodes, or that must be retained in order to make it possible to
    /// truncate the tree to any position with [`Retention::Checkpoint`] retention.
    ///
    /// This operation returns the final position at which a leaf was inserted, and the vector of
    /// [`IncompleteAt`] values that identify addresses at which [`Node::Nil`] nodes were
    /// introduced to the tree, as well as whether or not those newly introduced nodes will need to
    /// be filled with values in order to produce witnesses for inserted leaves with
    /// [`Retention::Marked`] retention.
    ///
    /// This method operates on a single thread. If you have parallelism available, consider using
    /// [`LocatedPrunableTree::from_iter`] and [`Self::insert_tree`] instead.
    #[allow(clippy::type_complexity)]
    pub fn batch_insert<I: Iterator<Item = (H, Retention<C>)>>(
        &mut self,
        mut start: Position,
        values: I,
    ) -> Result<Option<(Position, Vec<IncompleteAt>)>, ShardTreeError<S::Error>> {
        trace!("Batch inserting from {:?}", start);
        let mut values = values.peekable();
        let mut subtree_root_addr = Self::subtree_addr(start);
        let mut max_insert_position = None;
        let mut all_incomplete = vec![];
        loop {
            if values.peek().is_some() {
                let mut res = self
                    .store
                    .get_shard(subtree_root_addr)
                    .map_err(ShardTreeError::Storage)?
                    .unwrap_or_else(|| LocatedTree::empty(subtree_root_addr))
                    .batch_insert(start, values)?
                    .expect(
                        "Iterator containing leaf values to insert was verified to be nonempty.",
                    );
                self.store
                    .put_shard(res.subtree)
                    .map_err(ShardTreeError::Storage)?;
                for (id, position) in res.checkpoints.into_iter() {
                    self.store
                        .add_checkpoint(id, Checkpoint::at_position(position))
                        .map_err(ShardTreeError::Storage)?;
                }

                values = res.remainder;
                subtree_root_addr = subtree_root_addr.next_at_level();
                max_insert_position = res.max_insert_position;
                start = max_insert_position.unwrap() + 1;
                all_incomplete.append(&mut res.incomplete);
            } else {
                break;
            }
        }

        self.prune_excess_checkpoints()?;
        Ok(max_insert_position.map(|p| (p, all_incomplete)))
    }
}

/// A type for the result of a batch insertion operation.
///
/// This result type contains the newly constructed tree, the addresses any new incomplete internal
/// nodes within that tree that were introduced as a consequence of that insertion, and the
/// remainder of the iterator that provided the inserted values.
#[derive(Debug)]
pub struct BatchInsertionResult<H, C: Ord, I: Iterator<Item = (H, Retention<C>)>> {
    /// The updated tree after all insertions have been performed.
    pub subtree: LocatedPrunableTree<H>,
    /// A flag identifying whether the constructed subtree contains a marked node.
    pub contains_marked: bool,
    /// The vector of addresses of [`Node::Nil`] nodes that were inserted into the tree as part of
    /// the insertion operation, for nodes that are required in order to construct a witness for
    /// each inserted leaf with [`Retention::Marked`] retention.
    pub incomplete: Vec<IncompleteAt>,
    /// The maximum position at which a leaf was inserted.
    pub max_insert_position: Option<Position>,
    /// The positions of all leaves with [`Retention::Checkpoint`] retention that were inserted.
    pub checkpoints: BTreeMap<C, Position>,
    /// The unconsumed remainder of the iterator from which leaves were inserted, if the tree
    /// was completely filled before the iterator was fully consumed.
    pub remainder: I,
}

impl<H: Hashable + Clone + PartialEq> LocatedPrunableTree<H> {
    /// Append a values from an iterator, beginning at the first available position in the tree.
    ///
    /// Returns an error if the tree is full. If the position at the end of the iterator is outside
    /// of the subtree's range, the unconsumed part of the iterator will be returned as part of
    /// the result.
    pub fn batch_append<C: Clone + Ord, I: Iterator<Item = (H, Retention<C>)>>(
        &self,
        values: I,
    ) -> Result<Option<BatchInsertionResult<H, C, I>>, InsertionError> {
        let append_position = self
            .max_position()
            .map(|p| p + 1)
            .unwrap_or_else(|| self.root_addr.position_range_start());
        self.batch_insert(append_position, values)
    }

    /// Put a range of values into the subtree by consuming the given iterator, starting at the
    /// specified position.
    ///
    /// The start position must exist within the position range of this subtree. If the position at
    /// the end of the iterator is outside of the subtree's range, the unconsumed part of the
    /// iterator will be returned as part of the result.
    ///
    /// Returns `Ok(None)` if the provided iterator is empty, `Ok(Some(BatchInsertionResult))` if
    /// values were successfully inserted, or an error if the start position provided is outside
    /// of this tree's position range or if a conflict with an existing subtree root is detected.
    pub fn batch_insert<C: Clone + Ord, I: Iterator<Item = (H, Retention<C>)>>(
        &self,
        start: Position,
        values: I,
    ) -> Result<Option<BatchInsertionResult<H, C, I>>, InsertionError> {
        trace!("Batch inserting into {:?} from {:?}", self.root_addr, start);
        let subtree_range = self.root_addr.position_range();
        let contains_start = subtree_range.contains(&start);
        if contains_start {
            let position_range = Range {
                start,
                end: subtree_range.end,
            };
            Self::from_iter(position_range, self.root_addr.level(), values)
                .map(|mut res| {
                    let (subtree, mut incomplete) = self
                        .clone()
                        .insert_subtree(res.subtree, res.contains_marked)?;
                    res.subtree = subtree;
                    res.incomplete.append(&mut incomplete);
                    Ok(res)
                })
                .transpose()
        } else {
            Err(InsertionError::OutOfRange(start, subtree_range))
        }
    }

    /// Builds a [`LocatedPrunableTree`] from an iterator of level-0 leaves.
    ///
    /// This may be used in conjunction with [`ShardTree::insert_tree`] to support
    /// partially-parallelizable tree construction. Multiple subtrees may be constructed in
    /// parallel from iterators over (preferably, though not necessarily) disjoint leaf ranges, and
    /// [`ShardTree::insert_tree`] may be used to insert those subtrees into the [`ShardTree`] in
    /// arbitrary order.
    ///
    /// # Parameters:
    /// * `position_range` - The range of leaf positions at which values will be inserted. This
    ///   range is also used to place an upper bound on the number of items that will be consumed
    ///   from the `values` iterator.
    /// * `prune_below` - Nodes with [`Retention::Ephemeral`] retention that are not required to be retained
    ///   in order to construct a witness for a marked node or to make it possible to rewind to a
    ///   checkpointed node may be pruned so long as their address is at less than the specified
    ///   level.
    /// * `values` - The iterator of `(H, Retention)` pairs from which to construct the tree.
    pub fn from_iter<C: Clone + Ord, I: Iterator<Item = (H, Retention<C>)>>(
        position_range: Range<Position>,
        prune_below: Level,
        mut values: I,
    ) -> Option<BatchInsertionResult<H, C, I>> {
        trace!(
            position_range = ?position_range,
            prune_below = ?prune_below,
            "Creating minimal tree for insertion"
        );

        // A stack of complete subtrees to be inserted as descendants into the subtree labeled
        // with the addresses at which they will be inserted, along with their root hashes.
        let mut fragments: Vec<(Self, bool)> = vec![];
        let mut position = position_range.start;
        let mut checkpoints: BTreeMap<C, Position> = BTreeMap::new();
        while position < position_range.end {
            if let Some((value, retention)) = values.next() {
                if let Retention::Checkpoint { id, .. } = &retention {
                    checkpoints.insert(id.clone(), position);
                }

                let rflags = RetentionFlags::from(retention);
                let mut subtree = LocatedTree {
                    root_addr: Address::from(position),
                    root: Tree(Node::Leaf {
                        value: (value.clone(), rflags),
                    }),
                };

                if position.is_right_child() {
                    // At right-hand positions, we are completing a subtree and so we unite
                    // fragments up the stack until we get the largest possible subtree
                    while let Some((potential_sibling, marked)) = fragments.pop() {
                        if potential_sibling.root_addr.parent() == subtree.root_addr.parent() {
                            subtree = unite(potential_sibling, subtree, prune_below);
                        } else {
                            // this is not a sibling node, so we push it back on to the stack
                            // and are done
                            fragments.push((potential_sibling, marked));
                            break;
                        }
                    }
                }

                fragments.push((subtree, rflags.is_marked()));
                position += 1;
            } else {
                break;
            }
        }
        trace!("Initial fragments: {:?}", fragments);

        if position > position_range.start {
            let last_position = position - 1;
            let minimal_tree_addr =
                Address::from(position_range.start).common_ancestor(&last_position.into());
            trace!("Building minimal tree at {:?}", minimal_tree_addr);
            build_minimal_tree(fragments, minimal_tree_addr, prune_below).map(
                |(to_insert, contains_marked, incomplete)| BatchInsertionResult {
                    subtree: to_insert,
                    contains_marked,
                    incomplete,
                    max_insert_position: Some(last_position),
                    checkpoints,
                    remainder: values,
                },
            )
        } else {
            None
        }
    }
}

// Unite two subtrees by either adding a parent node, or a leaf containing the Merkle root
// of such a parent if both nodes are ephemeral leaves.
//
// `unite` is only called when both root addrs have the same parent.  `batch_insert` never
// constructs Nil nodes, so we don't create any incomplete root information here.
fn unite<H: Hashable + Clone + PartialEq>(
    lroot: LocatedPrunableTree<H>,
    rroot: LocatedPrunableTree<H>,
    prune_below: Level,
) -> LocatedPrunableTree<H> {
    assert_eq!(lroot.root_addr.parent(), rroot.root_addr.parent());
    LocatedTree {
        root_addr: lroot.root_addr.parent(),
        root: if lroot.root_addr.level() < prune_below {
            Tree::unite(lroot.root_addr.level(), None, lroot.root, rroot.root)
        } else {
            Tree(Node::Parent {
                ann: None,
                left: Arc::new(lroot.root),
                right: Arc::new(rroot.root),
            })
        },
    }
}

/// Combines the given subtree with an empty sibling node to obtain the next level
/// subtree.
///
/// `expect_left_child` is set to a constant at each callsite, to ensure that this
/// function is only called on either the left-most or right-most subtree.
fn combine_with_empty<H: Hashable + Clone + PartialEq>(
    root: LocatedPrunableTree<H>,
    expect_left_child: bool,
    incomplete: &mut Vec<IncompleteAt>,
    contains_marked: bool,
    prune_below: Level,
) -> LocatedPrunableTree<H> {
    assert_eq!(expect_left_child, root.root_addr.is_left_child());
    let sibling_addr = root.root_addr.sibling();
    incomplete.push(IncompleteAt {
        address: sibling_addr,
        required_for_witness: contains_marked,
    });
    let sibling = LocatedTree {
        root_addr: sibling_addr,
        root: Tree(Node::Nil),
    };
    let (lroot, rroot) = if root.root_addr.is_left_child() {
        (root, sibling)
    } else {
        (sibling, root)
    };
    unite(lroot, rroot, prune_below)
}

// Builds a single tree from the provided stack of subtrees, which must be non-overlapping
// and in position order. Returns the resulting tree, a flag indicating whether the
// resulting tree contains a `MARKED` node, and the vector of [`IncompleteAt`] values for
// [`Node::Nil`] nodes that were introduced in the process of constructing the tree.
fn build_minimal_tree<H: Hashable + Clone + PartialEq>(
    mut xs: Vec<(LocatedPrunableTree<H>, bool)>,
    root_addr: Address,
    prune_below: Level,
) -> Option<(LocatedPrunableTree<H>, bool, Vec<IncompleteAt>)> {
    // First, consume the stack from the right, building up a single tree
    // until we can't combine any more.
    if let Some((mut cur, mut contains_marked)) = xs.pop() {
        let mut incomplete = vec![];
        while let Some((top, top_marked)) = xs.pop() {
            while cur.root_addr.level() < top.root_addr.level() {
                cur = combine_with_empty(cur, true, &mut incomplete, top_marked, prune_below);
            }

            if cur.root_addr.level() == top.root_addr.level() {
                contains_marked = contains_marked || top_marked;
                if cur.root_addr.is_right_child() {
                    // We have a left child and a right child, so unite them.
                    cur = unite(top, cur, prune_below);
                } else {
                    // This is a left child, so we build it up one more level and then
                    // we've merged as much as we can from the right and need to work from
                    // the left
                    xs.push((top, top_marked));
                    cur = combine_with_empty(cur, true, &mut incomplete, top_marked, prune_below);
                    break;
                }
            } else {
                // top.root_addr.level < cur.root_addr.level, so we've merged as much as we
                // can from the right and now need to work from the left.
                xs.push((top, top_marked));
                break;
            }
        }

        // Ensure we can work from the left in a single pass by making this right-most subtree
        while cur.root_addr.level() + 1 < root_addr.level() {
            cur = combine_with_empty(cur, true, &mut incomplete, contains_marked, prune_below);
        }

        // push our accumulated max-height right hand node back on to the stack.
        xs.push((cur, contains_marked));

        // From the stack of subtrees, construct a single sparse tree that can be
        // inserted/merged into the existing tree
        let res_tree = xs.into_iter().fold(
            None,
            |acc: Option<LocatedPrunableTree<H>>, (next_tree, next_marked)| {
                if let Some(mut prev_tree) = acc {
                    // add nil branches to build up the left tree until we can merge it
                    // with the right
                    while prev_tree.root_addr.level() < next_tree.root_addr.level() {
                        contains_marked = contains_marked || next_marked;
                        prev_tree = combine_with_empty(
                            prev_tree,
                            false,
                            &mut incomplete,
                            next_marked,
                            prune_below,
                        );
                    }

                    Some(unite(prev_tree, next_tree, prune_below))
                } else {
                    Some(next_tree)
                }
            },
        );

        res_tree.map(|t| (t, contains_marked, incomplete))
    } else {
        None
    }
}

#[cfg(test)]
mod tests {
    use std::iter;

    use incrementalmerkletree::{Address, Level, Position, Retention};

    use super::{LocatedPrunableTree, RetentionFlags};
    use crate::{
        store::memory::MemoryShardStore,
        tree::tests::{leaf, nil, parent},
        BatchInsertionResult, ShardTree,
    };

    #[test]
    fn located_from_iter_non_sibling_adjacent() {
        let res = LocatedPrunableTree::from_iter::<(), _>(
            Position::from(3)..Position::from(5),
            Level::new(0),
            vec![
                ("d".to_string(), Retention::Ephemeral),
                ("e".to_string(), Retention::Ephemeral),
            ]
            .into_iter(),
        )
        .unwrap();
        assert_eq!(
            res.subtree,
            LocatedPrunableTree {
                root_addr: Address::from_parts(3.into(), 0),
                root: parent(
                    parent(
                        nil(),
                        parent(nil(), leaf(("d".to_string(), RetentionFlags::EPHEMERAL)))
                    ),
                    parent(
                        parent(leaf(("e".to_string(), RetentionFlags::EPHEMERAL)), nil()),
                        nil()
                    )
                )
            },
        );
    }

    #[test]
    fn located_insert() {
        let tree = LocatedPrunableTree::empty(Address::from_parts(Level::from(2), 0));
        let (base, _, _) = tree
            .append::<()>("a".to_string(), Retention::Ephemeral)
            .unwrap();
        assert_eq!(base.right_filled_root(), Ok("a___".to_string()));

        // Perform an in-order insertion.
        let (in_order, pos, _) = base
            .append::<()>("b".to_string(), Retention::Ephemeral)
            .unwrap();
        assert_eq!(pos, 1.into());
        assert_eq!(in_order.right_filled_root(), Ok("ab__".to_string()));

        // On the same tree, perform an out-of-order insertion.
        let out_of_order = base
            .batch_insert::<(), _>(
                Position::from(3),
                vec![("d".to_string(), Retention::Ephemeral)].into_iter(),
            )
            .unwrap()
            .unwrap();
        assert_eq!(
            out_of_order.subtree,
            LocatedPrunableTree {
                root_addr: Address::from_parts(2.into(), 0),
                root: parent(
                    parent(leaf(("a".to_string(), RetentionFlags::EPHEMERAL)), nil()),
                    parent(nil(), leaf(("d".to_string(), RetentionFlags::EPHEMERAL)))
                )
            }
        );

        let complete = out_of_order
            .subtree
            .batch_insert::<(), _>(
                Position::from(1),
                vec![
                    ("b".to_string(), Retention::Ephemeral),
                    ("c".to_string(), Retention::Ephemeral),
                ]
                .into_iter(),
            )
            .unwrap()
            .unwrap();
        assert_eq!(complete.subtree.right_filled_root(), Ok("abcd".to_string()));
    }

    #[allow(clippy::type_complexity)]
    pub(super) fn build_insert_tree_and_batch_insert(
        leaves: Vec<(String, Retention<usize>)>,
    ) -> (
        ShardTree<MemoryShardStore<String, usize>, 6, 3>,
        ShardTree<MemoryShardStore<String, usize>, 6, 3>,
    ) {
        let max_checkpoints = 10;
        let start = Position::from(0);
        let end = start + leaves.len() as u64;

        // Construct a tree using `ShardTree::insert_tree`.
        let mut left = ShardTree::new(MemoryShardStore::empty(), max_checkpoints);
        if let Some(BatchInsertionResult {
            subtree,
            checkpoints,
            mut remainder,
            ..
        }) = LocatedPrunableTree::from_iter(start..end, 0.into(), leaves.clone().into_iter())
        {
            assert_eq!(remainder.next(), None);
            left.insert_tree(subtree, checkpoints).unwrap();
        }

        // Construct a tree using `ShardTree::batch_insert`.
        let mut right = ShardTree::new(MemoryShardStore::empty(), max_checkpoints);
        right.batch_insert(start, leaves.into_iter()).unwrap();

        (left, right)
    }

    #[test]
    fn batch_insert_matches_insert_tree() {
        {
            let (lhs, rhs) = build_insert_tree_and_batch_insert(vec![]);
            assert_eq!(lhs.max_leaf_position(0), Ok(None));
            assert_eq!(rhs.max_leaf_position(0), Ok(None));
        }

        for i in 0..64 {
            let num_leaves = i + 1;
            let leaves = iter::repeat(("a".into(), Retention::Ephemeral))
                .take(num_leaves)
                .collect();
            let expected_root = (0..64)
                .map(|c| if c < num_leaves { 'a' } else { '_' })
                .fold(String::with_capacity(64), |mut acc, c| {
                    acc.push(c);
                    acc
                });

            let (lhs, rhs) = build_insert_tree_and_batch_insert(leaves);
            assert_eq!(lhs.max_leaf_position(0), Ok(Some(Position::from(i as u64))));
            assert_eq!(rhs.max_leaf_position(0), Ok(Some(Position::from(i as u64))));

            assert_eq!(lhs.root_at_checkpoint_depth(0).unwrap(), expected_root);
            assert_eq!(rhs.root_at_checkpoint_depth(0).unwrap(), expected_root);
        }
    }
}

#[cfg(test)]
mod proptests {
    use proptest::prelude::*;

    use super::tests::build_insert_tree_and_batch_insert;
    use crate::testing::{arb_char_str, arb_leaves};

    proptest! {
        #[test]
        fn batch_insert_matches_insert_tree(
            leaves in arb_leaves(arb_char_str())
        ) {
            let (left, right) = build_insert_tree_and_batch_insert(leaves);

            // Check that the resulting trees are equal.
            assert_eq!(left.root_at_checkpoint_depth(0), right.root_at_checkpoint_depth(0));
        }
    }
}
