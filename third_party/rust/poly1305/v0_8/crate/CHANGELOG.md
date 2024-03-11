# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.8.0 (2022-07-31)
### Changed
- Relax `zeroize` constraints ([#147])
- Upgrade to Rust 2021 edition ([#147])
- Use stable `aarch64_target_feature` ([#154])
- Bump `universal-hash` to v0.5 ([#155], [#162])
- Replace `armv8`/`force-soft` features with `cfg` attributes ([#159])

### Removed
- `armv8`/`force-soft` features ([#159])

[#147]: https://github.com/RustCrypto/universal-hashes/pull/147
[#154]: https://github.com/RustCrypto/universal-hashes/pull/154
[#155]: https://github.com/RustCrypto/universal-hashes/pull/155
[#159]: https://github.com/RustCrypto/universal-hashes/pull/159
[#162]: https://github.com/RustCrypto/universal-hashes/pull/162

## 0.7.2 (2021-08-27)
### Changed
- Bump `cpufeatures` dependency to v0.2 ([#136])

[#136]: https://github.com/RustCrypto/universal-hashes/pull/136

## 0.7.1 (2021-07-20)
### Changed
- Pin `zeroize` dependency to v1.3 ([#134])

[#134]: https://github.com/RustCrypto/universal-hashes/pull/134

## 0.7.0 (2021-04-29)
### Changed
- Use `ManuallyDrop` unions; MSRV 1.49+ ([#114])
- Use `cpufeatures` v0.1 crate release ([#116])

[#114]: https://github.com/RustCrypto/universal-hashes/pull/114
[#116]: https://github.com/RustCrypto/universal-hashes/pull/116

## 0.6.2 (2020-12-09)
### Added
- Runtime AVX2 detection ([#97])

[#97]: https://github.com/RustCrypto/universal-hashes/pull/97

## 0.6.1 (2020-09-29)
### Added
- AVX2 backend ([#49])

[#49]: https://github.com/RustCrypto/universal-hashes/pull/49

## 0.6.0 (2020-06-06)
### Added
- `Poly1305::compute_unpadded` for XSalsa20Poly1305 ([#55])

### Changed
- Bump `universal-hash` dependency to v0.4; MSRV 1.41 ([#52], [#57])
- Rename `result` methods to to `finalize` ([#56])

### Fixed
- Build with `zeroize` enabled ([#48])

[#57]: https://github.com/RustCrypto/universal-hashes/pull/57
[#56]: https://github.com/RustCrypto/universal-hashes/pull/56
[#55]: https://github.com/RustCrypto/universal-hashes/pull/55
[#52]: https://github.com/RustCrypto/universal-hashes/pull/52
[#48]: https://github.com/RustCrypto/universal-hashes/pull/48

## 0.5.2 (2019-11-14)
### Changed
- Upgrade to `zeroize` 1.0 ([#33])

[#33]: https://github.com/RustCrypto/universal-hashes/pull/33

## 0.5.1 (2019-10-04)
### Added
- Link to `chacha20poly1305` and `xsalsa20poly1305` crates from README.md ([#26])

[#26]: https://github.com/RustCrypto/universal-hashes/pull/26

## 0.5.0 (2019-10-04)
### Changed
- Upgrade to `universal-hash` crate v0.3 ([#22])

[#22]: https://github.com/RustCrypto/universal-hashes/pull/22

## 0.4.1 (2019-10-01)
### Changed
- Upgrade to `zeroize` v1.0.0-pre ([#19])

[#19]: https://github.com/RustCrypto/universal-hashes/pull/19

## 0.4.0 (2019-09-29)
### Changed
- Update to Rust 2018 edition ([#3])
- Use `UniversalHash` trait ([#5])

[#3]: https://github.com/RustCrypto/universal-hashes/pull/3
[#5]: https://github.com/RustCrypto/universal-hashes/pull/5

## 0.3.0 (2019-08-26)
### Changed
- Switch from `MacResult` to built-in `Tag` type ([RustCrypto/MACs#13])

[RustCrypto/MACs#13]: https://github.com/RustCrypto/MACs/pull/13

## 0.2.0 (2019-08-19)
### Added
- `Poly1305::input_padded()` ([#16])

### Changed
- Change output to be a `MacResult` ([RustCrypto/MACs#16])

[RustCrypto/MACs#16]: https://github.com/RustCrypto/MACs/pull/16

## 0.1.0 (2019-08-15)

- Initial release
