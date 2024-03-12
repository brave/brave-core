# Changelog

Changes to the FVM's Blockstore abstraction

## [Unreleased]

## 0.1.2 [2022-05-16]

Remove blake2b feature from multihash (we don't need it here). This is technically a breaking change
as downstream could be relying on this (by accident), but they shouldn't be, so we're not going to
bother with a minor version bump.
