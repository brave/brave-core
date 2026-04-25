## 0.3.0 - 2023-01-04

- Update minimum supported Rust version: 1.40.0 -> 1.50.0.
- Update dependencies (https://github.com/gendx/lzma-rs/pull/78):
  - `byteorder`: ^1.0.0 -> 1.4.3
  - `crc`: ^1.0.0 -> 3.0.0
  - `log`: ^0.4.14 -> 0.4.17
  - `env_logger`: ^0.8.3 -> 0.9.0
- Expose a new `raw_decoder` API (https://github.com/gendx/lzma-rs/pull/74).
- Reduce the number of allocations (https://github.com/gendx/lzma-rs/pull/77).
- Display features on rustdoc (https://github.com/gendx/lzma-rs/pull/70).
- Configure formatting style to `imports_granularity = "Module"`
  (https://github.com/gendx/lzma-rs/pull/82).
- Add code coverage reporting (https://github.com/gendx/lzma-rs/pull/86).

## 0.2.0 - 2021-05-02

- Update minimum supported Rust version: 1.32.0 -> 1.40.0.
- Update dependencies:
  - `log`: ^0.4.8 -> ^0.4.14
  - `env_logger`: 0.7.1 -> ^0.8.3
- [Breaking change] Rename acronyms to be lowercase, following
  clippy::upper-case-acronyms.
- [Breaking change] Add a memory limit option
  (https://github.com/gendx/lzma-rs/pull/50).
- Fix bug in LZMA2 decompression (https://github.com/gendx/lzma-rs/pull/61).
- Fix bug in CRC32 validation (https://github.com/gendx/lzma-rs/pull/56).
- Add a streaming mode for LZMA decompression, gated by the `stream` feature.
- Add more fuzzing targets, including comparison with the `xz2` crate.
- Various improvements: benchmarks, fix lint warnings.
- Migrate from Travis-CI to GitHub Actions.

## 0.1.4 - 2021-05-02

- Backports from 0.2.0:
  - Fix bug in LZMA2 decompression (https://github.com/gendx/lzma-rs/pull/61).
  - Fix bug in CRC32 validation (https://github.com/gendx/lzma-rs/pull/56).

## 0.1.3 - 2020-05-05

- Minimum supported Rust version: 1.32.0.
- Update dependencies:
  - `log`: ^0.4.0 -> ^0.4.8
  - `env_logger`: 0.6.0 -> ^0.7.1
- Gate logging behind an opt-in feature. This improves decoding performance by
  ~25% (https://github.com/gendx/lzma-rs/pull/31).
- Lazily allocate the circular buffer (https://github.com/gendx/lzma-rs/pull/22).
  This improves memory usage (especially for WebAssembly targets) at the expense
  of a ~5%  performance regression (https://github.com/gendx/lzma-rs/issues/27).
- Return an error instead of panicking on unsupported SHA-256 checksum for XZ
  decoding (https://github.com/gendx/lzma-rs/pull/40).
- Add Clippy to CI.
- Document public APIs.
- Deny missing docs, missing Debug implementations and build warnings.
- Forbid unsafe code.
- Remove extern statements that are unnecessary on the 2018 edition.

## 0.1.2 - 2019-12-17

- Fix bug in the range coder (https://github.com/gendx/lzma-rs/issues/15).
- Add support for specifying the unpacked size outside of the header
  (https://github.com/gendx/lzma-rs/pull/17).
- Migrate to Rust 2018 edition.
- Add benchmarks.
- Fix some Clippy warnings.

## 0.1.1 - 2019-02-24

- Upgrade `env_logger` dependency.
- Refactoring to use `std::io::Take`, operator `?`.

## 0.1.0 - 2018-01-07

- Initial release.
