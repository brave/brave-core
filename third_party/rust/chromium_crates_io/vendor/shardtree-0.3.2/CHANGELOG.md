# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

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
