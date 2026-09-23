# Changelog
All notable changes to this library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this library adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.11.0

### Added
- `std` feature flag; building with `--no-default-features` now enables `no_std` use.
- `NonEmpty::sort` was added.
- `NonEmpty::as_ref` added for converting a `&NonEmpty` to `NonEmpty<&T>`.

### Changed
- MSRV is now 1.56 (this is a semver-breaking change).
- `NonEmpty::split` now returns `Option<&T>` for last element.
- `cargo clippy` and `cargo doc` failures are fixed.

## [Unreleased]

