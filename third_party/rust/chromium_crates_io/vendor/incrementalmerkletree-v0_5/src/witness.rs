use std::convert::TryInto;
use std::iter::repeat;

use crate::{
    frontier::{CommitmentTree, PathFiller},
    Hashable, Level, MerklePath, Position, Source,
};

/// An updatable witness to a path from a position in a particular [`CommitmentTree`].
///
/// Appending the same commitments in the same order to both the original
/// [`CommitmentTree`] and this `IncrementalWitness` will result in a witness to the path
/// from the target position to the root of the updated tree.
///
/// # Examples
///
/// ```
/// use incrementalmerkletree::{
///     frontier::{CommitmentTree, testing::TestNode},
///     witness::IncrementalWitness,
///     Position
/// };
///
/// let mut tree = CommitmentTree::<TestNode, 8>::empty();
///
/// tree.append(TestNode(0));
/// tree.append(TestNode(1));
/// let mut witness = IncrementalWitness::from_tree(tree.clone());
/// assert_eq!(witness.witnessed_position(), Position::from(1));
/// assert_eq!(tree.root(), witness.root());
///
/// let next = TestNode(2);
/// tree.append(next.clone());
/// witness.append(next);
/// assert_eq!(tree.root(), witness.root());
/// ```
#[derive(Clone, Debug)]
pub struct IncrementalWitness<H, const DEPTH: u8> {
    tree: CommitmentTree<H, DEPTH>,
    filled: Vec<H>,
    cursor_depth: u8,
    cursor: Option<CommitmentTree<H, DEPTH>>,
}

impl<H, const DEPTH: u8> IncrementalWitness<H, DEPTH> {
    /// Creates an `IncrementalWitness` for the most recent commitment added to the given
    /// [`CommitmentTree`].
    pub fn from_tree(tree: CommitmentTree<H, DEPTH>) -> Self {
        IncrementalWitness {
            tree,
            filled: vec![],
            cursor_depth: 0,
            cursor: None,
        }
    }

    pub fn from_parts(
        tree: CommitmentTree<H, DEPTH>,
        filled: Vec<H>,
        cursor: Option<CommitmentTree<H, DEPTH>>,
    ) -> Self {
        let mut witness = IncrementalWitness {
            tree,
            filled,
            cursor_depth: 0,
            cursor,
        };

        witness.cursor_depth = witness.next_depth();

        witness
    }

    pub fn tree(&self) -> &CommitmentTree<H, DEPTH> {
        &self.tree
    }

    pub fn filled(&self) -> &Vec<H> {
        &self.filled
    }

    pub fn cursor(&self) -> &Option<CommitmentTree<H, DEPTH>> {
        &self.cursor
    }

    /// Returns the position of the witnessed leaf node in the commitment tree.
    pub fn witnessed_position(&self) -> Position {
        Position::try_from(self.tree.size() - 1)
            .expect("Commitment trees with more than 2^64 leaves are unsupported.")
    }

    /// Returns the position of the last leaf appended to the witness.
    pub fn tip_position(&self) -> Position {
        let leaves_to_cursor_start = self
            .witnessed_position()
            .witness_addrs(Level::from(DEPTH))
            .filter_map(|(addr, source)| {
                if source == Source::Future {
                    Some(addr)
                } else {
                    None
                }
            })
            .take(self.filled.len())
            .fold(0u64, |acc, addr| acc + (1u64 << u8::from(addr.level())));

        self.witnessed_position()
            + leaves_to_cursor_start
            + self.cursor.as_ref().map_or(0, |c| {
                u64::try_from(c.size())
                    .expect("Note commitment trees with > 2^64 leaves are not supported.")
            })
    }

    /// Finds the next "depth" of an unfilled subtree.
    fn next_depth(&self) -> u8 {
        let mut skip: u8 = self
            .filled
            .len()
            .try_into()
            .expect("Merkle tree depths may not exceed the bounds of a u8");

        if self.tree.left.is_none() {
            if skip > 0 {
                skip -= 1;
            } else {
                return 0;
            }
        }

        if self.tree.right.is_none() {
            if skip > 0 {
                skip -= 1;
            } else {
                return 0;
            }
        }

        let mut d = 1;
        for p in &self.tree.parents {
            if p.is_none() {
                if skip > 0 {
                    skip -= 1;
                } else {
                    return d;
                }
            }
            d += 1;
        }

        d + skip
    }
}

impl<H: Hashable + Clone, const DEPTH: u8> IncrementalWitness<H, DEPTH> {
    fn filler(&self) -> PathFiller<H> {
        let cursor_root = self
            .cursor
            .as_ref()
            .map(|c| c.root_at_depth(self.cursor_depth, PathFiller::empty()));

        PathFiller::new(self.filled.iter().cloned().chain(cursor_root).collect())
    }

    /// Tracks a leaf node that has been added to the underlying tree.
    ///
    /// Returns an error if the tree is full.
    #[allow(clippy::result_unit_err)]
    pub fn append(&mut self, node: H) -> Result<(), ()> {
        if let Some(mut cursor) = self.cursor.take() {
            cursor.append(node).expect("cursor should not be full");
            if cursor.is_complete(self.cursor_depth) {
                self.filled
                    .push(cursor.root_at_depth(self.cursor_depth, PathFiller::empty()));
            } else {
                self.cursor = Some(cursor);
            }
        } else {
            self.cursor_depth = self.next_depth();
            if self.cursor_depth >= DEPTH {
                // Tree is full
                return Err(());
            }

            if self.cursor_depth == 0 {
                self.filled.push(node);
            } else {
                let mut cursor = CommitmentTree::empty();
                cursor.append(node).expect("cursor should not be full");
                self.cursor = Some(cursor);
            }
        }

        Ok(())
    }

    /// Returns the current root of the tree corresponding to the witness.
    pub fn root(&self) -> H {
        self.tree.root_at_depth(DEPTH, self.filler())
    }

    /// Returns the current witness, or None if the tree is empty.
    pub fn path(&self) -> Option<MerklePath<H, DEPTH>> {
        self.path_inner(DEPTH)
    }

    fn path_inner(&self, depth: u8) -> Option<MerklePath<H, DEPTH>> {
        let mut filler = self.filler();
        let mut auth_path = Vec::new();

        if let Some(node) = &self.tree.left {
            if self.tree.right.is_some() {
                auth_path.push(node.clone());
            } else {
                auth_path.push(filler.next(0.into()));
            }
        } else {
            // Can't create an authentication path for the beginning of the tree
            return None;
        }

        for (p, i) in self
            .tree
            .parents
            .iter()
            .chain(repeat(&None))
            .take((depth - 1).into())
            .zip(0u8..)
        {
            auth_path.push(match p {
                Some(node) => node.clone(),
                None => filler.next(Level::from(i + 1)),
            });
        }

        assert_eq!(auth_path.len(), usize::from(depth));

        MerklePath::from_parts(auth_path, self.witnessed_position()).ok()
    }
}

#[cfg(test)]
mod tests {
    use crate::{frontier::CommitmentTree, witness::IncrementalWitness, Position};
    #[test]
    fn witness_tip_position() {
        let mut base_tree = CommitmentTree::<String, 6>::empty();
        for c in 'a'..'h' {
            base_tree.append(c.to_string()).unwrap();
        }
        let mut witness = IncrementalWitness::from_tree(base_tree);
        for c in 'h'..'z' {
            witness.append(c.to_string()).unwrap();
        }

        assert_eq!(witness.tip_position(), Position::from(24));
    }
}
