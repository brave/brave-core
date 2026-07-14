//! Implementation of an in-memory shard store with persistence.

use std::collections::{BTreeMap, BTreeSet};
use std::convert::Infallible;

use incrementalmerkletree::Address;

use super::{memory::MemoryShardStore, Checkpoint, ShardStore};
use crate::{LocatedPrunableTree, PrunableTree};

#[derive(Debug)]
enum Action<C> {
    TruncateShards(u64),
    RemoveCheckpoint(C),
    RemoveRetainedCheckpoint(C),
    TruncateCheckpointsRetaining(C),
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
            // TODO: Once MSRV is at least 1.82, replace this (and similar `expect()`s below) with:
            // `let Ok(_) = cache.add_checkpoint(checkpoint_id.clone(), checkpoint.clone());`
            cache
                .add_checkpoint(checkpoint_id.clone(), checkpoint.clone())
                .expect("error type is Infallible");
            Ok(())
        })?;
        // Seed the retention set so that pruning performed through the overlay observes the
        // backend's durable anchors (a retained checkpoint must never be pruned).
        for checkpoint_id in backend.retained_checkpoints()? {
            cache
                .add_retained_checkpoint(checkpoint_id)
                .expect("error type is Infallible");
        }

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
                Action::TruncateShards(index) => self.backend.truncate_shards(*index),
                Action::RemoveCheckpoint(checkpoint_id) => {
                    self.backend.remove_checkpoint(checkpoint_id)
                }
                Action::RemoveRetainedCheckpoint(checkpoint_id) => {
                    self.backend.remove_retained_checkpoint(checkpoint_id)
                }
                Action::TruncateCheckpointsRetaining(checkpoint_id) => {
                    self.backend.truncate_checkpoints_retaining(checkpoint_id)
                }
            }?;
        }
        self.deferred_actions.clear();

        for shard_root in self
            .cache
            .get_shard_roots()
            .expect("error type is Infallible")
        {
            self.backend.put_shard(
                self.cache
                    .get_shard(shard_root)
                    .expect("error type is Infallible")
                    .expect("known address"),
            )?;
        }
        self.backend
            .put_cap(self.cache.get_cap().expect("error type is Infallible"))?;

        let mut checkpoints = Vec::with_capacity(
            self.cache
                .checkpoint_count()
                .expect("error type is Infallible"),
        );
        self.cache
            .with_checkpoints(
                self.cache
                    .checkpoint_count()
                    .expect("error type is Infallible"),
                |checkpoint_id, checkpoint| {
                    checkpoints.push((checkpoint_id.clone(), checkpoint.clone()));
                    Ok(())
                },
            )
            .expect("error type is Infallible");
        for (checkpoint_id, checkpoint) in checkpoints {
            self.backend.add_checkpoint(checkpoint_id, checkpoint)?;
        }

        for checkpoint_id in self
            .cache
            .retained_checkpoints()
            .expect("error type is Infallible")
        {
            self.backend.add_retained_checkpoint(checkpoint_id)?;
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

    fn truncate_shards(&mut self, shard_index: u64) -> Result<(), Self::Error> {
        self.deferred_actions
            .push(Action::TruncateShards(shard_index));
        self.cache.truncate_shards(shard_index)
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
    fn for_each_checkpoint<F>(&self, limit: usize, callback: F) -> Result<(), Self::Error>
    where
        F: FnMut(&Self::CheckpointId, &Checkpoint) -> Result<(), Self::Error>,
    {
        self.cache.for_each_checkpoint(limit, callback)
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

    fn add_retained_checkpoint(
        &mut self,
        checkpoint_id: Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        self.cache.add_retained_checkpoint(checkpoint_id)
    }

    fn remove_retained_checkpoint(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        self.deferred_actions
            .push(Action::RemoveRetainedCheckpoint(checkpoint_id.clone()));
        self.cache.remove_retained_checkpoint(checkpoint_id)
    }

    fn retained_checkpoints(&self) -> Result<BTreeSet<Self::CheckpointId>, Self::Error> {
        self.cache.retained_checkpoints()
    }

    fn truncate_checkpoints_retaining(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        self.deferred_actions
            .push(Action::TruncateCheckpointsRetaining(checkpoint_id.clone()));
        self.cache.truncate_checkpoints_retaining(checkpoint_id)
    }
}

/// The error type for [`SparseCachingShardStore`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SparseStoreError {
    /// A shard that exists in the backend was accessed without having been preloaded.
    ///
    /// [`SparseCachingShardStore`] is strict: it caches only the working set explicitly passed to
    /// [`SparseCachingShardStore::with_preloaded`]. Returning `None` for a shard the backend
    /// actually contains would silently corrupt tree operations (the append path would build on an
    /// assumed-empty base and overwrite the real shard on flush), so the miss is surfaced as this
    /// error instead. The caller must preload every shard its operations will touch.
    NotPreloaded(u64),
}

impl std::fmt::Display for SparseStoreError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            SparseStoreError::NotPreloaded(index) => write!(
                f,
                "shard index {index} exists in the backend but was not preloaded into the sparse store"
            ),
        }
    }
}

