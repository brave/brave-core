# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.5.3 (2024-01-20)
### Fixed
- Documentation ([#458], [#459])
- Big endian support  ([#482])

[#458]: https://github.com/RustCrypto/password-hashes/pull/458
[#459]: https://github.com/RustCrypto/password-hashes/pull/459
[#482]: https://github.com/RustCrypto/password-hashes/pull/482

## 0.5.2 (2023-09-03)
### Changed
- Improved `const fn` support ([#450])

### Fixed
- Max params ([#452])

[#450]: https://github.com/RustCrypto/password-hashes/pull/450
[#452]: https://github.com/RustCrypto/password-hashes/pull/452

## 0.5.1 (2023-07-13)
### Added
- Provide `std::error::Error::source` for `argon2::Error` ([#379])
- `ParamsBuilder::context` ([#400])
- Enable `password-hash/alloc` when `alloc` feature is enabled ([#422])
- Impl `Debug` for `Argon2` ([#423])
- `Block::new()` const initializer ([#427])
- `Params::DEFAULT` constant ([#439])
- Initial AVX2 SIMD optimizations ([#440])

[#379]: https://github.com/RustCrypto/password-hashes/pull/379
[#400]: https://github.com/RustCrypto/password-hashes/pull/400
[#422]: https://github.com/RustCrypto/password-hashes/pull/422
[#423]: https://github.com/RustCrypto/password-hashes/pull/423
[#427]: https://github.com/RustCrypto/password-hashes/pull/427
[#439]: https://github.com/RustCrypto/password-hashes/pull/439
[#440]: https://github.com/RustCrypto/password-hashes/pull/440

## 0.5.0 (2023-03-04)
### Added
- Key derivation usage example ([#366])
- Inherent constants for `Params` recommendations ([#387])

### Changed
- Merge `Argon2` and `Instance` structs ([#247])
- Refactor `ParamsBuilder` to make it more ergonomic ([#247])
- Bump `password-hash` dependency to v0.5 ([#383])
- Adopt OWASP recommended default `Params` ([#386])
- MSRV 1.65 ([#391])

### Fixed
- Erroneous docs for `m_cost` and `Block` ([#247])
- Allow `zeroize` in heapless environments (i.e. without `alloc`) ([#374])

### Removed
- `Memory` struct ([#247])
- Unsound `parallel` feature - see [#380] ([#247])

[#247]: https://github.com/RustCrypto/password-hashes/pull/247
[#366]: https://github.com/RustCrypto/password-hashes/pull/366
[#374]: https://github.com/RustCrypto/password-hashes/pull/374
[#380]: https://github.com/RustCrypto/password-hashes/pull/380
[#383]: https://github.com/RustCrypto/password-hashes/pull/383
[#386]: https://github.com/RustCrypto/password-hashes/pull/386
[#387]: https://github.com/RustCrypto/password-hashes/pull/387
[#391]: https://github.com/RustCrypto/password-hashes/pull/391

## 0.4.1 (2022-06-27)
### Added
- `argon2::RECOMMENDED_SALT_LEN` ([#307])

[#307]: https://github.com/RustCrypto/password-hashes/pull/307

## 0.4.0 (2022-03-18)
### Changed
- Bump `password-hash` dependency to v0.4; MSRV 1.57 ([#283])
- 2021 edition upgrade ([#284])

[#283]: https://github.com/RustCrypto/password-hashes/pull/283
[#284]: https://github.com/RustCrypto/password-hashes/pull/284

## 0.3.4 (2022-02-17)
### Fixed
- Minimal versions build ([#273])

[#273]: https://github.com/RustCrypto/password-hashes/pull/273

## 0.3.3 (2022-01-30)
### Changed
- Make `Params::block_count()` public ([#258])

[#258]: https://github.com/RustCrypto/password-hashes/pull/258

## 0.3.2 (2021-12-07)
### Changed
- Bump `blake2` dependency to v0.10 ([#254])

[#254]: https://github.com/RustCrypto/password-hashes/pull/254

## 0.3.1 (2021-09-11)
### Fixed
- Handling of `p_cost` parameter ([#235])

[#235]: https://github.com/RustCrypto/password-hashes/pull/235

## 0.3.0 (2021-08-27) [YANKED]
### Added
- `alloc` feature ([#215])
- `Params` now supports `keyid` and `data` fields ([#216])

### Changed
- `Argon2::new` now has explicit `version` and `params` ([#213])
- Factored apart `Argon2::new` and `Argon2::new_with_secret`, making the
  former infallible ([#213])
- `Params` struct is now opaque with field accessors, and ensured to
  always represent a valid set of parameters ([#213])
- Removed `version` parameter from `hash_password_into`, using the one
  supplied to `Argon2::new` ([#213])
- Bump `password-hash` to v0.3 ([#217], [RustCrypto/traits#724])
- Use `resolver = "2"`; MSRV 1.51+ ([#220])

### Removed
- `Params` no longer has a `version` field ([#211])

[#211]: https://github.com/RustCrypto/password-hashes/pull/211
[#213]: https://github.com/RustCrypto/password-hashes/pull/213
[#215]: https://github.com/RustCrypto/password-hashes/pull/215
[#216]: https://github.com/RustCrypto/password-hashes/pull/216
[#217]: https://github.com/RustCrypto/password-hashes/pull/217
[#220]: https://github.com/RustCrypto/password-hashes/pull/220
[RustCrypto/traits#724]: https://github.com/RustCrypto/traits/pull/724

## 0.2.4 (2021-08-21)
### Added
- Impl `std::error::Error` for `argon2::Error` ([#200])
- Impl `TryFrom<Params>` for `Argon2` ([#202])
- `Result` type alias ([#203])
- `ParamsBuilder` ([#204])

[#200]: https://github.com/RustCrypto/password-hashes/pull/200
[#202]: https://github.com/RustCrypto/password-hashes/pull/202
[#203]: https://github.com/RustCrypto/password-hashes/pull/203
[#204]: https://github.com/RustCrypto/password-hashes/pull/204

## 0.2.3 (2021-08-15)
### Changed
- Relax `zeroize` requirements to `>=1, <1.4` ([#195])

[#195]: https://github.com/RustCrypto/password-hashes/pull/195

## 0.2.2 (2021-07-20)
### Changed
- Pin `zeroize` dependency to v1.3 ([#190])

[#190]: https://github.com/RustCrypto/password-hashes/pull/190

## 0.2.1 (2021-05-28)
### Changed
- `Params` always available; no longer feature-gated on `password-hash` ([#182])

### Fixed
- Configured params are used with `hash_password_simple` ([#182])

[#182]: https://github.com/RustCrypto/password-hashes/pull/182

## 0.2.0 (2021-04-29)
### Changed
- Forbid unsafe code outside parallel implementation ([#157])
- Bump `password-hash` crate dependency to v0.2 ([#164])

### Removed
- `argon2::BLOCK_SIZE` constant ([#161])

[#157]: https://github.com/RustCrypto/password-hashes/pull/157
[#161]: https://github.com/RustCrypto/password-hashes/pull/161
[#164]: https://github.com/RustCrypto/password-hashes/pull/164

## 0.1.5 (2021-04-18)
### Added
- Parallel lane processing using `rayon` ([#149])

[#149]: https://github.com/RustCrypto/password-hashes/pull/149

## 0.1.4 (2021-02-28)
### Added
- `std` feature ([#141])

[#141]: https://github.com/RustCrypto/password-hashes/pull/141

## 0.1.3 (2021-02-12)
### Fixed
- Salt-length related panic ([#135])

[#135]: https://github.com/RustCrypto/password-hashes/pull/135

## 0.1.2 (2021-02-07)
### Fixed
- rustdoc typo ([#128])

[#128]: https://github.com/RustCrypto/password-hashes/pull/128

## 0.1.1 (2021-02-07)
### Added
- `rand` feature; enabled-by-default ([#126])

[#126]: https://github.com/RustCrypto/password-hashes/pull/126

## 0.1.0 (2021-01-29)
- Initial release
