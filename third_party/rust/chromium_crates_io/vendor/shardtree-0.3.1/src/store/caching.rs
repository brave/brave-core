//! Implementation of an in-memory shard store with persistence.

use std::convert::Infallible;

use incrementalmerkletree::Address;

use super::{memory::MemoryShardStore, Checkpoint, ShardStore};
use crate::{LocatedPrunableTree, PrunableTree};

#[derive(Debug)]
enum Action<C> {
    Truncate(Address),
    RemoveCheckpoint(C),
    TruncateCheckpoints(C),
}

/// An implementation of [`ShardStore`] that caches all state in memory.
///
/// Cache state is flushed to the backend via [`Self::flush`]. Dropping will instead drop
/// the cached state and not make any changes to the backend.
#[derive(Debug)]
pub struct CachingShardStore<S>
where
    S: ShardStore,
    S::H: Clone,
    S::CheckpointId: Clone + Ord,
{
    backend: S,
    cache: MemoryShardStore<S::H, S::CheckpointId>,
    deferred_actions: Vec<Action<S::CheckpointId>>,
}

impl<S> CachingShardStore<S>
where
    S: ShardStore,
    S::H: Clone,
    S::CheckpointId: Clone + Ord,
{
    /// Loads a `CachingShardStore` from the given backend.
    pub fn load(mut backend: S) -> Result<Self, S::Error> {
        let mut cache = MemoryShardStore::empty();

        for shard_root in backend.get_shard_roots()? {
            let _ = cache.put_shard(backend.get_shard(shard_root)?.expect("known address"));
        }
        let _ = cache.put_cap(backend.get_cap()?);

        backend.with_checkpoints(backend.checkpoint_count()?, |checkpoint_id, checkpoint| {
            cache
                .add_checkpoint(checkpoint_id.clone(), checkpoint.clone())
                .unwrap();
            Ok(())
        })?;

        Ok(Self {
            backend,
            cache,
            deferred_actions: vec![],
        })
    }

    /// Flushes the current cache state to the backend and returns it.
    pub fn flush(mut self) -> Result<S, S::Error> {
        for action in &self.deferred_actions {
            match action {
                Action::Truncate(from) => self.backend.truncate(*from),
                Action::RemoveCheckpoint(checkpoint_id) => {
                    self.backend.remove_checkpoint(checkpoint_id)
                }
                Action::TruncateCheckpoints(checkpoint_id) => {
                    self.backend.truncate_checkpoints(checkpoint_id)
                }
            }?;
        }
        self.deferred_actions.clear();

        for shard_root in self.cache.get_shard_roots().unwrap() {
            self.backend.put_shard(
                self.cache
                    .get_shard(shard_root)
                    .unwrap()
                    .expect("known address"),
            )?;
        }
        self.backend.put_cap(self.cache.get_cap().unwrap())?;

        let mut checkpoints = Vec::with_capacity(self.cache.checkpoint_count().unwrap());
        self.cache
            .with_checkpoints(
                self.cache.checkpoint_count().unwrap(),
                |checkpoint_id, checkpoint| {
                    checkpoints.push((checkpoint_id.clone(), checkpoint.clone()));
                    Ok(())
                },
            )
            .unwrap();
        for (checkpoint_id, checkpoint) in checkpoints {
            self.backend.add_checkpoint(checkpoint_id, checkpoint)?;
        }

        Ok(self.backend)
    }
}