impl std::error::Error for SparseStoreError {}

/// A [`ShardStore`] overlay that caches only an explicitly preloaded *working set* of shards,
/// rather than eagerly copying the entire backend.
///
/// Where [`CachingShardStore::load`] copies every shard out of the backend (`O(tree)` memory and
/// work -- costly for a store synced from a recent birthday, or for an async/wasm backend where
/// each read is expensive), `SparseCachingShardStore` copies only the shards passed to
/// [`Self::with_preloaded`], and on flush writes back only the delta accumulated since preload.
/// This makes the overlay `O(working set)` rather than `O(tree)` per checkpoint: preload the
/// shards a scan range will touch, mutate, then flush only what changed.
///
/// It maintains its own cache, dirty-tracking and delta flush; it does *not* wrap a
/// [`CachingShardStore`], so that type is unaffected by the sparse semantics. The behavioural
/// differences from the eager store are:
///
/// - **Strict read-miss policy.** [`ShardStore::get_shard`] for a shard that exists in the backend
///   but was not preloaded returns [`SparseStoreError::NotPreloaded`] rather than `None`, so a
///   missing preload fails loudly instead of silently corrupting the tree. (Hence the error type
///   is not [`Infallible`].)
/// - [`ShardStore::get_shard_roots`] reports the union of the cached shards and the shards present
///   in the backend at preload time, so cap/frontier logic sees the true set of shard roots.
///
/// The caller is responsible for preloading every shard its operations will touch -- **including the
/// frontier (last) shard**, on which appends build.
pub struct SparseCachingShardStore<S>
where
    S: ShardStore,
    S::H: Clone,
    S::CheckpointId: Clone + Ord,
{
    backend: S,
    /// Working-set cache: the preloaded shards plus any created since, the cap, and all
    /// checkpoints. Reads are served from here; the strict read-miss policy distinguishes a
    /// genuine absence from an un-preloaded backend shard via `backend_shard_roots`.
    cache: MemoryShardStore<S::H, S::CheckpointId>,
    /// Root addresses of the shards present in the backend at preload time, keyed by shard index.
    /// Distinguishes "exists in the backend but not preloaded" (-> [`SparseStoreError::NotPreloaded`])
    /// from "absent everywhere" (-> `None`).
    backend_shard_roots: BTreeMap<u64, Address>,
    /// Truncations/removals deferred to the next flush, applied in order before the dirty writes.
    deferred_actions: Vec<Action<S::CheckpointId>>,
    /// Root addresses of shards written since preload / the last flush, keyed by shard index.
    dirty_shards: BTreeMap<u64, Address>,
    /// Whether the cap has been written since preload / the last flush.
    cap_dirty: bool,
    /// Identifiers of checkpoints added or updated since preload / the last flush.
    dirty_checkpoints: BTreeSet<S::CheckpointId>,
    /// Checkpoint identifiers marked for retention since preload / the last flush.
    /// `remove_retained_checkpoint` cancels a pending add (the deferred removal then
    /// replays against the backend, which tolerates ids it never stored).
    dirty_retained: BTreeSet<S::CheckpointId>,
}

