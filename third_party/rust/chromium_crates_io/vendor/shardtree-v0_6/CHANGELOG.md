# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

## [0.6.2] - 2026-02-20

### Added
- `shardtree::prunable::{LocatedPrunableTree::frontier, FrontierError}`
- `shardtree::ShardTree::frontier`

## [0.6.1] - 2025-01-30

### Changed
- Update to `incrementalmerkletree 0.8.1`, `incrementalmerkletree-testing 0.3.0`.
- `shardtree::BatchInsertionResult.max_insert_position` now has type `Position`
  instead of `Option<Position>` (all APIs return `Option<BatchInsertionResult>`
  and use `None` at that level to represent "no leaves inserted").
- `shardtree::LocatedTree::from_parts` now returns `Option<Self>` (returning
  `None` if the provided `Address` and `Tree` are inconsistent).

## [0.6.0] - 2025-01-28

YANKED due to dependency incompatibilities, please use `shardtree 0.6.1` instead.

## [0.5.0] - 2024-10-04

This release includes a significant refactoring and rework of several methods
of the `shardtree::ShardTree` type and the `shardtree::store::ShardStore`
trait. Please read the notes for this release carefully as the semantics of
important methods have changed. These changes may require changes to clients of
this crate; in particular, the existence of a checkpoint is required for all
witnessing and rewinding operations.

### Added
- `shardtree::store::ShardStore::for_each_checkpoint`
- `shardtree::ShardTree::truncate_to_checkpoint_depth`. This replaces
  the `ShardTree::truncate_to_depth` method, with modified semantics such that
  the provided `checkpoint_depth` argument is now treated strictly as a
  zero-based index into the checkpoints applied to the tree, in reverse order
  of checkpoint identifier. It is no longer possible to truncate the tree if no
  checkpoints have been created.
- `shardtree::ShardTree::truncate_to_checkpoint` replaces
  `ShardTree::truncate_removing_checkpoint`. Instead of removing
  the checkpoint, this replacement method removes all state from the tree
  related to leaves having positions greater than the checkpointed position,
  but unlike `truncate_removing_checkpoint` it leaves the checkpoint itself
  in place.
- `shardtree::store::ShardStore::truncate_shards` replaces
  `ShardStore::truncate`. Instead of taking an `Address` and requiring that
  implementations impose additional runtime restriction on the level of that
  address, the replacement method directly takes a shard index.
- `ShardStore::truncate_truncate_checkpoints_retaining` replaces
  `ShardStore::truncate_checkpoints`. Instead of removing the checkpoint
  having the specified identifier, the associated checkpoint should be retained
  but any additional metadata stored with the checkpoint, such as information
  about marks removed after the creation of the checkpoint, should be deleted.

### Changed
- MSRV is now 1.64
- `shardtree::ShardTree`:
  - `ShardTree::max_leaf_position` now takes its `checkpoint_depth` argument
    as `Option<usize>` instead of `usize`. The semantics of this method are now
    changed such that if a checkpoint depth is provided, it is now treated
    strictly as a zero-based index into the checkpoints of the tree in reverse
    order of checkpoint identifier, and an error is returned if no checkpoint
    exists at the given index. A `None` value passed for this argument causes
    it to return the maximum position among all leaves added to the tree.
  - `ShardTree::root_at_checkpoint_id` and `root_at_checkpoint_id_caching` now
    each return the computed root as an optional value, returning `Ok(None)` if
    the checkpoint corresponding to the requested identifier does not exist.
  - `ShardTree::root_at_checkpoint_depth` and `root_at_checkpoint_depth_caching`
    now take their `checkpoint_depth` argument as `Option<usize>` instead of
    `usize`. The semantics of these methods are now changed such that if a
    checkpoint depth is provided, it is now treated strictly as a zero-based
    index into the checkpoints of the tree in reverse order of checkpoint
    identifier, and an error is returned if no checkpoint exists at the given
    index. A `None` value passed for this argument causes it to return the root
    computed over all of the leaves in the tree. These methods now each return
    the computed root as an optional value, returning `Ok(None)` if no checkpoint
    exist at the requested checkpoint depth.
  - `ShardTree::witness_at_checkpoint_id` and `witness_at_checkpoint_id_caching`
    now each return the computed witness as an optional value, returning
    `Ok(None)` if the checkpoint corresponding to the requested identifier does
    not exist.
  - `ShardTree::witness_at_checkpoint_depth` and `witness_at_checkpoint_depth_caching`
    now each return the computed witness as an optional value, returning
    `Ok(None)` if no checkpoint was available at the given checkpoint depth. The
    semantics of this method are now changed such that if a checkpoint depth is
    provided, it is now treated strictly as a zero-based index into the
    checkpoints of the tree in reverse order of checkpoint identifier. IT IS NO
    LONGER POSSIBLE TO COMPUTE A WITNESS WITHOUT A CHECKPOINT BEING PRESENT IN
    THE TREE.