impl<S> ShardStore for CachingShardStore<S>
where
    S: ShardStore,
    S::H: Clone,
    S::CheckpointId: Clone + Ord,
{
    type H = S::H;
    type CheckpointId = S::CheckpointId;
    type Error = Infallible;

    fn get_shard(
        &self,
        shard_root: Address,
    ) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        self.cache.get_shard(shard_root)
    }

    fn last_shard(&self) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        self.cache.last_shard()
    }

    fn put_shard(&mut self, subtree: LocatedPrunableTree<Self::H>) -> Result<(), Self::Error> {
        self.cache.put_shard(subtree)
    }

    fn get_shard_roots(&self) -> Result<Vec<Address>, Self::Error> {
        self.cache.get_shard_roots()
    }

    fn truncate(&mut self, from: Address) -> Result<(), Self::Error> {
        self.deferred_actions.push(Action::Truncate(from));
        self.cache.truncate(from)
    }

    fn get_cap(&self) -> Result<PrunableTree<Self::H>, Self::Error> {
        self.cache.get_cap()
    }

    fn put_cap(&mut self, cap: PrunableTree<Self::H>) -> Result<(), Self::Error> {
        self.cache.put_cap(cap)
    }

    fn add_checkpoint(
        &mut self,
        checkpoint_id: Self::CheckpointId,
        checkpoint: Checkpoint,
    ) -> Result<(), Self::Error> {
        self.cache.add_checkpoint(checkpoint_id, checkpoint)
    }

    fn checkpoint_count(&self) -> Result<usize, Self::Error> {
        self.cache.checkpoint_count()
    }

    fn get_checkpoint(
        &self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<Option<Checkpoint>, Self::Error> {
        self.cache.get_checkpoint(checkpoint_id)
    }

    fn get_checkpoint_at_depth(
        &self,
        checkpoint_depth: usize,
    ) -> Result<Option<(Self::CheckpointId, Checkpoint)>, Self::Error> {
        self.cache.get_checkpoint_at_depth(checkpoint_depth)
    }

    fn min_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        self.cache.min_checkpoint_id()
    }

    fn max_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        self.cache.max_checkpoint_id()
    }

    fn with_checkpoints<F>(&mut self, limit: usize, callback: F) -> Result<(), Self::Error>
    where
        F: FnMut(&Self::CheckpointId, &Checkpoint) -> Result<(), Self::Error>,
    {
        self.cache.with_checkpoints(limit, callback)
    }

    fn update_checkpoint_with<F>(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
        update: F,
    ) -> Result<bool, Self::Error>
    where
        F: Fn(&mut Checkpoint) -> Result<(), Self::Error>,
    {
        self.cache.update_checkpoint_with(checkpoint_id, update)
    }

    fn remove_checkpoint(&mut self, checkpoint_id: &Self::CheckpointId) -> Result<(), Self::Error> {
        self.deferred_actions
            .push(Action::RemoveCheckpoint(checkpoint_id.clone()));
        self.cache.remove_checkpoint(checkpoint_id)
    }

    fn truncate_checkpoints(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        self.deferred_actions
            .push(Action::TruncateCheckpoints(checkpoint_id.clone()));
        self.cache.truncate_checkpoints(checkpoint_id)
    }
}

#[cfg(test)]
mod tests {
    use incrementalmerkletree::{
        testing::{
            append_str, check_operations, unmark, witness, CombinedTree, Operation, TestHashable,
            Tree,
        },
        Hashable, Position, Retention,
    };

    use super::CachingShardStore;
    use crate::{
        store::{memory::MemoryShardStore, ShardStore},
        ShardTree,
    };

    fn check_equal(
        mut lhs: MemoryShardStore<String, u64>,
        rhs: CachingShardStore<MemoryShardStore<String, u64>>,
    ) {
        let rhs = rhs.flush().unwrap();
        assert_eq!(lhs.get_shard_roots(), rhs.get_shard_roots());
        for shard_root in lhs.get_shard_roots().unwrap() {
            assert_eq!(lhs.get_shard(shard_root), rhs.get_shard(shard_root));
        }
        assert_eq!(
            lhs.checkpoint_count().unwrap(),
            rhs.checkpoint_count().unwrap(),
        );
        lhs.with_checkpoints(
            lhs.checkpoint_count().unwrap(),
            |checkpoint_id, lhs_checkpoint| {
                let rhs_checkpoint = rhs.get_checkpoint(checkpoint_id).unwrap().unwrap();
                assert_eq!(lhs_checkpoint.tree_state, rhs_checkpoint.tree_state);
                assert_eq!(lhs_checkpoint.marks_removed, rhs_checkpoint.marks_removed);
                Ok(())
            },
        )
        .unwrap();
    }

