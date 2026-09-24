# Changelog
All notable changes to this library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this library adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.1] - 2025-02-20
### Added
- `zcash_spec::PrfExpand::REGISTERED_ZIP32_CHILD` (for tagged ZIP 32 child
  derivation).
- `zcash_spec::PrfExpand::ADHOC_ZIP32_CHILD` (renamed and retyped per ZIP 32).
- `zcash_spec::VariableLengthSlice`

### Changed
- `zcash_spec::PrfExpand::ORCHARD_ZIP32_CHILD` now has type
  `PrfExpand<([u8; 32], [u8; 4], [u8; 1], VariableLengthSlice)>` due to
  ZIP 32 changes.

### Removed
- `zcash_spec::PrfExpand::ARBITRARY_ZIP32_CHILD` (use `ADHOC_ZIP32_CHILD`
  instead).

Note: There was no v0.2.0 release because the tag was originally pushed
incorrectly.

## [0.1.2] - 2024-10-01
### Added
- `zcash_spec::PrfExpand::ARBITRARY_ZIP32_CHILD`

## [0.1.1] - 2024-09-20
### Fixed
- The `std` default feature of `blake2b_simd` is now disabled, to enable no-std
  usage.

## [0.1.0] - 2023-12-07
Initial release.