- `shardtree::store::ShardStore`:
  - The semantics of `ShardStore::get_checkpoint_at_depth` HAVE CHANGED WITHOUT
    CHANGES TO THE METHOD SIGNATURE. The `checkpoint_depth` argument to this
    method is now treated strictly as a zero-based index into the checkpoints
    of the tree in reverse order of checkpoint identifier. Previously, this
    method always returned `None` for `checkpoint_depth == 0`, and
    `checkpoint_depth` was otherwise treated as a 1-based index instead of a
    0-based index.

### Removed
- `shardtree::ShardTree::truncate_to_depth` has been replaced by
  `ShardTree::truncate_to_checkpoint_depth`
- `shardtree::ShardTree::truncate_removing_checkpoint` has been replaced by
  `ShardTree::truncate_to_checkpoint`
- `shardtree::store::ShardStore::truncate` has been replaced by
  `ShardStore::truncate_shards`
- `shardtree::store::ShardStore::truncate_checkpoints` has been replaced by
  `ShardStore::truncate_checkpoints_retaining`

## [0.4.0] - 2024-08-12

This is a bugfix release that fixes a couple of subtle problems related to
pruning in the presence of inserted `Frontier` nodes. See the `Removed` and
`Fixed` sections below for additional details.

### Added
- `shardtree::tree::Tree::{is_leaf, map, try_map, empty_pruned}`
- `shardtree::tree::LocatedTree::{map, try_map}`
- `shardtree::prunable::PrunableTree::{has_computable_root, is_full}`
- `shardtree::prunable::LocatedPrunableTree::{max_position}`

### Changed
- Updated to use `incrementalmerkletree` v0.6.

### Removed
- `shardtree::tree::LocatedTree::max_position` did not behave correctly regarding
  annotated parent nodes. Use `LocatedPrunableTree::max_position` instead.
- `shardtree::tree::Tree::is_complete` was somewhat misleadingly named, as `Nil`
  nodes that were inserted as a consequence of insertion after pruning could be
  interpreted as rendering the tree incomplete. Use `PrunableTree::has_computable_root`
  instead to determine whether it is possible to compute the root of a tree.

### Fixed
- Fixes an error that could occur if an inserted `Frontier` node was
  interpreted as a node that had actually had its value observed as though it
  had been inserted using the ordinary tree insertion methods.
- Fixes an error in an internal method that could result in subtree root
  annotation data being discarded when pruning a `Parent` node having
  `Nil` nodes for both its left and right children.

## [0.3.2] - 2024-12-09
- Replaces `unwrap` calls with `expect` calls & documents panics.

## [0.3.1] - 2024-04-03

### Fixed
- Fixes a missing transitive dependency when using the `test-dependencies` feature flag.

## [0.3.0] - 2024-03-25

### Added
- `ShardTree::{store, store_mut}`
- `ShardTree::insert_frontier`

### Changed
- `shardtree::error::InsertionError` has new variant `MarkedRetentionInvalid`

## [0.2.0] - 2023-11-07

### Added
- `ShardTree::{root_at_checkpoint_id, root_at_checkpoint_id_caching}`
- `ShardTree::{witness_at_checkpoint_id, witness_at_checkpoint_id_caching}`

### Changed
- `ShardTree::root_at_checkpoint` and `ShardTree::root_at_checkpoint_caching` have
  been renamed to `root_at_checkpoint_depth` and `root_at_checkpoint_depth_caching`,
  respectively.
- `ShardTree::witness` and `ShardTree::witness_caching` have
  been renamed to `witness_at_checkpoint_depth` and `witness_at_checkpoint_depth_caching`,
  respectively.

## [0.1.0] - 2023-09-08

Initial release!
