//! Sample implementation of the Tree interface.
use std::cmp::min;
use std::collections::{BTreeMap, BTreeSet};

use crate::{testing::Tree, Hashable, Level, Position, Retention};

const MAX_COMPLETE_SIZE_ERROR: &str = "Positions of a `CompleteTree` must fit into the platform word size, because larger complete trees are not representable.";

pub(crate) fn root<H: Hashable + Clone>(leaves: &[H], depth: u8) -> H {
    let empty_leaf = H::empty_leaf();
    let mut leaves = leaves
        .iter()
        .chain(std::iter::repeat(&empty_leaf))
        .take(1 << depth)
        .cloned()
        .collect::<Vec<H>>();

    //leaves are always at level zero, so we start there.
    let mut level = Level::from(0);
    while leaves.len() != 1 {
        leaves = leaves
            .iter()
            .enumerate()
            .filter(|(i, _)| (i % 2) == 0)
            .map(|(_, a)| a)
            .zip(
                leaves
                    .iter()
                    .enumerate()
                    .filter(|(i, _)| (i % 2) == 1)
                    .map(|(_, b)| b),
            )
            .map(|(a, b)| H::combine(level, a, b))
            .collect();
        level = level + 1;
    }

    leaves[0].clone()
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Checkpoint {
    /// The number of leaves in the tree when the checkpoint was created.
    leaves_len: usize,
    /// A set of the positions that have been marked during the period that this
    /// checkpoint is the current checkpoint.
    marked: BTreeSet<Position>,
    /// When a mark is forgotten, we add it to the checkpoint's forgotten set but
    /// don't immediately remove it from the `marked` set; that removal occurs when
    /// the checkpoint is eventually dropped.
    forgotten: BTreeSet<Position>,
}

impl Checkpoint {
    fn at_length(leaves_len: usize) -> Self {
        Checkpoint {
            leaves_len,
            marked: BTreeSet::new(),
            forgotten: BTreeSet::new(),
        }
    }
}

#[derive(Clone, Debug)]
pub struct CompleteTree<H, C: Ord, const DEPTH: u8> {
    leaves: Vec<Option<H>>,
    marks: BTreeSet<Position>,
    checkpoints: BTreeMap<C, Checkpoint>,
    max_checkpoints: usize,
}

impl<H: Hashable, C: Clone + Ord + core::fmt::Debug, const DEPTH: u8> CompleteTree<H, C, DEPTH> {
    /// Creates a new, empty binary tree
    pub fn new(max_checkpoints: usize) -> Self {
        Self {
            leaves: vec![],
            marks: BTreeSet::new(),
            checkpoints: BTreeMap::new(),
            max_checkpoints,
        }
    }

    /// Appends a new value to the tree at the next available slot.
    ///
    /// Returns true if successful and false if the tree is full or, for values with `Checkpoint`
    /// retention, if a checkpoint id would be introduced that is less than or equal to the current
    /// maximum checkpoint id.
    fn append(&mut self, value: H, retention: Retention<C>) -> Result<(), AppendError<C>> {
        fn append<H, C>(
            leaves: &mut Vec<Option<H>>,
            value: H,
            depth: u8,
        ) -> Result<(), AppendError<C>> {
            if leaves.len() < (1 << depth) {
                leaves.push(Some(value));
                Ok(())
            } else {
                Err(AppendError::TreeFull)
            }
        }

        match retention {
            Retention::Marked => {
                append(&mut self.leaves, value, DEPTH)?;
                self.mark();
            }
            Retention::Checkpoint { id, is_marked } => {
                let latest_checkpoint = self.checkpoints.keys().rev().next();
                if Some(&id) > latest_checkpoint {
                    append(&mut self.leaves, value, DEPTH)?;
                    if is_marked {
                        self.mark();
                    }
                    self.checkpoint(id, self.current_position());
                } else {
                    return Err(AppendError::CheckpointOutOfOrder {
                        current_max: latest_checkpoint.cloned(),
                        checkpoint: id,
                    });
                }
            }
            Retention::Ephemeral => {
                append(&mut self.leaves, value, DEPTH)?;
            }
        }

        Ok(())
    }

    fn current_position(&self) -> Option<Position> {
        if self.leaves.is_empty() {
            None
        } else {
            // this unwrap is safe because nobody is ever going to create a complete
            // tree with more than 2^64 leaves
            Some((self.leaves.len() - 1).try_into().unwrap())
        }
    }

    /// Marks the current tree state leaf as a value that we're interested in
    /// marking. Returns the current position if the tree is non-empty.
    fn mark(&mut self) -> Option<Position> {
        if let Some(pos) = self.current_position() {
            if !self.marks.contains(&pos) {
                self.marks.insert(pos);

                if let Some(checkpoint) = self.checkpoints.values_mut().rev().next() {
                    checkpoint.marked.insert(pos);
                }
            }

            Some(pos)
        } else {
            None
        }
    }

    fn checkpoint(&mut self, id: C, pos: Option<Position>) {
        self.checkpoints.insert(
            id,
            Checkpoint::at_length(pos.map_or_else(
                || 0,
                |p| usize::try_from(p).expect(MAX_COMPLETE_SIZE_ERROR) + 1,
            )),
        );
        if self.checkpoints.len() > self.max_checkpoints {
            self.drop_oldest_checkpoint();
        }
    }

    fn leaves_at_checkpoint_depth(&self, checkpoint_depth: usize) -> Option<usize> {
        if checkpoint_depth == 0 {
            Some(self.leaves.len())
        } else {
            self.checkpoints
                .iter()
                .rev()
                .skip(checkpoint_depth - 1)
                .map(|(_, c)| c.leaves_len)
                .next()
        }
    }

    /// Removes the oldest checkpoint. Returns true if successful and false if
    /// there are fewer than `self.max_checkpoints` checkpoints.
    fn drop_oldest_checkpoint(&mut self) -> bool {
        if self.checkpoints.len() > self.max_checkpoints {
            let (id, c) = self.checkpoints.iter().next().unwrap();
            for pos in c.forgotten.iter() {
                self.marks.remove(pos);
            }
            let id = id.clone(); // needed to avoid mutable/immutable borrow conflict
            self.checkpoints.remove(&id);
            true
        } else {
            false
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
enum AppendError<C> {
    TreeFull,
    CheckpointOutOfOrder {
        current_max: Option<C>,
        checkpoint: C,
    },
}

impl<H: Hashable + PartialEq + Clone, C: Ord + Clone + core::fmt::Debug, const DEPTH: u8> Tree<H, C>
    for CompleteTree<H, C, DEPTH>
{
    fn depth(&self) -> u8 {
        DEPTH
    }

    fn append(&mut self, value: H, retention: Retention<C>) -> bool {
        Self::append(self, value, retention).is_ok()
    }

    fn current_position(&self) -> Option<Position> {
        Self::current_position(self)
    }

    fn marked_positions(&self) -> BTreeSet<Position> {
        self.marks.clone()
    }

    fn get_marked_leaf(&self, position: Position) -> Option<H> {
        if self.marks.contains(&position) {
            self.leaves
                .get(usize::try_from(position).expect(MAX_COMPLETE_SIZE_ERROR))
                .and_then(|opt: &Option<H>| opt.clone())
        } else {
            None
        }
    }

    fn root(&self, checkpoint_depth: usize) -> Option<H> {
        self.leaves_at_checkpoint_depth(checkpoint_depth)
            .and_then(|len| root(&self.leaves[0..len], DEPTH))
    }

    fn witness(&self, position: Position, checkpoint_depth: usize) -> Option<Vec<H>> {
        if self.marks.contains(&position) && checkpoint_depth <= self.checkpoints.len() {
            let leaves_len = self.leaves_at_checkpoint_depth(checkpoint_depth)?;
            let c_idx = self.checkpoints.len() - checkpoint_depth;
            if self
                .checkpoints
                .iter()
                .skip(c_idx)
                .any(|(_, c)| c.marked.contains(&position))
            {
                // The requested position was marked after the checkpoint was created, so we
                // cannot create a witness.
                None
            } else {
                let mut path = vec![];

                let mut leaf_idx: usize = position.try_into().expect(MAX_COMPLETE_SIZE_ERROR);
                for bit in 0..DEPTH {
                    leaf_idx ^= 1 << bit;
                    path.push(if leaf_idx < leaves_len {
                        let subtree_end = min(leaf_idx + (1 << bit), leaves_len);
                        root(&self.leaves[leaf_idx..subtree_end], bit)?
                    } else {
                        H::empty_root(Level::from(bit))
                    });
                    leaf_idx &= usize::MAX << (bit + 1);
                }

                Some(path)
            }
        } else {
            None
        }
    }

    fn remove_mark(&mut self, position: Position) -> bool {
        if self.marks.contains(&position) {
            if let Some(c) = self.checkpoints.values_mut().rev().next() {
                c.forgotten.insert(position);
            } else {
                self.marks.remove(&position);
            }
            true
        } else {
            false
        }
    }

    fn checkpoint(&mut self, id: C) -> bool {
        if Some(&id) > self.checkpoints.keys().rev().next() {
            Self::checkpoint(self, id, self.current_position());
            true
        } else {
            false
        }
    }

    fn rewind(&mut self) -> bool {
        if let Some((id, c)) = self.checkpoints.iter().rev().next() {
            self.leaves.truncate(c.leaves_len);
            for pos in c.marked.iter() {
                self.marks.remove(pos);
            }
            let id = id.clone(); // needed to avoid mutable/immutable borrow conflict
            self.checkpoints.remove(&id);
            true
        } else {
            false
        }
    }
}

#[cfg(test)]
mod tests {
    use std::convert::TryFrom;

    use super::CompleteTree;
    use crate::{
        testing::{
            check_append, check_checkpoint_rewind, check_rewind_remove_mark, check_root_hashes,
            check_witnesses, compute_root_from_witness, SipHashable, Tree,
        },
        Hashable, Level, Position, Retention,
    };

    #[test]
    fn correct_empty_root() {
        const DEPTH: u8 = 5;
        let mut expected = SipHashable(0u64);
        for lvl in 0u8..DEPTH {
            expected = SipHashable::combine(lvl.into(), &expected, &expected);
        }

        let tree = CompleteTree::<SipHashable, (), DEPTH>::new(100);
        assert_eq!(tree.root(0).unwrap(), expected);
    }

    #[test]
    fn correct_root() {
        const DEPTH: u8 = 3;
        let values = (0..(1 << DEPTH)).map(SipHashable);

        let mut tree = CompleteTree::<SipHashable, (), DEPTH>::new(100);
        for value in values {
            assert!(tree.append(value, Retention::Ephemeral).is_ok());
        }
        assert!(tree.append(SipHashable(0), Retention::Ephemeral).is_err());

        let expected = SipHashable::combine(
            Level::from(2),
            &SipHashable::combine(
                Level::from(1),
                &SipHashable::combine(Level::from(1), &SipHashable(0), &SipHashable(1)),
                &SipHashable::combine(Level::from(1), &SipHashable(2), &SipHashable(3)),
            ),
            &SipHashable::combine(
                Level::from(1),
                &SipHashable::combine(Level::from(1), &SipHashable(4), &SipHashable(5)),
                &SipHashable::combine(Level::from(1), &SipHashable(6), &SipHashable(7)),
            ),
        );

        assert_eq!(tree.root(0).unwrap(), expected);
    }

    #[test]
    fn append() {
        check_append(CompleteTree::<String, usize, 4>::new);
    }

    #[test]
    fn root_hashes() {
        check_root_hashes(CompleteTree::<String, usize, 4>::new);
    }

    #[test]
    fn witnesses() {
        check_witnesses(CompleteTree::<String, usize, 4>::new);
    }

    #[test]
    fn correct_witness() {
        use crate::{testing::Tree, Retention};

        const DEPTH: u8 = 3;
        let values = (0..(1 << DEPTH)).map(SipHashable);

        let mut tree = CompleteTree::<SipHashable, (), DEPTH>::new(100);
        for value in values {
            assert!(Tree::append(&mut tree, value, Retention::Marked));
        }
        assert!(tree.append(SipHashable(0), Retention::Ephemeral).is_err());

        let expected = SipHashable::combine(
            <Level>::from(2),
            &SipHashable::combine(
                Level::from(1),
                &SipHashable::combine(Level::from(1), &SipHashable(0), &SipHashable(1)),
                &SipHashable::combine(Level::from(1), &SipHashable(2), &SipHashable(3)),
            ),
            &SipHashable::combine(
                Level::from(1),
                &SipHashable::combine(Level::from(1), &SipHashable(4), &SipHashable(5)),
                &SipHashable::combine(Level::from(1), &SipHashable(6), &SipHashable(7)),
            ),
        );

        assert_eq!(tree.root(0).unwrap(), expected);

        for i in 0u64..(1 << DEPTH) {
            let position = Position::try_from(i).unwrap();
            let path = tree.witness(position, 0).unwrap();
            assert_eq!(
                compute_root_from_witness(SipHashable(i), position, &path),
                expected
            );
        }
    }

    #[test]
    fn checkpoint_rewind() {
        check_checkpoint_rewind(|max_checkpoints| {
            CompleteTree::<String, usize, 4>::new(max_checkpoints)
        });
    }

    #[test]
    fn rewind_remove_mark() {
        check_rewind_remove_mark(|max_checkpoints| {
            CompleteTree::<String, usize, 4>::new(max_checkpoints)
        });
    }
}
