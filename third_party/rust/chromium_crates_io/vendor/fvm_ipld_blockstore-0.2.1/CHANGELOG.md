# Changelog

Changes to the FVM's Blockstore abstraction

## [Unreleased]

## 0.2.1 [2024-04-30]

- Constify `Block::new`.
- Implement `PartialEq` between blocks with different underlying buffer types.
- Improve `Block` `Debug` implementation.

## 0.2.0 [2023-06-28]

Breaking Changes:

- Update cid/multihash. This is a breaking change as it affects the API.

## 0.1.2 [2023-05-03]

- Impl blockstore for `Arc<BS>`.
- Add a `copy_to` method to the memory blockstore.

## 0.1.1 [2022-05-16]

Remove blake2b feature from multihash (we don't need it here). This is technically a breaking change
as downstream could be relying on this (by accident), but they shouldn't be, so we're not going to
bother with a minor version bump.
