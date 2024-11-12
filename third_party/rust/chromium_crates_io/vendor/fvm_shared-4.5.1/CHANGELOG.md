# Changelog

## [Unreleased]

## 4.5.1 [2024-11-08]

Remove unnecessary features from `multihash-codetable`.

## 4.5.0 [2024-10-31]

- Update `cid` to v0.11 and `multihash` to v0.19.
- Update to `fvm_ipld_blockstore` 0.3.0 and `fvm_ipld_encoding` 0.5.0.

You will have to update your multihash and cid crates to be compatible, see the [multihash release notes](https://github.com/multiformats/rust-multihash/blob/master/CHANGELOG.md#-2023-06-06) for details on the breaking changes.

## 4.4.3 [2024-10-21]

- Update wasmtime to 25.0.2.
- Fixes long wasm compile times with wasmtime 24.

## 4.4.2 [2024-10-09]

- Update wasmtime to 24.0.1.

## 4.4.1 [2024-10-04]

- chore: remove the `nv24-dev` feature flag [#2051](https://github.com/filecoin-project/ref-fvm/pull/2051)

## 4.4.0 [2024-09-12]

- Update to wasmtime 24.
- Switch from mach ports to unix signal handlers on macos.
- Update misc dependencies.

## 4.3.2 [2024-08-16]

- feat: add `nv24-dev` feature flag [#2029](https://github.com/filecoin-project/ref-fvm/pull/2029)

## 4.3.1 [2024-06-26]

- chore: remove the `nv23-dev` feature flag [#2022](https://github.com/filecoin-project/ref-fvm/pull/2022)

## 4.3.0 [2024-06-12]

- feat: FIP-0079: syscall for aggregated bls verification [#2003](https://github.com/filecoin-project/ref-fvm/pull/2003)
- fix: install rust nightly toolchain for clusterfuzzlite [#2007](https://github.com/filecoin-project/ref-fvm/pull/2007)
- chore: upgrade rust toolchain to 1.78.0 [#2006](https://github.com/filecoin-project/ref-fvm/pull/2006)
- fix: remove the pairing feature from fvm_shared [#2009](https://github.com/filecoin-project/ref-fvm/pull/2009)
- Small tidy-ups in CONTRIBUTING.md [#2012](https://github.com/filecoin-project/ref-fvm/pull/2012)
- NI-PoRep support [#2010](https://github.com/filecoin-project/ref-fvm/pull/2010)

## 4.2.0 [2024-04-29]

- chore: update to wasmtime 19.0.1 [#1993](https://github.com/filecoin-project/ref-fvm/pull/1993)
- Enable nv23 support behind the `nv23-dev` feature flag [#2000](https://github.com/filecoin-project/ref-fvm/pull/2000)
- feat: fvm: remove once_cell [#1989](https://github.com/filecoin-project/ref-fvm/pull/1989)
- feat: shared: check bls zero address without lazy_static [#1984](https://github.com/filecoin-project/ref-fvm/pull/1984)

## 4.1.2 [2024-01-31]

feat: allow CBOR events

## 4.1.1 [2024-01-25]

Enable nv22 support by default.

## 4.1.0 [2024-01-24]

- Pretty-print addresses when debug-formatting, instead of printing the raw bytes as a vector.
- Move the `ActorState` struct to this crate (from the `fvm` crate).
- Add an `upgrade` module to this crate to support the new (disabled by default) actor-upgrade syscall.

## 4.0.0 [2023-10-31]

Final release, no changes.

## 4.0.0-alpha.4 [2023-09-28]

- Add back some proof types that were mistakenly removed, and fix some of the constants.

## 4.0.0-alpha.3 [2023-09-27]

- Remove support for v1 proofs.

## 4.0.0-alpha.2 [2023-09-21]

- Implement FIP-0071, FIP-0072, FIP-0073, FIP-0075

## 4.0.0-alpha.1 [2023-09-20]

Unreleased. This release simply marks the change-over to v4.

## 3.6.0 [2023-09-06]

- BREAKING: Upgrade the proofs API to v16.
- BREAKING (linking): upgrade blstrs to v0.7 and
- BREAKING: update the minimum rust version to 1.70.0
- Update & trim some dependencies.
- Add support for the new proofs in v16.

## 3.5.0 [2023-08-18]

- Add the V21 network version constant

## 3.4.0 [2023-06-27]

Breaking Changes:

- Update cid/multihash. This is a breaking change as it affects the API.

## 3.3.1 [2023-05-04]

Fix some address constants (lazy statics, to be precise) when the current network is set to "testnet". Previously, if said constants were evaluated _after_ switching to testnet mode (calling `address::set_current_network`), they'd fail to parse and crash the program when dereferenced.

## 3.3.0 [2023-04-23]

- Fixes an issue with proof bindings.

## 3.2.0 [2023-04-04]

- Remove unused dependencies.
- Remove unused dependencies.
- BREAKING: Drop unused `registered_seal_proof` method. This appears to have been unused by anyone.

## 3.1.0 [2023-03-09]

Update proofs. Unfortunately, this is a breaking change in a minor release but we need to do the same on the v2 release as well. The correct solution is to introduce two crates, fvm1 and fvm2, but that's a future project.

## 3.0.0 [2022-02-24]

- Final release for NV18.

## 3.0.0-alpha.20 [2022-02-06]

- Change the `BLOCK_GAS_LIMIT` constant to a `u64` to match all the other gas values.

## 3.0.0-alpha.19 [2022-02-06]

- Change the event datastructure to take a codec and not double-encode the value.
- Make the message version and gas limits `u64`s instead of `i64`s.

## 3.0.0-alpha.18 [2022-02-01]

- Improve rustdocs around events and gas premium.

## 3.0.0-alpha.17 [2022-01-17]

- Add `hyperspace` feature to loosen up network version restrictions.

## 3.0.0-alpha.16 [2023-01-12]

- Remove uses of the Cbor trait
- Refactor: Move Response from SDK to shared

## 3.0.0-alpha.15 [2022-12-14]

- Refactor: ChainID was moved from FVM to shared
- Implement Ethereum Account abstraction
  - Removes the f4-as-accont feature, and support for Delegated signature validations

## 3.0.0-alpha.14 [2022-12-07]

- Remove GasLimit from the message context.
- Add the message nonce to the message context
- Add the chain ID to the network context.

## 3.0.0-alpha.13 [2022-11-29]

- Remove deprecated SYS_INVALID_METHOD exit code
- Add a read-only mode to Sends
  - Adds ContextFlags to MessageContext, and a special ReadOnly error 

## 3.0.0-alpha.12 [2022-11-17]

- Refactor network/message contexts to reduce the number of syscalls.

## 3.0.0-alpha.11 [2022-11-15]

- Add support for actor events (FIP-0049).

## 3.0.0-alpha.10 [2022-11-14]

- Split `InvokeContext` into two (#1070)
- fix: correctly format negative token amounts (#1065)

## 3.0.0-alpha.9 [2022-11-08]

- Add support for state-tree v5.

## 3.0.0-alpha.8 [2022-10-22]

- fix compile issues with f4-as-account feature.

## 3.0.0-alpha.7 [2022-10-21]

- Temporary workaround: allow validating signatures from embryo f4 addresses

## 3.0.0-alpha.6 [2022-10-20]

- Make the f4 address conform to FIP0048 (use `f` as the separator).
- Implement `TryFrom<Payload>` for `DelegatedAddress` (and make `DelegatedAddress` public).

## 3.0.0-alpha.5 [2022-10-10]

- Bumps `fvm_ipld_encoding` and switches from `cs_serde_bytes` to `fvm_ipld_encoding::strict_bytes`.

## 3.0.0-alpha.4 [2022-10-10]

- Small f4 address fixes.

## 3.0.0-alpha.3 [2022-10-10]

- Switch to rust 2021 edition.
- Add network version 18.
- BREAKING: Allow changing the address "network" at runtime.
- BREAKING: Update the f4 address format and include a checksum.
- BREAKING: Add the gas premium and gas limit to the `vm::context` return type.

## 3.0.0-alpha.2 [2022-09-16]

- Add basic f4 address support (without checksums for now).
- Change TokenAmount::from_whole to take any `Into<BigInt>` parameter.
- Add nv17 to the network versions.

The only breaking change is the change to `Address`/`Protocol` (in case anyone is exhaustively matching on them).

## 3.0.0-alpha.1 [2022-08-31]

- Bump base version to v3.
- Add `origin` to `vm::Context`.

## 2.0.0...

See the `release/v2` branch.

- Add recover secp public key syscall.
- Removed `actor::builtin::Type` (moved to the actors themselves).
- Add additional hash functions to the hash syscall.
- Add blake2b512
- Change TokenAmount from a type alias to a struct wrapping BigInt

## 0.8.0 [2022-06-13]

- Add a new proofs version type.

## 0.7.1 [2022-05-26]

Add a shared `MAX_CID_LEN` constant.

## 0.7.0 [2022-05-16]

- Updates the blockstore.
- Removes unnecessary chrono dep.
- Removes the `DomainSeparationTag` type. This is moving into the actors themselves as the FVM
  doesn't care about it.
      - Downstream crates should just replicate this type internally, if necessary.
- Adds a new `crypto::signature::verify` function to allow verifying signatures without creating a
  new `Signature` object. This allows verifying _borrowed_ signatures without allocating.
- Updates for the syscall refactor (see `fvm_sdk` v0.7.0):
    - Adds a `BufferTooSmall` `ErrorNumber`.
    - Marks `ErrorNumber` as non-exhaustive for future extension.
    - Changes the syscall "out" types for the syscall refactor.

## 0.6.1 [2022-04-29]

- Added `testing` feature to have `Default` derive on `Message`. Extended this feature to `Address` and `Payload`.
- Improve `ErrorNumber` documentation.
- Update `fvm_ipld_encoding` for the cbor encoder switch.

## 0.6.0 [2022-04-14]

BREAKING: Switch syscall struct alignment: https://github.com/filecoin-project/fvm-specs/issues/63

Actors built against this new version of fvm_shared will be incompatible with prior FVM versions,
and vice-versa.

- Added `Display` trait to `Type` for error printing. 
- Added _cfg = "testing"_ on `Default` trait for `Message` structure.

## 0.5.1  [2022-04-11]

Add the `USR_ASSERTION_FAILED` exit code.

## 0.5.0 [2022-04-11]

- Enforce maximum big-int size to match lotus.
- Make signature properties public.
- Major error type refactor.

The largest change here is a major error type refactor.

1. It's now a u32 with a set of pre-defined values instead of an enum.
2. The error codes have been reworked according to the FVM spec.

Both of these changes were made to better support user-defined actors.