impl<S> SparseCachingShardStore<S>
where
    S: ShardStore,
    S::H: Clone,
    S::CheckpointId: Clone + Ord,
{
    /// Loads a sparse store from `backend`, preloading only the shards identified by `shards`
    /// (along with the cap and all checkpoints). Shards not listed are left in the backend;
    /// accessing one that the backend contains returns [`SparseStoreError::NotPreloaded`].
    ///
    /// The caller must preload every shard its tree operations will touch, including the frontier
    /// (last) shard on which appends build. Addresses in `shards` not present in the backend are
    /// ignored.
    pub fn with_preloaded(
        mut backend: S,
        shards: impl IntoIterator<Item = Address>,
    ) -> Result<Self, S::Error> {
        let backend_shard_roots = backend
            .get_shard_roots()?
            .into_iter()
            .map(|addr| (addr.index(), addr))
            .collect();

        let mut cache = MemoryShardStore::empty();
        for shard_root in shards {
            if let Some(shard) = backend.get_shard(shard_root)? {
                cache
                    .put_shard(shard)
                    .expect("MemoryShardStore error type is Infallible");
            }
        }
        cache
            .put_cap(backend.get_cap()?)
            .expect("MemoryShardStore error type is Infallible");
        backend.with_checkpoints(backend.checkpoint_count()?, |checkpoint_id, checkpoint| {
            cache
                .add_checkpoint(checkpoint_id.clone(), checkpoint.clone())
                .expect("MemoryShardStore error type is Infallible");
            Ok(())
        })?;
        // Seed the retention set so that pruning performed through the overlay observes the
        // backend's durable anchors (a retained checkpoint must never be pruned).
        for checkpoint_id in backend.retained_checkpoints()? {
            cache
                .add_retained_checkpoint(checkpoint_id)
                .expect("MemoryShardStore error type is Infallible");
        }

        Ok(Self {
            backend,
            cache,
            backend_shard_roots,
            deferred_actions: vec![],
            dirty_shards: BTreeMap::new(),
            cap_dirty: false,
            dirty_checkpoints: BTreeSet::new(),
            dirty_retained: BTreeSet::new(),
        })
    }

    /// Flushes the accumulated delta to the backend and returns it. See [`Self::flush_delta`].
    pub fn flush(mut self) -> Result<S, S::Error> {
        self.flush_delta()?;
        Ok(self.backend)
    }

    /// Flushes the changes accumulated since preload (or the previous `flush_delta`) to the
    /// backend, leaving `self` usable for further operations.
    ///
    /// Only the delta is written: the deferred truncations/removals are applied in order, then
    /// every shard and checkpoint that was modified and still exists in the cache is written
    /// (along with the cap, if it changed). State that was not modified is left untouched in the
    /// backend rather than being rewritten, and a modification that was subsequently truncated
    /// away is skipped (the deferred truncation already removed it from the backend).
    pub fn flush_delta(&mut self) -> Result<(), S::Error> {
        for action in std::mem::take(&mut self.deferred_actions) {
            match action {
                Action::TruncateShards(index) => self.backend.truncate_shards(index),
                Action::RemoveCheckpoint(checkpoint_id) => {
                    self.backend.remove_checkpoint(&checkpoint_id)
                }
                Action::RemoveRetainedCheckpoint(checkpoint_id) => {
                    self.backend.remove_retained_checkpoint(&checkpoint_id)
                }
                Action::TruncateCheckpointsRetaining(checkpoint_id) => {
                    self.backend.truncate_checkpoints_retaining(&checkpoint_id)
                }
            }?;
        }

        for shard_root in std::mem::take(&mut self.dirty_shards).into_values() {
            if let Some(shard) = self
                .cache
                .get_shard(shard_root)
                .expect("error type is Infallible")
            {
                self.backend.put_shard(shard)?;
            }
        }

        if self.cap_dirty {
            self.backend
                .put_cap(self.cache.get_cap().expect("error type is Infallible"))?;
            self.cap_dirty = false;
        }

        for checkpoint_id in std::mem::take(&mut self.dirty_checkpoints) {
            if let Some(checkpoint) = self
                .cache
                .get_checkpoint(&checkpoint_id)
                .expect("error type is Infallible")
            {
                self.backend.add_checkpoint(checkpoint_id, checkpoint)?;
            }
        }

        for checkpoint_id in std::mem::take(&mut self.dirty_retained) {
            self.backend.add_retained_checkpoint(checkpoint_id)?;
        }

        Ok(())
    }
}

