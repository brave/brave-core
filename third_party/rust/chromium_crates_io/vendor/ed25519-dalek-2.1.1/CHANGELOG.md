# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Entries are listed in reverse chronological order per undeprecated major series.

# Unreleased

# 2.x series

## 2.1.1

* Fix nightly SIMD build

## 2.1.0

* Add `SigningKey::to_scalar_bytes` for getting the unclamped scalar from a signing key
* Loosened `signature` dependency to allow version 2.2

##  2.0.0

### Breaking changes

* Bump MSRV from 1.41 to 1.60.0
* Bump Rust edition
* Bump `signature` dependency to 2.0
* Make `digest` an optional dependency
* Make `zeroize` an optional dependency
* Make `rand_core` an optional dependency
* [curve25519 backends] are now automatically selected
* [curve25519 backends] are now overridable via cfg instead of using additive features
* Make all batch verification deterministic remove `batch_deterministic` (PR [#256](https://github.com/dalek-cryptography/ed25519-dalek/pull/256))
* Rename `Keypair` → `SigningKey` and `PublicKey` → `VerifyingKey`
* Remove default-public `ExpandedSecretKey` API (PR [#205](https://github.com/dalek-cryptography/ed25519-dalek/pull/205))
* Make `hazmat` feature to expose `ExpandedSecretKey`, `raw_sign()`, `raw_sign_prehashed()`, `raw_verify()`, and `raw_verify_prehashed()`

[curve25519 backends]: https://github.com/dalek-cryptography/curve25519-dalek/#backends

### Other changes

* Add `Context` type for prehashed signing
* Add `VerifyingKey::{verify_prehash_strict, is_weak}`
* Add `pkcs` feature to support PKCS #8 (de)serialization of `SigningKey` and `VerifyingKey`
* Add `fast` feature to include basepoint tables
* Add tests for validation criteria
* Impl `DigestSigner`/`DigestVerifier` for `SigningKey`/`VerifyingKey`, respectively
* Impl `Hash` for `VerifyingKey`
* Impl `Clone`, `Drop`, and `ZeroizeOnDrop` for `SigningKey`
* Remove `rand` dependency
* Improve key deserialization diagnostics
