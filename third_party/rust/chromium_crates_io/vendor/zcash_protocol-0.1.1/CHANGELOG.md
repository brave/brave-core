# Changelog
All notable changes to this library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this library adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.1] - 2024-03-25
### Added
- `zcash_protocol::memo`:
  - `impl TryFrom<&MemoBytes> for Memo`

### Removed
- `unstable-nu6` and `zfuture` feature flags (use `--cfg zcash_unstable=\"nu6\"`
  or `--cfg zcash_unstable=\"zfuture\"` in `RUSTFLAGS` and `RUSTDOCFLAGS`
  instead).

## [0.1.0] - 2024-03-06
The entries below are relative to the `zcash_primitives` crate as of the tag
`zcash_primitives-0.14.0`.

### Added
- The following modules have been extracted from `zcash_primitives` and
  moved to this crate:
  - `consensus`
  - `constants`
  - `zcash_protocol::value` replaces `zcash_primitives::transaction::components::amount`
- `zcash_protocol::consensus`:
  - `NetworkConstants` has been extracted from the `Parameters` trait. Relative to the
    state prior to the extraction:
    - The Bech32 prefixes now return `&'static str` instead of `&str`.
    - Added `NetworkConstants::hrp_tex_address`.
  - `NetworkType`
  - `Parameters::b58_sprout_address_prefix`
- `zcash_protocol::consensus`:
  - `impl Hash for LocalNetwork`
- `zcash_protocol::constants::{mainnet, testnet}::B58_SPROUT_ADDRESS_PREFIX`
- Added in `zcash_protocol::value`:
  - `Zatoshis`
  - `ZatBalance`
  - `MAX_BALANCE` has been added to replace previous instances where
    `zcash_protocol::value::MAX_MONEY` was used as a signed value.

### Changed
- `zcash_protocol::value::COIN` has been changed from an `i64` to a `u64`
- `zcash_protocol::value::MAX_MONEY` has been changed from an `i64` to a `u64`
- `zcash_protocol::consensus::Parameters` has been split into two traits, with
  the newly added `NetworkConstants` trait providing all network constant
  accessors. Also, the `address_network` method has been replaced with a new
  `network_type` method that serves the same purpose. A blanket impl of 
  `NetworkConstants` is provided for all types that implement `Parameters`,
  so call sites for methods that have moved to `NetworkConstants` should 
  remain unchanged (though they may require an additional `use` statement.)

### Removed
- From `zcash_protocol::value`:
  - `NonNegativeAmount` (use `Zatoshis` instead.)
  - `Amount` (use `ZatBalance` instead.)
  - The following conversions have been removed relative to `zcash_primitives-0.14.0`,
    as `zcash_protocol` does not depend on the `orchard` or `sapling-crypto` crates.
    - `From<NonNegativeAmount> for orchard::NoteValue>`
    - `TryFrom<orchard::ValueSum> for Amount`
    - `From<NonNegativeAmount> for sapling::value::NoteValue>`
    - `TryFrom<sapling::value::NoteValue> for NonNegativeAmount`
  - `impl AddAssign for NonNegativeAmount`
  - `impl SubAssign for NonNegativeAmount`
