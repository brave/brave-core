//! Implementation of an in-memory shard store with no persistence.

use std::collections::BTreeMap;
use std::convert::{Infallible, TryFrom};

use incrementalmerkletree::Address;

use super::{Checkpoint, ShardStore};
use crate::{LocatedPrunableTree, LocatedTree, Node, PrunableTree, Tree};

/// An implementation of [`ShardStore`] that stores all state in memory.
///
/// State is not persisted anywhere, and will be lost when the struct is dropped.
#[derive(Debug)]
pub struct MemoryShardStore<H, C: Ord> {
    shards: Vec<LocatedPrunableTree<H>>,
    checkpoints: BTreeMap<C, Checkpoint>,
    cap: PrunableTree<H>,
}

impl<H, C: Ord> MemoryShardStore<H, C> {
    /// Constructs a new empty `MemoryShardStore`.
    pub fn empty() -> Self {
        Self {
            shards: vec![],
            checkpoints: BTreeMap::new(),
            cap: PrunableTree::empty(),
        }
    }
}

impl<H: Clone, C: Clone + Ord> ShardStore for MemoryShardStore<H, C> {
    type H = H;
    type CheckpointId = C;
    type Error = Infallible;

    fn get_shard(
        &self,
        shard_root: Address,
    ) -> Result<Option<LocatedPrunableTree<H>>, Self::Error> {
        let shard_idx =
            usize::try_from(shard_root.index()).expect("SHARD_HEIGHT > 64 is unsupported");
        Ok(self.shards.get(shard_idx).cloned())
    }

    fn last_shard(&self) -> Result<Option<LocatedPrunableTree<H>>, Self::Error> {
        Ok(self.shards.last().cloned())
    }

    fn put_shard(&mut self, subtree: LocatedPrunableTree<H>) -> Result<(), Self::Error> {
        let subtree_addr = subtree.root_addr;
        for subtree_idx in
            self.shards.last().map_or(0, |s| s.root_addr.index() + 1)..=subtree_addr.index()
        {
            self.shards.push(LocatedTree {
                root_addr: Address::from_parts(subtree_addr.level(), subtree_idx),
                root: Tree(Node::Nil),
            })
        }

        let shard_idx =
            usize::try_from(subtree_addr.index()).expect("SHARD_HEIGHT > 64 is unsupported");
        self.shards[shard_idx] = subtree;
        Ok(())
    }

    fn get_shard_roots(&self) -> Result<Vec<Address>, Self::Error> {
        Ok(self.shards.iter().map(|s| s.root_addr).collect())
    }

    fn truncate(&mut self, from: Address) -> Result<(), Self::Error> {
        let shard_idx = usize::try_from(from.index()).expect("SHARD_HEIGHT > 64 is unsupported");
        self.shards.truncate(shard_idx);
        Ok(())
    }

    fn get_cap(&self) -> Result<PrunableTree<H>, Self::Error> {
        Ok(self.cap.clone())
    }

    fn put_cap(&mut self, cap: PrunableTree<H>) -> Result<(), Self::Error> {
        self.cap = cap;
        Ok(())
    }

    fn add_checkpoint(
        &mut self,
        checkpoint_id: C,
        checkpoint: Checkpoint,
    ) -> Result<(), Self::Error> {
        self.checkpoints.insert(checkpoint_id, checkpoint);
        Ok(())
    }

    fn checkpoint_count(&self) -> Result<usize, Self::Error> {
        Ok(self.checkpoints.len())
    }

    fn get_checkpoint(
        &self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<Option<Checkpoint>, Self::Error> {
        Ok(self.checkpoints.get(checkpoint_id).cloned())
    }

    fn get_checkpoint_at_depth(
        &self,
        checkpoint_depth: usize,
    ) -> Result<Option<(C, Checkpoint)>, Self::Error> {
        Ok(if checkpoint_depth == 0 {
            None
        } else {
            self.checkpoints
                .iter()
                .rev()
                .nth(checkpoint_depth - 1)
                .map(|(id, c)| (id.clone(), c.clone()))
        })
    }

    fn min_checkpoint_id(&self) -> Result<Option<C>, Self::Error> {
        Ok(self.checkpoints.keys().next().cloned())
    }

    fn max_checkpoint_id(&self) -> Result<Option<C>, Self::Error> {
        Ok(self.checkpoints.keys().last().cloned())
    }

    fn with_checkpoints<F>(&mut self, limit: usize, mut callback: F) -> Result<(), Self::Error>
    where
        F: FnMut(&C, &Checkpoint) -> Result<(), Self::Error>,
    {
        for (cid, checkpoint) in self.checkpoints.iter().take(limit) {
            callback(cid, checkpoint)?
        }

        Ok(())
    }

    fn update_checkpoint_with<F>(
        &mut self,
        checkpoint_id: &C,
        update: F,
    ) -> Result<bool, Self::Error>
    where
        F: Fn(&mut Checkpoint) -> Result<(), Self::Error>,
    {
        if let Some(c) = self.checkpoints.get_mut(checkpoint_id) {
            update(c)?;
            return Ok(true);
        }

        Ok(false)
    }

    fn remove_checkpoint(&mut self, checkpoint_id: &C) -> Result<(), Self::Error> {
        self.checkpoints.remove(checkpoint_id);
        Ok(())
    }

    fn truncate_checkpoints(&mut self, checkpoint_id: &C) -> Result<(), Self::Error> {
        self.checkpoints.split_off(checkpoint_id);
        Ok(())
    }
}
