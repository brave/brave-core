# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.1.5 (2024-01-12)
### Changed
- Enable ARMv8 ASM backend for `p1600` ([#68])

[#68]: https://github.com/RustCrypto/sponges/pull/68

## 0.1.4 (2023-05-04)
### Added
- `keccak_p` fns for `[200, 400, 800, 1600]` ([#55])

### Changed
- 2018 edition upgrade ([#32])

[#32]: https://github.com/RustCrypto/sponges/pull/32
[#55]: https://github.com/RustCrypto/sponges/pull/55

## 0.1.3 (2022-11-14)
### Added
- ARMv8 SHA3 ASM intrinsics implementation for `keccak_f1600` ([#23])

[#23]: https://github.com/RustCrypto/sponges/pull/23

## 0.1.2 (2022-05-24)
### Changed
- Implement `simd` feature with  `portable_simd` instead of deprecated `packed_simd` ([#16])

[#16]: https://github.com/RustCrypto/sponges/pull/16

## 0.1.1 (2022-05-24)
### Added
- Generic keccak-p and keccak-f {200, 400, 800} ([#7])
- f1600x{2, 4, 8} ([#8])

[#7]: https://github.com/RustCrypto/sponges/pull/7
[#8]: https://github.com/RustCrypto/sponges/pull/8

## 0.1.0 (2018-03-27)
- Initial release
