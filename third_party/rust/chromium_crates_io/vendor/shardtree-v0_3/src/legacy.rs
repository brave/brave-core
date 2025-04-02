use std::fmt;

use incrementalmerkletree::{witness::IncrementalWitness, Address, Hashable, Level, Retention};

use crate::{
    store::ShardStore, InsertionError, LocatedPrunableTree, LocatedTree, PrunableTree,
    RetentionFlags, ShardTree, ShardTreeError, Tree,
};

impl<
        H: Hashable + Clone + PartialEq,
        C: Clone + fmt::Debug + Ord,
        S: ShardStore<H = H, CheckpointId = C>,
        const DEPTH: u8,
        const SHARD_HEIGHT: u8,
    > ShardTree<S, DEPTH, SHARD_HEIGHT>
{
    /// Add the leaf and ommers of the provided witness as nodes within the subtree corresponding
    /// to the frontier's position, and update the cap to include the nodes of the witness at
    /// levels greater than or equal to the shard height. Also, if the witness spans multiple
    /// subtrees, update the subtree corresponding to the current witness "tip" accordingly.
    pub fn insert_witness_nodes(
        &mut self,
        witness: IncrementalWitness<H, DEPTH>,
        checkpoint_id: S::CheckpointId,
    ) -> Result<(), ShardTreeError<S::Error>> {
        let leaf_position = witness.witnessed_position();
        let subtree_root_addr = Address::above_position(Self::subtree_level(), leaf_position);

        let shard = self
            .store
            .get_shard(subtree_root_addr)
            .map_err(ShardTreeError::Storage)?
            .unwrap_or_else(|| LocatedTree::empty(subtree_root_addr));

        let (updated_subtree, supertree, tip_subtree) =
            shard.insert_witness_nodes(witness, checkpoint_id)?;

        self.store
            .put_shard(updated_subtree)
            .map_err(ShardTreeError::Storage)?;

        if let Some(supertree) = supertree {
            let new_cap = LocatedTree {
                root_addr: Self::root_addr(),
                root: self.store.get_cap().map_err(ShardTreeError::Storage)?,
            }
            .insert_subtree(supertree, true)?;

            self.store
                .put_cap(new_cap.0.root)
                .map_err(ShardTreeError::Storage)?;
        }

        if let Some(tip_subtree) = tip_subtree {
            let tip_subtree_addr = Address::above_position(
                Self::subtree_level(),
                tip_subtree.root_addr().position_range_start(),
            );

            let tip_shard = self
                .store
                .get_shard(tip_subtree_addr)
                .map_err(ShardTreeError::Storage)?
                .unwrap_or_else(|| LocatedTree::empty(tip_subtree_addr));

            self.store
                .put_shard(tip_shard.insert_subtree(tip_subtree, false)?.0)
                .map_err(ShardTreeError::Storage)?;
        }

        Ok(())
    }
}

/// Operations on [`LocatedTree`]s that are annotated with Merkle hashes.
impl<H: Hashable + Clone + PartialEq> LocatedPrunableTree<H> {
    fn combine_optional(
        opt_t0: Option<Self>,
        opt_t1: Option<Self>,
        contains_marked: bool,
    ) -> Result<Option<Self>, InsertionError> {
        match (opt_t0, opt_t1) {
            (Some(t0), Some(t1)) => {
                let into = LocatedTree {
                    root_addr: t0.root_addr().common_ancestor(&t1.root_addr()),
                    root: Tree::empty(),
                };

                into.insert_subtree(t0, contains_marked)
                    .and_then(|(into, _)| into.insert_subtree(t1, contains_marked))
                    .map(|(t, _)| Some(t))
            }
            (t0, t1) => Ok(t0.or(t1)),
        }
    }

    fn from_witness_filled_nodes(
        leaf_addr: Address,
        mut filled: impl Iterator<Item = H>,
        split_at: Level,
    ) -> (Self, Option<Self>) {
        // add filled nodes to the subtree; here, we do not need to worry about
        // whether or not these nodes can be invalidated by a rewind
        let mut addr = leaf_addr;
        let mut subtree = Tree::empty();
        while addr.level() < split_at {
            if addr.is_left_child() {
                // the current  root is a left child, so take the right sibling from the
                // filled iterator
                if let Some(right) = filled.next() {
                    // once we have a right-hand node, add a parent with the current tree
                    // as the left-hand sibling
                    subtree = Tree::parent(
                        None,
                        subtree,
                        Tree::leaf((right.clone(), RetentionFlags::EPHEMERAL)),
                    );
                } else {
                    break;
                }
            } else {
                // the current address is for a right child, so add an empty left sibling
                subtree = Tree::parent(None, Tree::empty(), subtree);
            }

            addr = addr.parent();
        }

        let subtree = LocatedTree {
            root_addr: addr,
            root: subtree,
        };

        // add filled nodes to the supertree
        let supertree = if addr.level() == split_at {
            let mut supertree = None;
            for right in filled {
                // build up the right-biased tree until we get a left-hand node
                while addr.is_right_child() {
                    supertree = supertree.map(|t| Tree::parent(None, Tree::empty(), t));
                    addr = addr.parent();
                }

                // once we have a left-hand root, add a parent with the current ommer as the right-hand sibling
                supertree = Some(Tree::parent(
                    None,
                    supertree.unwrap_or_else(PrunableTree::empty),
                    Tree::leaf((right.clone(), RetentionFlags::EPHEMERAL)),
                ));
                addr = addr.parent();
            }

            supertree.map(|t| LocatedTree {
                root_addr: addr,
                root: t,
            })
        } else {
            None
        };

        (subtree, supertree)
    }