impl<S> ShardStore for SparseCachingShardStore<S>
where
    S: ShardStore,
    S::H: Clone,
    S::CheckpointId: Clone + Ord,
{
    type H = S::H;
    type CheckpointId = S::CheckpointId;
    type Error = SparseStoreError;

    fn get_shard(
        &self,
        shard_root: Address,
    ) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        // A cache hit (preloaded, or created since preload) is returned directly.
        if let Some(shard) = self.cache.get_shard(shard_root).map_err(|e| match e {})? {
            return Ok(Some(shard));
        }
        // Not cached: if the backend holds this shard, the caller failed to preload it. Returning
        // `None` here would silently corrupt tree operations, so fail loudly instead.
        if self.backend_shard_roots.contains_key(&shard_root.index()) {
            return Err(SparseStoreError::NotPreloaded(shard_root.index()));
        }
        // Absent in both the cache and the backend.
        Ok(None)
    }

    fn last_shard(&self) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        self.cache.last_shard().map_err(|e| match e {})
    }

    fn put_shard(&mut self, subtree: LocatedPrunableTree<Self::H>) -> Result<(), Self::Error> {
        self.dirty_shards
            .insert(subtree.root_addr.index(), subtree.root_addr);
        self.cache.put_shard(subtree).map_err(|e| match e {})
    }

    fn get_shard_roots(&self) -> Result<Vec<Address>, Self::Error> {
        // The tree must see every shard root that exists anywhere: the cached shards (preloaded or
        // created since) unioned with the shards present in the backend at preload time.
        let mut roots = self.backend_shard_roots.clone();
        for addr in self.cache.get_shard_roots().map_err(|e| match e {})? {
            roots.insert(addr.index(), addr);
        }
        Ok(roots.into_values().collect())
    }

    fn truncate_shards(&mut self, shard_index: u64) -> Result<(), Self::Error> {
        // Shards at or above `shard_index` are being removed; drop them from the backend snapshot
        // so they're no longer reported (or treated as not-preloaded) before the flush applies it.
        self.backend_shard_roots.split_off(&shard_index);
        // Apply to the cache now; defer the backend truncation to the next flush.
        self.deferred_actions
            .push(Action::TruncateShards(shard_index));
        self.cache
            .truncate_shards(shard_index)
            .map_err(|e| match e {})
    }

    fn get_cap(&self) -> Result<PrunableTree<Self::H>, Self::Error> {
        self.cache.get_cap().map_err(|e| match e {})
    }

    fn put_cap(&mut self, cap: PrunableTree<Self::H>) -> Result<(), Self::Error> {
        self.cap_dirty = true;
        self.cache.put_cap(cap).map_err(|e| match e {})
    }

    fn add_checkpoint(
        &mut self,
        checkpoint_id: Self::CheckpointId,
        checkpoint: Checkpoint,
    ) -> Result<(), Self::Error> {
        self.dirty_checkpoints.insert(checkpoint_id.clone());
        self.cache
            .add_checkpoint(checkpoint_id, checkpoint)
            .map_err(|e| match e {})
    }

    fn checkpoint_count(&self) -> Result<usize, Self::Error> {
        self.cache.checkpoint_count().map_err(|e| match e {})
    }

    fn get_checkpoint(
        &self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<Option<Checkpoint>, Self::Error> {
        self.cache
            .get_checkpoint(checkpoint_id)
            .map_err(|e| match e {})
    }

    fn get_checkpoint_at_depth(
        &self,
        checkpoint_depth: usize,
    ) -> Result<Option<(Self::CheckpointId, Checkpoint)>, Self::Error> {
        self.cache
            .get_checkpoint_at_depth(checkpoint_depth)
            .map_err(|e| match e {})
    }

    fn min_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        self.cache.min_checkpoint_id().map_err(|e| match e {})
    }

    fn max_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        self.cache.max_checkpoint_id().map_err(|e| match e {})
    }

    fn with_checkpoints<F>(&mut self, limit: usize, mut callback: F) -> Result<(), Self::Error>
    where
        F: FnMut(&Self::CheckpointId, &Checkpoint) -> Result<(), Self::Error>,
    {
        // The cache is `Infallible`, so it can't carry the caller's `Self::Error`: stash the
        // first error from `callback` and surface it after the remaining (no-op) iterations.
        let mut result = Ok(());
        self.cache
            .with_checkpoints(limit, |checkpoint_id, checkpoint| {
                if result.is_ok() {
                    result = callback(checkpoint_id, checkpoint);
                }
                Ok(())
            })
            .map_err(|e| match e {})?;
        result
    }

    fn for_each_checkpoint<F>(&self, limit: usize, mut callback: F) -> Result<(), Self::Error>
    where
        F: FnMut(&Self::CheckpointId, &Checkpoint) -> Result<(), Self::Error>,
    {
        let mut result = Ok(());
        self.cache
            .for_each_checkpoint(limit, |checkpoint_id, checkpoint| {
                if result.is_ok() {
                    result = callback(checkpoint_id, checkpoint);
                }
                Ok(())
            })
            .map_err(|e| match e {})?;
        result
    }

    fn update_checkpoint_with<F>(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
        update: F,
    ) -> Result<bool, Self::Error>
    where
        F: Fn(&mut Checkpoint) -> Result<(), Self::Error>,
    {
        // `update` is `Fn`, so stash its error through a `Cell` (the cache is `Infallible`).
        let stashed: std::cell::Cell<Option<Self::Error>> = std::cell::Cell::new(None);
        let updated = self
            .cache
            .update_checkpoint_with(checkpoint_id, |checkpoint| {
                if let Err(e) = update(checkpoint) {
                    stashed.set(Some(e));
                }
                Ok(())
            })
            .map_err(|e| match e {})?;
        // Mark dirty whenever the checkpoint existed (matching the eager store), then surface any
        // error the caller's `update` stashed.
        if updated {
            self.dirty_checkpoints.insert(checkpoint_id.clone());
        }
        match stashed.into_inner() {
            Some(e) => Err(e),
            None => Ok(updated),
        }
    }

    fn remove_checkpoint(&mut self, checkpoint_id: &Self::CheckpointId) -> Result<(), Self::Error> {
        self.deferred_actions
            .push(Action::RemoveCheckpoint(checkpoint_id.clone()));
        self.cache
            .remove_checkpoint(checkpoint_id)
            .map_err(|e| match e {})
    }

    fn add_retained_checkpoint(
        &mut self,
        checkpoint_id: Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        self.dirty_retained.insert(checkpoint_id.clone());
        self.cache
            .add_retained_checkpoint(checkpoint_id)
            .map_err(|e| match e {})
    }

    fn remove_retained_checkpoint(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        // Cancel a not-yet-flushed add so the deferred removal below cannot be undone by the
        // dirty-write phase (deferred actions replay before dirty writes).
        self.dirty_retained.remove(checkpoint_id);
        self.deferred_actions
            .push(Action::RemoveRetainedCheckpoint(checkpoint_id.clone()));
        self.cache
            .remove_retained_checkpoint(checkpoint_id)
            .map_err(|e| match e {})
    }

    fn retained_checkpoints(&self) -> Result<BTreeSet<Self::CheckpointId>, Self::Error> {
        self.cache.retained_checkpoints().map_err(|e| match e {})
    }

    fn truncate_checkpoints_retaining(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        self.deferred_actions
            .push(Action::TruncateCheckpointsRetaining(checkpoint_id.clone()));
        self.cache
            .truncate_checkpoints_retaining(checkpoint_id)
            .map_err(|e| match e {})
    }
}

