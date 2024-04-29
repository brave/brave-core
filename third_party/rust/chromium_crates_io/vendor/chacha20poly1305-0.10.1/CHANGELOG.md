# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.10.1 (2022-08-09)
### Added
- `rand_core` feature ([#467])

[#467]: https://github.com/RustCrypto/AEADs/pull/467

## 0.10.0 (2022-07-31)
### Added
- `getrandom` feature ([#446])
- Impl `ZeroizeOnDrop` for `ChaChaPoly1305` ([#447])

### Changed
- Bump `chacha20` dependency to v0.9 ([#402])
- Rust 2021 edition upgrade; MSRV 1.56+ ([#435])
- Bump `aead` dependency to v0.5 ([#444])
- Bump `poly1305` dependency to v0.8 ([#454])

[#402]: https://github.com/RustCrypto/AEADs/pull/402
[#435]: https://github.com/RustCrypto/AEADs/pull/435
[#444]: https://github.com/RustCrypto/AEADs/pull/444
[#446]: https://github.com/RustCrypto/AEADs/pull/446
[#447]: https://github.com/RustCrypto/AEADs/pull/447
[#454]: https://github.com/RustCrypto/AEADs/pull/454

## 0.9.1 (2022-07-07)
### Changed
- Unpin `zeroize` dependency ([#438])

[#438]: https://github.com/RustCrypto/AEADs/pull/438

## 0.9.0 (2021-08-29)
### Changed
- Bump `chacha20` to v0.8: now a hard dependency ([#365])
- MSRV 1.51+ ([#365])

### Removed
- `chacha20` feature: now a hard dependency ([#365])
- `xchacha20` feature: now always-on ([#365])
- `chacha20-reduced-round` and `xchacha20-reduced-round` have been coalesced
  into the `reduced-round` feature ([#365])

[#365]: https://github.com/RustCrypto/AEADs/pull/365

## 0.8.2 (2021-08-28)
### Added
- `XChaCha*` reduced-round variants ([#355])

### Changed
- Relax `subtle` and `zeroize` requirements ([#360])

[#355]: https://github.com/RustCrypto/AEADs/pull/355
[#360]: https://github.com/RustCrypto/AEADs/pull/360

## 0.8.1 (2021-07-20)
### Changed
- Pin `zeroize` dependency to v1.3 ([#349])

[#349]: https://github.com/RustCrypto/AEADs/pull/349

## 0.8.0 (2021-04-29)
### Added
- Wycheproof test vectors ([#274])

### Changed
- Bump `aead` crate dependency to v0.4 ([#270])
- `xchacha` feature name ([#257])
- MSRV 1.49+ ([#286], [#289])
- Bump `chacha20` crate dependency to v0.7 ([#286])
- Bump `poly1305` crate dependency to v0.7 ([#289])

[#257]: https://github.com/RustCrypto/AEADs/pull/257
[#270]: https://github.com/RustCrypto/AEADs/pull/270
[#274]: https://github.com/RustCrypto/AEADs/pull/274
[#286]: https://github.com/RustCrypto/AEADs/pull/286
[#289]: https://github.com/RustCrypto/AEADs/pull/289

## 0.7.1 (2020-10-25)
### Changed
- Expand README.md ([#233])

[#233]: https://github.com/RustCrypto/AEADs/pull/233

## 0.7.0 (2020-10-16)
### Changed
- Replace `block-cipher`/`stream-cipher` with `cipher` crate ([#229])
- Bump `chacha20` dependency to v0.6 ([#229])

[#229]: https://github.com/RustCrypto/AEADs/pull/229

## 0.6.0 (2020-09-17)
### Added
- Optional `std` feature; disabled by default ([#217])

### Changed
- Upgrade `chacha20` to v0.5; `stream-cipher` to v0.7 ([#209])

[#217]: https://github.com/RustCrypto/AEADs/pull/217
[#209]: https://github.com/RustCrypto/AEADs/pull/209

## 0.5.1 (2020-06-11)
### Added
- `Key`, `Nonce`, and `XNonce` type aliases ([#168])

[#168]: https://github.com/RustCrypto/AEADs/pull/168

## 0.5.0 (2020-06-06)
### Changed
- Bump `aead` crate dependency to v0.3; MSRV 1.41+ ([#144])
- Bump `chacha20` crate dependency to v0.4 ([#159])
- Bump `poly1305` crate dependency to v0.6 ([#158])

[#159]: https://github.com/RustCrypto/AEADs/pull/159
[#158]: https://github.com/RustCrypto/AEADs/pull/158
[#144]: https://github.com/RustCrypto/AEADs/pull/144

## 0.4.1 (2020-03-09)
### Fixed
- `Clone` impl on `ChaChaPoly1305` ([#103])

[#103]: https://github.com/RustCrypto/AEADs/pull/103

## 0.4.0 (2020-03-07)
### Added
- `chacha20` cargo feature; ; replace macros with generics ([#99])

[#99]: https://github.com/RustCrypto/AEADs/pull/99

## 0.3.3 (2020-02-27)
### Fixed
- Wording in documentation about security audit ([#84])

[#84]: https://github.com/RustCrypto/AEADs/pull/84

## 0.3.2 (2020-02-26)
### Added
- Notes about NCC audit to documentation ([#80])

[#80]: https://github.com/RustCrypto/AEADs/pull/80

## 0.3.1 (2020-01-16)
### Added
- `ChaCha8Poly1305`/`ChaCha12Poly1305` reduced round variants ([#69])
- `criterion`-based benchmark ([#66])

### Changed
- Upgrade to `chacha20` v0.3; adds AVX2 backend w\ +60% perf ([#67])

[#66]: https://github.com/RustCrypto/AEADs/pull/66
[#67]: https://github.com/RustCrypto/AEADs/pull/67
[#69]: https://github.com/RustCrypto/AEADs/pull/69

## 0.3.0 (2019-11-26)
### Added
- `heapless` feature ([#51])

### Changed
- Upgrade `aead` crate to v0.2; `alloc` now optional ([#43])

[#51]: https://github.com/RustCrypto/AEADs/pull/51
[#43]: https://github.com/RustCrypto/AEADs/pull/43

## 0.2.2 (2019-11-14)
### Changed
- Upgrade to `zeroize` 1.0 ([#36])

[#36]: https://github.com/RustCrypto/AEADs/pull/36

## 0.2.1 (2019-10-15)
### Changed
- Documentation improvements ([#34])

[#34]: https://github.com/RustCrypto/AEADs/pull/34

## 0.2.0 (2019-10-06)
### Added
- Expose "detached" in-place encryption/decryption APIs ([#21])

### Changed
- Upgrade to `poly1305` crate v0.5 ([#20])

[#21]: https://github.com/RustCrypto/AEADs/pull/21
[#20]: https://github.com/RustCrypto/AEADs/pull/20

## 0.1.2 (2019-10-01)
### Changed
- Update to `zeroize` 1.0.0-pre ([#17])

[#17]: https://github.com/RustCrypto/AEADs/pull/17

## 0.1.1 (2019-09-19)
### Changed
- Update to `poly1305` v0.4 ([#8])

[#8]: https://github.com/RustCrypto/AEADs/pull/8

## 0.1.0 (2019-08-30)

- Initial release
