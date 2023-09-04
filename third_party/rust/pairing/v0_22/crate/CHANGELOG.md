# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.22.0] - 2021-05-04
### Changed
- MSRV bumped to `1.56.0`
- Bumped dependencies to `group 0.12`.
- Removed unused dev dependencies.

## [0.21.0] - 2021-09-02
### Added
- `Debug + Send + Sync` bounds on `pairing::Engine`.
- Various bounds on `pairing::MillerLoopResult`:
  - `Copy + Default + Debug`
  - `Send + Sync`
  - `Add<Output = Self> + AddAssign`
  - `for<'a> Add<&'a Self, Output = Self>`
  - `for<'a> AddAssign<&'a Self>`

## [0.20.0] - 2021-06-01
### Added
- `pairing::group`, which re-exports the `group` crate to make version-matching
  easier. `ff` is transitively re-exported as `pairing::group::ff`.

### Changed
- Bumped `group` to 0.10.
- MSRV is now 1.51.0.

## [0.19.0] - 2021-01-26
### Changed
- Bumped dependencies to `ff 0.9` and `group 0.9`.
- MSRV is now 1.47.0.

## [0.18.0] - 2020-09-08
### Added
- `no-std` support.

### Changed
- Bumped dependencies to `ff 0.8` and `group 0.8`.
- MSRV is now 1.44.0.

### Removed
- Obsolete feature flags `expose-arith` and `unstable-features`.