#[cfg(test)]
mod tests {
    use incrementalmerkletree::{Address, Hashable, Marking, Position, Retention};
    use incrementalmerkletree_testing::{
        append_str, check_operations, unmark, witness, CombinedTree, Operation, TestHashable, Tree,
    };

    use super::{CachingShardStore, SparseCachingShardStore, SparseStoreError};
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

    fn sparse_check_equal(
        mut lhs: MemoryShardStore<String, u64>,
        rhs: SparseCachingShardStore<MemoryShardStore<String, u64>>,
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

    // A sparse store seeded from an empty backend behaves exactly like the eager reference for
    // fresh appends (every shard is created in the cache, so no preload is required), and its
    // delta flush reproduces the same backend state.
    #[test]
    fn sparse_store_matches_eager_on_fresh_appends() {
        use Retention::*;

        let mut lhs = MemoryShardStore::empty();
        let mut rhs = SparseCachingShardStore::with_preloaded(
            MemoryShardStore::<String, u64>::empty(),
            std::iter::empty::<Address>(),
        )
        .unwrap();
        let mut tree = CombinedTree::<String, _, _, _>::new(
            ShardTree::<_, 4, 3>::new(&mut lhs, 100),
            ShardTree::<_, 4, 3>::new(&mut rhs, 100),
        );

        assert!(tree.append(
            String::from_u64(0),
            Checkpoint {
                id: 1,
                marking: Marking::Marked,
            },
        ));
        for i in 1..8 {
            assert!(tree.append(String::from_u64(i), Ephemeral));
        }

        sparse_check_equal(lhs, rhs);
    }

    // The strict read-miss policy: a preloaded shard is a hit, a shard that exists in the backend
    // but wasn't preloaded errors (rather than silently reading back as empty), and a shard absent
    // everywhere reads back as `None`. `get_shard_roots` reports the union of both sets.
    #[test]
    fn sparse_store_strict_read_miss_policy() {
        use Retention::*;

        // Build a backend containing two shards (indices 0 and 1) via the eager path.
        let backend = {
            let mut lhs = MemoryShardStore::<String, u64>::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            {
                let mut tree = CombinedTree::<String, _, _, _>::new(
                    ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                    ShardTree::<_, 4, 3>::new(&mut rhs, 100),
                );
                for i in 0..16 {
                    assert!(tree.append(String::from_u64(i), Ephemeral));
                }
            }
            rhs.flush().unwrap()
        };

        let roots = backend.get_shard_roots().unwrap();
        assert_eq!(roots.len(), 2);
        let (shard0, shard1) = (roots[0], roots[1]);

        // Preload only shard 0.
        let sparse = SparseCachingShardStore::with_preloaded(backend, [shard0]).unwrap();

        // A preloaded shard is a cache hit.
        assert!(sparse.get_shard(shard0).unwrap().is_some());
        // A shard that exists in the backend but wasn't preloaded fails loudly.
        assert_eq!(
            sparse.get_shard(shard1),
            Err(SparseStoreError::NotPreloaded(shard1.index())),
        );
        // A shard absent everywhere reads back as `None`.
        let absent = Address::from_parts(shard0.level(), 99);
        assert_eq!(sparse.get_shard(absent), Ok(None));
        // `get_shard_roots` reports the union of cached and backend-resident shards.
        assert_eq!(sparse.get_shard_roots().unwrap(), roots);
    }

    #[test]
    fn load_seeds_retained_checkpoints() {
        use std::collections::BTreeSet;

        use crate::store::Checkpoint;

        let mut backend = MemoryShardStore::<String, u64>::empty();
        backend.add_checkpoint(5, Checkpoint::tree_empty()).unwrap();
        backend.add_retained_checkpoint(5).unwrap();

        // The overlay must observe the backend's durable retention set: pruning performed
        // through the overlay (`ShardTree::prune_excess_checkpoints`) consults
        // `retained_checkpoints` to decide what it may remove, so a checkpoint retained in
        // the backend but invisible to the cache could be pruned away.
        let store = CachingShardStore::load(backend).unwrap();
        assert_eq!(store.retained_checkpoints().unwrap(), BTreeSet::from([5]));

        // A load -> flush round trip must also preserve the retention marker.
        let backend = store.flush().unwrap();
        assert_eq!(backend.retained_checkpoints().unwrap(), BTreeSet::from([5]));
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
                tree.root(None).unwrap(),
                String::combine_all(tree.depth(), &[]),
            );
            assert!(tree.append(String::from_u64(0), Ephemeral));
            assert_eq!(
                tree.root(None).unwrap(),
                String::combine_all(tree.depth(), &[0]),
            );
            assert!(tree.append(String::from_u64(1), Ephemeral));
            assert_eq!(
                tree.root(None).unwrap(),
                String::combine_all(tree.depth(), &[0, 1]),
            );
            assert!(tree.append(String::from_u64(2), Ephemeral));
            assert_eq!(
                tree.root(None).unwrap(),
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
                    marking: Marking::Marked,
                },
            ));
            for _ in 0..3 {
                assert!(t.append(String::from_u64(0), Ephemeral));
            }
            assert_eq!(
                t.root(None).unwrap(),
                String::combine_all(t.depth(), &[0, 0, 0, 0])
            );

            check_equal(lhs, rhs);
        }
    }

    #[test]
    fn sparse_flush_delta_composes_across_incremental_flushes() {
        use Retention::*;

        let mut lhs = MemoryShardStore::<_, u64>::empty();
        let mut rhs = SparseCachingShardStore::with_preloaded(
            MemoryShardStore::<String, u64>::empty(),
            std::iter::empty::<Address>(),
        )
        .unwrap();

        // Batch 1: a few leaves plus a checkpoint.
        {
            let mut tree = CombinedTree::<String, _, _, _>::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );
            assert!(tree.append(
                String::from_u64(0),
                Checkpoint {
                    id: 1,
                    marking: Marking::Marked,
                },
            ));
            for i in 1..5 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
            }
        }

        // An intermediate delta flush writes batch 1 and leaves `rhs` usable; the dirty
        // state must be fully reset so the next flush writes only what changes afterwards.
        rhs.flush_delta().unwrap();
        assert!(rhs.dirty_shards.is_empty());
        assert!(!rhs.cap_dirty);
        assert!(rhs.dirty_checkpoints.is_empty());

        // Batch 2: more leaves (re-touching shard 0 and crossing into shard 1) plus another
        // checkpoint -- mutating after a flush exercises the dirty-state reset.
        {
            let mut tree = CombinedTree::<String, _, _, _>::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );
            for i in 5..12 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
            }
            assert!(tree.append(
                String::from_u64(12),
                Checkpoint {
                    id: 2,
                    marking: Marking::Marked,
                },
            ));
        }

        // The final flush (inside `sparse_check_equal`) writes only batch 2's delta; the backend
        // must still match the directly-mutated reference, proving the flushes compose.
        sparse_check_equal(lhs, rhs);
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

            assert!(tree.append(
                String::from_u64(0),
                Checkpoint {
                    id: 0,
                    marking: Marking::Marked
                }
            ));
            assert_eq!(
                tree.witness(Position::from(0), 0),
                Some(vec![
                    String::empty_root(0.into()),
                    String::empty_root(1.into()),
                    String::empty_root(2.into()),
                    String::empty_root(3.into())
                ])
            );

            assert!(tree.append(
                String::from_u64(1),
                Checkpoint {
                    id: 1,
                    marking: Marking::None
                }
            ));
            assert_eq!(
                tree.witness(0.into(), 0),
                Some(vec![
                    String::from_u64(1),
                    String::empty_root(1.into()),
                    String::empty_root(2.into()),
                    String::empty_root(3.into())
                ])
            );

            assert!(tree.append(
                String::from_u64(2),
                Checkpoint {
                    id: 2,
                    marking: Marking::Marked
                }
            ));
            assert_eq!(
                tree.witness(Position::from(2), 0),
                Some(vec![
                    String::empty_root(0.into()),
                    String::combine_all(1, &[0, 1]),
                    String::empty_root(2.into()),
                    String::empty_root(3.into())
                ])
            );

            assert!(tree.append(
                String::from_u64(3),
                Checkpoint {
                    id: 3,
                    marking: Marking::None
                }
            ));
            assert_eq!(
                tree.witness(Position::from(2), 0),
                Some(vec![
                    String::from_u64(3),
                    String::combine_all(1, &[0, 1]),
                    String::empty_root(2.into()),
                    String::empty_root(3.into())
                ])
            );

            assert!(tree.append(
                String::from_u64(4),
                Checkpoint {
                    id: 4,
                    marking: Marking::None
                }
            ));
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
            assert!(tree.append(
                String::from_u64(7),
                Checkpoint {
                    id: 0,
                    marking: Marking::None
                }
            ));

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
            assert!(tree.append(
                String::from_u64(6),
                Checkpoint {
                    id: 0,
                    marking: Marking::None
                }
            ));

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
            assert!(tree.append(
                String::from_u64(11),
                Checkpoint {
                    id: 0,
                    marking: Marking::None
                }
            ));

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
                    marking: Marking::Marked,
                },
            ));
            assert!(tree.rewind(0));
            for i in 1..4 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
            }
            assert!(tree.append(String::from_u64(4), Marked));
            for i in 5..8 {
                assert!(tree.append(String::from_u64(i), Ephemeral));
            }
            tree.checkpoint(2);
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
                    marking: Marking::Marked,
                },
            ));
            assert!(tree.append(String::from_u64(7), Ephemeral));
            assert!(tree.rewind(0));
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
            assert!(tree.append(
                String::from_u64(15),
                Checkpoint {
                    id: 0,
                    marking: Marking::None
                }
            ));

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
                .chain(Some(Append(
                    String::from_u64(13),
                    Checkpoint {
                        id: 0,
                        marking: Marking::None,
                    },
                )))
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
                        marking: Marking::Marked,
                    },
                ),
                Append(String::from_u64(4), Marked),
                Operation::Checkpoint(2),
                Append(
                    String::from_u64(5),
                    Checkpoint {
                        id: 3,
                        marking: Marking::None,
                    },
                ),
                Append(
                    String::from_u64(6),
                    Checkpoint {
                        id: 4,
                        marking: Marking::None,
                    },
                ),
                Append(
                    String::from_u64(7),
                    Checkpoint {
                        id: 5,
                        marking: Marking::None,
                    },
                ),
                Witness(3u64.into(), 4),
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
                        marking: Marking::Marked,
                    },
                ),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Append(
                    String::from_u64(0),
                    Checkpoint {
                        id: 2,
                        marking: Marking::None,
                    },
                ),
                Append(String::from_u64(0), Ephemeral),
                Append(String::from_u64(0), Ephemeral),
                Witness(Position::from(3), 0),
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
                        marking: Marking::None,
                    },
                ),
                Rewind(0),
                Rewind(1),
                Witness(Position::from(7), 2),
            ];

            let mut lhs = MemoryShardStore::empty();
            let mut rhs = CachingShardStore::load(MemoryShardStore::empty()).unwrap();
            let mut tree = CombinedTree::new(
                ShardTree::<_, 4, 3>::new(&mut lhs, 100),
                ShardTree::<_, 4, 3>::new(&mut rhs, 100),
            );

            // There is no checkpoint at depth 2, so we cannot compute a witness
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
                        marking: Marking::Marked,
                    },
                ),
                Append(
                    String::from_u64(0),
                    Checkpoint {
                        id: 4,
                        marking: Marking::None,
                    },
                ),
                Witness(Position::from(2), 1),
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

            assert!(!t.rewind(0));

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
            assert!(t.rewind(0));

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
            assert!(t.rewind(0));
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
            assert!(t.rewind(0));

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
            assert!(t.rewind(0));
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
            assert!(t.rewind(1));
            t.append("b".to_string(), Retention::Ephemeral);
            assert!(t.rewind(0));
            t.append("b".to_string(), Retention::Ephemeral);
            assert_eq!(t.root(None).unwrap(), "ab______________");

            check_equal(lhs, rhs);
        }
    }

    #[test]
    fn check_remove_mark() {
        let samples = [
            vec![
                append_str("a", Retention::Ephemeral),
                append_str(
                    "a",
                    Retention::Checkpoint {
                        id: 1,
                        marking: Marking::Marked,
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
            assert!(tree.rewind(0));
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

        let samples = [
            vec![
                append_str("x", Retention::Marked),
                Checkpoint(1),
                Rewind(0),
                unmark(0),
            ],
            vec![
                append_str("d", Retention::Marked),
                Checkpoint(1),
                unmark(0),
                Rewind(0),
                unmark(0),
            ],
            vec![
                append_str("o", Retention::Marked),
                Checkpoint(1),
                Checkpoint(2),
                unmark(0),
                Rewind(0),
                Rewind(1),
            ],
            vec![
                append_str("s", Retention::Marked),
                append_str("m", Retention::Ephemeral),
                Checkpoint(1),
                unmark(0),
                Rewind(0),
                unmark(0),
                unmark(0),
            ],
            vec![
                append_str("a", Retention::Marked),
                Checkpoint(1),
                Rewind(0),
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
                Rewind(1),
                append_str("b", Retention::Ephemeral),
                witness(0, 0),
            ],
            vec![
                append_str("a", Retention::Marked),
                Checkpoint(1),
                Checkpoint(2),
                Rewind(1),
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