    /// Insert the nodes belonging to the given incremental witness to this tree, truncating the
    /// witness to the given position.
    ///
    /// Returns a copy of this tree updated to include the witness nodes, any partial supertree that is
    /// produced from nodes "higher" in the witness tree
    ///
    /// # Panics
    ///
    /// Panics if `witness` corresponds to the empty tree.
    pub fn insert_witness_nodes<C, const DEPTH: u8>(
        &self,
        witness: IncrementalWitness<H, DEPTH>,
        checkpoint_id: C,
    ) -> Result<(Self, Option<Self>, Option<Self>), InsertionError> {
        let subtree_range = self.root_addr.position_range();
        if subtree_range.contains(&witness.witnessed_position()) {
            // construct the subtree and cap based on the frontier containing the
            // witnessed position
            let (past_subtree, past_supertree) = self.insert_frontier_nodes::<C>(
                witness
                    .tree()
                    .to_frontier()
                    .take()
                    .expect("IncrementalWitness must not be created from the empty tree."),
                &Retention::Marked,
            )?;

            // construct subtrees from the `filled` nodes of the witness
            let (future_subtree, future_supertree) = Self::from_witness_filled_nodes(
                Address::from(witness.witnessed_position()),
                witness.filled().iter().cloned(),
                self.root_addr.level(),
            );

            // construct subtrees from the `cursor` part of the witness
            let cursor_trees = witness.cursor().as_ref().filter(|c| c.size() > 0).map(|c| {
                Self::from_frontier_parts(
                    witness.tip_position(),
                    c.leaf()
                        .cloned()
                        .expect("Cannot have an empty leaf for a non-empty tree"),
                    c.ommers_iter().cloned(),
                    &Retention::Checkpoint {
                        id: checkpoint_id,
                        is_marked: false,
                    },
                    self.root_addr.level(),
                )
            });

            let (subtree, _) = past_subtree.insert_subtree(future_subtree, true)?;

            let supertree =
                LocatedPrunableTree::combine_optional(past_supertree, future_supertree, true)?;

            Ok(if let Some((cursor_sub, cursor_super)) = cursor_trees {
                let (complete_subtree, fragment) =
                    if subtree.root_addr().contains(&cursor_sub.root_addr()) {
                        // the cursor subtree can be absorbed into the current subtree
                        (subtree.insert_subtree(cursor_sub, false)?.0, None)
                    } else {
                        // the cursor subtree must be maintained separately
                        (subtree, Some(cursor_sub))
                    };

                let complete_supertree =
                    LocatedPrunableTree::combine_optional(supertree, cursor_super, false)?;

                (complete_subtree, complete_supertree, fragment)
            } else {
                (subtree, supertree, None)
            })
        } else {
            Err(InsertionError::OutOfRange(
                witness.witnessed_position(),
                subtree_range,
            ))
        }
    }
}

#[cfg(test)]
mod tests {
    use assert_matches::assert_matches;

    use incrementalmerkletree::{
        frontier::CommitmentTree, witness::IncrementalWitness, Address, Level, Position,
    };

    use crate::{LocatedPrunableTree, RetentionFlags, Tree};

    #[test]
    fn insert_witness_nodes() {
        let mut base_tree = CommitmentTree::<String, 6>::empty();
        for c in 'a'..'h' {
            base_tree.append(c.to_string()).unwrap();
        }
        let mut witness = IncrementalWitness::from_tree(base_tree);
        for c in 'h'..'z' {
            witness.append(c.to_string()).unwrap();
        }

        let root_addr = Address::from_parts(Level::from(3), 0);
        let tree = LocatedPrunableTree::empty(root_addr);
        let result = tree.insert_witness_nodes(witness, 3usize);
        assert_matches!(result, Ok((ref _t, Some(ref _c), Some(ref _r))));

        if let Ok((t, Some(c), Some(r))) = result {
            // verify that we can find the "marked" leaf
            assert_eq!(
                t.root.root_hash(root_addr, Position::from(7)),
                Ok("abcdefg_".to_string())
            );

            assert_eq!(
                c.root,
                Tree::parent(
                    None,
                    Tree::parent(
                        None,
                        Tree::empty(),
                        Tree::leaf(("ijklmnop".to_string(), RetentionFlags::EPHEMERAL)),
                    ),
                    Tree::parent(
                        None,
                        Tree::leaf(("qrstuvwx".to_string(), RetentionFlags::EPHEMERAL)),
                        Tree::empty()
                    )
                )
            );

            assert_eq!(
                r.root
                    .root_hash(Address::from_parts(Level::from(3), 3), Position::from(25)),
                Ok("y_______".to_string())
            );
        }
    }

    #[test]
    fn insert_witness_nodes_sub_shard_height() {
        let mut base_tree = CommitmentTree::<String, 6>::empty();
        for c in 'a'..='c' {
            base_tree.append(c.to_string()).unwrap();
        }
        let mut witness = IncrementalWitness::from_tree(base_tree);
        witness.append("d".to_string()).unwrap();

        let root_addr = Address::from_parts(Level::from(3), 0);
        let tree = LocatedPrunableTree::empty(root_addr);
        let result = tree.insert_witness_nodes(witness, 3usize);
        assert_matches!(result, Ok((ref _t, None, None)));

        if let Ok((t, None, None)) = result {
            // verify that we can find the "marked" leaf
            assert_eq!(
                t.root.root_hash(root_addr, Position::from(3)),
                Ok("abc_____".to_string())
            );
        }
    }
}