    #[test]
    fn root_hashes() {
        use Retention::*;

        {
            let mut lhs = MemoryShardStore::<_, u64>::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::<String, _, _, _>::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert_eq!(
                tree.root(0).unwrap(),
                String::combine_all(tree.depth(), &[]),
            );
            assert!(tree.append(String::from_u64(0), Ephemeral));
            assert_eq!(
                tree.root(0).unwrap(),
                String::combine_all(tree.depth(), &[0]),
            );
            assert!(tree.append(String::from_u64(1), Ephemeral));
            assert_eq!(
                tree.root(0).unwrap(),
                String::combine_all(tree.depth(), &[0, 1]),
            );
            assert!(tree.append(String::from_u64(2), Ephemeral));
            assert_eq!(
                tree.root(0).unwrap(),
                String::combine_all(tree.depth(), &[0, 1, 2]),
            );

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut t = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert!(t.append(
                String::from_u64(0),
                Checkpoint {
                    id: 1,
                    is_marked: true,
                },
            ));
            for _ in 0..3 {
                assert!(t.append(String::from_u64(0), Ephemeral));
            }
            assert_eq!(
                t.root(0).unwrap(),
                String::combine_all(t.depth(), &[0, 0, 0, 0])
            );

            check_equal(lhs, rhs);
        }
    }

    #[test]
    fn append() {
        use Retention::*;

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert_eq!(tree.depth(), 4);

            // 16 appends should succeed
            for i in 0..16 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
                assert_eq!(tree.current_position(), Some(Position::from(i)));
            }

            // 17th append should fail
            assert!(!tree.append(String::from_u64(16), Ephemeral));

            check_equal(lhs, rhs);
        }

        {
            // The following checks a condition on state restoration in the case that an append fails.
            // We want to ensure that a failed append does not cause a loss of information.
            let ops = (0..17)
                .map(|i| Operation::Append(String::from_u64(i), Ephemeral))
                .collect::<Vec<_>>();

            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            check_operations(tree, &ops).unwrap();
            check_equal(lhs, rhs);
        }
    }

    #[test]
    fn check_witnesses() {
        use Operation::{Append, Rewind, Witness};
        use Retention::*;

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert!(tree.append(String::from_u64(0), Ephemeral));
            assert!(tree.append(String::from_u64(1), Marked));
            assert_eq!(tree.witness(Position::from(0), 0), None);

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert!(tree.append(String::from_u64(0), Marked));
            assert_eq!(
                tree.witness(Position::from(0), 0),
                Some(vec![
                    String::empty_root(0.into()),
                    String::empty_root(1.into()),
                    String::empty_root(2.into()),
                    String::empty_root(3.into())
                ])
            );

            assert!(tree.append(String::from_u64(1), Ephemeral));
            assert_eq!(
                tree.witness(0.into(), 0),
                Some(vec![
                    String::from_u64(1),
                    String::empty_root(1.into()),
                    String::empty_root(2.into()),
                    String::empty_root(3.into())
                ])
            );

            assert!(tree.append(String::from_u64(2), Marked));
            assert_eq!(
                tree.witness(Position::from(2), 0),
                Some(vec![
                    String::empty_root(0.into()),
                    String::combine_all(1, &[0, 1]),
                    String::empty_root(2.into()),
                    String::empty_root(3.into())
                ])
            );

            assert!(tree.append(String::from_u64(3), Ephemeral));
            assert_eq!(
                tree.witness(Position::from(2), 0),
                Some(vec![
                    String::from_u64(3),
                    String::combine_all(1, &[0, 1]),
                    String::empty_root(2.into()),
                    String::empty_root(3.into())
                ])
            );

            assert!(tree.append(String::from_u64(4), Ephemeral));
            assert_eq!(
                tree.witness(Position::from(2), 0),
                Some(vec![
                    String::from_u64(3),
                    String::combine_all(1, &[0, 1]),
                    String::combine_all(2, &[4]),
                    String::empty_root(3.into())
                ])
            );

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert!(tree.append(String::from_u64(0), Marked));
            for i in 1..6 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
            }
            assert!(tree.append(String::from_u64(6), Marked));
            assert!(tree.append(String::from_u64(7), Ephemeral));

            assert_eq!(
                tree.witness(0.into(), 0),
                Some(vec![
                    String::from_u64(1),
                    String::combine_all(1, &[2, 3]),
                    String::combine_all(2, &[4, 5, 6, 7]),
                    String::empty_root(3.into())
                ])
            );

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert!(tree.append(String::from_u64(0), Marked));
            assert!(tree.append(String::from_u64(1), Ephemeral));
            assert!(tree.append(String::from_u64(2), Ephemeral));
            assert!(tree.append(String::from_u64(3), Marked));
            assert!(tree.append(String::from_u64(4), Marked));
            assert!(tree.append(String::from_u64(5), Marked));
            assert!(tree.append(String::from_u64(6), Ephemeral));

            assert_eq!(
                tree.witness(Position::from(5), 0),
                Some(vec![
                    String::from_u64(4),
                    String::combine_all(1, &[6]),
                    String::combine_all(2, &[0, 1, 2, 3]),
                    String::empty_root(3.into())
                ])
            );

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            for i in 0..10 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
            }
            assert!(tree.append(String::from_u64(10), Marked));
            assert!(tree.append(String::from_u64(11), Ephemeral));

            assert_eq!(
                tree.witness(Position::from(10), 0),
                Some(vec![
                    String::from_u64(11),
                    String::combine_all(1, &[8, 9]),
                    String::empty_root(2.into()),
                    String::combine_all(3, &[0, 1, 2, 3, 4, 5, 6, 7])
                ])
            );

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert!(tree.append(
                String::from_u64(0),
                Checkpoint {
                    id: 1,
                    is_marked: true,
                },
            ));
            assert!(tree.rewind());
            for i in 1..4 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
            }
            assert!(tree.append(String::from_u64(4), Marked));
            for i in 5..8 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
            }
            assert_eq!(
                tree.witness(0.into(), 0),
                Some(vec![
                    String::from_u64(1),
                    String::combine_all(1, &[2, 3]),
                    String::combine_all(2, &[4, 5, 6, 7]),
                    String::empty_root(3.into()),
                ])
            );

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert!(tree.append(String::from_u64(0), Ephemeral));
            assert!(tree.append(String::from_u64(1), Ephemeral));
            assert!(tree.append(String::from_u64(2), Marked));
            assert!(tree.append(String::from_u64(3), Ephemeral));
            assert!(tree.append(String::from_u64(4), Ephemeral));
            assert!(tree.append(String::from_u64(5), Ephemeral));
            assert!(tree.append(
                String::from_u64(6),
                Checkpoint {
                    id: 1,
                    is_marked: true,
                },
            ));
            assert!(tree.append(String::from_u64(7), Ephemeral));
            assert!(tree.rewind());
            assert_eq!(
                tree.witness(Position::from(2), 0),
                Some(vec![
                    String::from_u64(3),
                    String::combine_all(1, &[0, 1]),
                    String::combine_all(2, &[4, 5, 6]),
                    String::empty_root(3.into())
                ])
            );

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            for i in 0..12 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
            }
            assert!(tree.append(String::from_u64(12), Marked));
            assert!(tree.append(String::from_u64(13), Marked));
            assert!(tree.append(String::from_u64(14), Ephemeral));
            assert!(tree.append(String::from_u64(15), Ephemeral));

            assert_eq!(
                tree.witness(Position::from(12), 0),
                Some(vec![
                    String::from_u64(13),
                    String::combine_all(1, &[14, 15]),
                    String::combine_all(2, &[8, 9, 10, 11]),
                    String::combine_all(3, &[0, 1, 2, 3, 4, 5, 6, 7]),
                ])
            );

            check_equal(lhs, rhs);
        }

        {
            let ops = (0..=11)
                .map(|i| Append(String::from_u64(i), Marked))
                .chain(Some(Append(String::from_u64(12), Ephemeral)))
                .chain(Some(Append(String::from_u64(13), Ephemeral)))
                .chain(Some(Witness(11u64.into(), 0)))
                .collect::<Vec<_>>();

            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert_eq!(
                Operation::apply_all(&ops, &mut tree),
                Some((
                    Position::from(11),
                    vec![
                        String::from_u64(10),
                        String::combine_all(1, &[8, 9]),
                        String::combine_all(2, &[12, 13]),
                        String::combine_all(3, &[0, 1, 2, 3, 4, 5, 6, 7]),
                    ]
                ))
            );

            check_equal(lhs, rhs);
        }

        {
            let ops = vec![
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(1), Ephemeral),
                Append(String::from_u64(2), Ephemeral),
                Append(
                    String::from_u64(3),
                    Checkpoint {
                        id: 1,
                        is_marked: true,
                    },
                ),
                Append(String::from_u64(4), Marked),
                Operation::Checkpoint(2),
                Append(
                    String::from_u64(5),
                    Checkpoint {
                        id: 3,
                        is_marked: false,
                    },
                ),
                Append(
                    String::from_u64(6),
                    Checkpoint {
                        id: 4,
                        is_marked: false,
                    },
                ),
                Append(
                    String::from_u64(7),
                    Checkpoint {
                        id: 5,
                        is_marked: false,
                    },
                ),
                Witness(3u64.into(), 5),
            ];

            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert_eq!(
                Operation::apply_all(&ops, &mut tree),
                Some((
                    Position::from(3),
                    vec![
                        String::from_u64(2),
                        String::combine_all(1, &[0, 1]),
                        String::combine_all(2, &[]),
                        String::combine_all(3, &[]),
                    ]
                ))
            );

            check_equal(lhs, rhs);
        }

        {
            let ops = vec![
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(
                    String::from_u64(0),
                    Checkpoint {
                        id: 1,
                        is_marked: true,
                    },
                ),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(
                    String::from_u64(0),
                    Checkpoint {
                        id: 2,
                        is_marked: false,
                    },
                ),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Witness(Position::from(3), 1),
            ];

            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert_eq!(
                Operation::apply_all(&ops, &mut tree),
                Some((
                    Position::from(3),
                    vec![
                        String::from_u64(0),
                        String::combine_all(1, &[0, 0]),
                        String::combine_all(2, &[0, 0, 0, 0]),
                        String::combine_all(3, &[]),
                    ]
                ))
            );

            check_equal(lhs, rhs);
        }

        {
            let ops = vec![
                Append(String::from_u64(0), Marked),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Operation::Checkpoint(1),
                Append(String::from_u64(0), Marked),
                Operation::Checkpoint(2),
                Operation::Checkpoint(3),
                Append(
                    String::from_u64(0),
                    Checkpoint {
                        id: 4,
                        is_marked: false,
                    },
                ),
                Rewind,
                Rewind,
                Witness(Position::from(7), 2),
            ];

            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert_eq!(Operation::apply_all(&ops, &mut tree), None);

            check_equal(lhs, rhs);
        }

        {
            let ops = vec![
                Append(String::from_u64(0), Marked),
                Append(String::from_u64(0), Ephemeral),
                Append(
                    String::from_u64(0),
                    Checkpoint {
                        id: 1,
                        is_marked: true,
                    },
                ),
                Append(
                    String::from_u64(0),
                    Checkpoint {
                        id: 4,
                        is_marked: false,
                    },
                ),
                Witness(Position::from(2), 2),
            ];

            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert_eq!(
                Operation::apply_all(&ops, &mut tree),
                Some((
                    Position::from(2),
                    vec![
                        String::empty_leaf(),
                        String::combine_all(1, &[0, 0]),
                        String::combine_all(2, &[]),
                        String::combine_all(3, &[]),
                    ]
                ))
            );

            check_equal(lhs, rhs);
        }
    }

    #[test]
    fn check_checkpoint_rewind() {
        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut t = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert!(!t.rewind());

            check_equal(lhs, rhs);
        }
        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut t = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            assert!(t.checkpoint(1));
            assert!(t.rewind());

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut t = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            t.append("a".to_string(), Retention::Ephemeral);
            assert!(t.checkpoint(1));
            t.append("b".to_string(), Retention::Marked);
            assert!(t.rewind());
            assert_eq!(Some(Position::from(0)), t.current_position());

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut t = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            t.append("a".to_string(), Retention::Marked);
            assert!(t.checkpoint(1));
            assert!(t.rewind());

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut t = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            t.append("a".to_string(), Retention::Marked);
            assert!(t.checkpoint(1));
            t.append("a".to_string(), Retention::Ephemeral);
            assert!(t.rewind());
            assert_eq!(Some(Position::from(0)), t.current_position());

            check_equal(lhs, rhs);
        }

        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut t = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            t.append("a".to_string(), Retention::Ephemeral);
            assert!(t.checkpoint(1));
            assert!(t.checkpoint(2));
            assert!(t.rewind());
            t.append("b".to_string(), Retention::Ephemeral);
            assert!(t.rewind());
            t.append("b".to_string(), Retention::Ephemeral);
            assert_eq!(t.root(0).unwrap(), "ab______________");

            check_equal(lhs, rhs);
        }
    }

    #[test]
    fn check_remove_mark() {
        let samples = vec![
            vec![
                append_str("a", Retention::Ephemeral),
                append_str(
                    "a",
                    Retention::Checkpoint {
                        id: 1,
                        is_marked: true,
                    },
                ),
                witness(1, 1),
            ],
            vec![
                append_str("a", Retention::Ephemeral),
                append_str("a", Retention::Ephemeral),
                append_str("a", Retention::Ephemeral),
                append_str("a", Retention::Marked),
                Operation::Checkpoint(1),
                unmark(3),
                witness(3, 0),
            ],
        ];

        for (i, sample) in samples.iter().enumerate() {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            let result = check_operations(tree, sample);
            assert!(
                matches!(result, Ok(())),
                "Reference/Test mismatch at index {}: {:?}",
                i,
                result
            );

            check_equal(lhs, rhs);
        }
    }

    #[test]
    fn check_rewind_remove_mark() {
        use Operation::*;

        // rewinding doesn't remove a mark
        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            tree.append("e".to_string(), Retention::Marked);
            assert!(tree.checkpoint(1));
            assert!(tree.rewind());
            assert!(tree.remove_mark(0u64.into()));

            check_equal(lhs, rhs);
        }

        // use a maximum number of checkpoints of 1
        {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 1),
                ShardTree::<_, 4, 3>::new(&mut rhs, 1),
            );

            assert!(tree.append("e".to_string(), Retention::Marked));
            assert!(tree.checkpoint(1));
            assert!(tree.marked_positions().contains(&0u64.into()));
            assert!(tree.append("f".to_string(), Retention::Ephemeral));
            // simulate a spend of `e` at `f`
            assert!(tree.remove_mark(0u64.into()));
            // even though the mark has been staged for removal, it's not gone yet
            assert!(tree.marked_positions().contains(&0u64.into()));
            assert!(tree.checkpoint(2));
            // the newest checkpoint will have caused the oldest to roll off, and
            // so the forgotten node will be unmarked
            assert!(!tree.marked_positions().contains(&0u64.into()));
            assert!(!tree.remove_mark(0u64.into()));

            check_equal(lhs, rhs);
        }

        // The following check_operations tests cover errors where the
        // test framework itself previously did not correctly handle
        // chain state restoration.

        let samples = vec![
            vec![
                append_str("x", Retention::Marked),
                Checkpoint(1),
                Rewind,
                unmark(0),
            ],
            vec![
                append_str("d", Retention::Marked),
                Checkpoint(1),
                unmark(0),
                Rewind,
                unmark(0),
            ],
            vec![
                append_str("o", Retention::Marked),
                Checkpoint(1),
                Checkpoint(2),
                unmark(0),
                Rewind,
                Rewind,
            ],
            vec![
                append_str("s", Retention::Marked),
                append_str("m", Retention::Ephemeral),
                Checkpoint(1),
                unmark(0),
                Rewind,
                unmark(0),
                unmark(0),
            ],
            vec![
                append_str("a", Retention::Marked),
                Checkpoint(1),
                Rewind,
                append_str("a", Retention::Marked),
            ],
        ];

        for (i, sample) in samples.iter().enumerate() {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            let result = check_operations(tree, sample);
            assert!(
                matches!(result, Ok(())),
                "Reference/Test mismatch at index {}: {:?}",
                i,
                result
            );

            check_equal(lhs, rhs);
        }
    }

    #[test]
    fn check_witness_consistency() {
        use Operation::*;

        let samples = vec![
            // Reduced examples
            vec![
                append_str("a", Retention::Ephemeral),
                append_str("b", Retention::Marked),
                Checkpoint(1),
                witness(0, 1),
            ],
            vec![
                append_str("c", Retention::Ephemeral),
                append_str("d", Retention::Marked),
                Checkpoint(1),
                witness(1, 1),
            ],
            vec![
                append_str("e", Retention::Marked),
                Checkpoint(1),
                append_str("f", Retention::Ephemeral),
                witness(0, 1),
            ],
            vec![
                append_str("g", Retention::Marked),
                Checkpoint(1),
                unmark(0),
                append_str("h", Retention::Ephemeral),
                witness(0, 0),
            ],
            vec![
                append_str("i", Retention::Marked),
                Checkpoint(1),
                unmark(0),
                append_str("j", Retention::Ephemeral),
                witness(0, 0),
            ],
            vec![
                append_str("i", Retention::Marked),
                append_str("j", Retention::Ephemeral),
                Checkpoint(1),
                append_str("k", Retention::Ephemeral),
                witness(0, 1),
            ],
            vec![
                append_str("l", Retention::Marked),
                Checkpoint(1),
                Checkpoint(2),
                append_str("m", Retention::Ephemeral),
                Checkpoint(3),
                witness(0, 2),
            ],
            vec![
                Checkpoint(1),
                append_str("n", Retention::Marked),
                witness(0, 1),
            ],
            vec![
                append_str("a", Retention::Marked),
                Checkpoint(1),
                unmark(0),
                Checkpoint(2),
                append_str("b", Retention::Ephemeral),
                witness(0, 1),
            ],
            vec![
                append_str("a", Retention::Marked),
                append_str("b", Retention::Ephemeral),
                unmark(0),
                Checkpoint(1),
                witness(0, 0),
            ],
            vec![
                append_str("a", Retention::Marked),
                Checkpoint(1),
                unmark(0),
                Checkpoint(2),
                Rewind,
                append_str("b", Retention::Ephemeral),
                witness(0, 0),
            ],
            vec![
                append_str("a", Retention::Marked),
                Checkpoint(1),
                Checkpoint(2),
                Rewind,
                append_str("a", Retention::Ephemeral),
                unmark(0),
                witness(0, 1),
            ],
            // Unreduced examples
            vec![
                append_str("o", Retention::Ephemeral),
                append_str("p", Retention::Marked),
                append_str("q", Retention::Ephemeral),
                Checkpoint(1),
                unmark(1),
                witness(1, 1),
            ],
            vec![
                append_str("r", Retention::Ephemeral),
                append_str("s", Retention::Ephemeral),
                append_str("t", Retention::Marked),
                Checkpoint(1),
                unmark(2),
                Checkpoint(2),
                witness(2, 2),
            ],
            vec![
                append_str("u", Retention::Marked),
                append_str("v", Retention::Ephemeral),
                append_str("w", Retention::Ephemeral),
                Checkpoint(1),
                unmark(0),
                append_str("x", Retention::Ephemeral),
                Checkpoint(2),
                Checkpoint(3),
                witness(0, 3),
            ],
        ];

        for (i, sample) in samples.iter().enumerate() {
            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            let result = check_operations(tree, sample);
            assert!(
                matches!(result, Ok(())),
                "Reference/Test mismatch at index {}: {:?}",
                i,
                result
            );

            check_equal(lhs, rhs);
        }
    }
}
