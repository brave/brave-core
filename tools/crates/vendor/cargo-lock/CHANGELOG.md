# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 10.1.0 (2025-01-18)
### Fixed

 - Fixed git tags in Cargo.lock not being normalized correctly when using lockfile v4 format ([#1298])
 - Increased MSRV to 1.73 to match the requirements of dependencies in Cargo.lock

[#1298]: https://github.com/RustSec/rustsec/pull/1298

## 10.0.1 (2024-10-25)
### Fixed
- Remove `precise` from source IDs during normalization ([#1270])

[#1270]: https://github.com/RustSec/rustsec/pull/1270

## 10.0.0 (2024-10-15)
### Added
- V4 lockfile support ([#1206])

### Changed
- MSRV 1.70 ([#1092])

### Removed
- `toml` dependency from public API ([#1226])

[#1092]: https://github.com/RustSec/rustsec/pull/1092
[#1206]: https://github.com/RustSec/rustsec/pull/1206
[#1226]: https://github.com/RustSec/rustsec/pull/1226

## 9.0.0 (2023-04-24)
### Added
- Implement `From<Name>` for `String` ([#776])
- Support sparse registry references in `Lockfile`s ([#780])

### Changed
- Mark `SourceKind` as `#[non_exhaustive]` ([#793])
- Use `Display` for `io::ErrorKind`; MSRV 1.60 ([#794])
- Bump `toml` to 0.7 ([#800], [#805])
- Improvements to the `cargo lock tree` subcommand ([#860])

### Fixed
- `Source::is_default_registry` for sparse index ([#859])

[#776]: https://github.com/RustSec/rustsec/pull/776
[#780]: https://github.com/RustSec/rustsec/pull/780
[#793]: https://github.com/RustSec/rustsec/pull/793
[#794]: https://github.com/RustSec/rustsec/pull/794
[#800]: https://github.com/RustSec/rustsec/pull/800
[#805]: https://github.com/RustSec/rustsec/pull/805
[#859]: https://github.com/RustSec/rustsec/pull/859
[#860]: https://github.com/RustSec/rustsec/pull/860

## 8.0.3 (2022-11-30)
### Fixed
- Encoding inconsistency when there's only one registry for all packages ([#767])

[#767]: https://github.com/RustSec/rustsec/pull/767

## 8.0.2 (2022-06-30)
### Fixed
- Re-export `GitReference` ([#595])
- Encode version into V3 lockfiles ([#596])

[#595]: https://github.com/RustSec/rustsec/pull/595
[#596]: https://github.com/RustSec/rustsec/pull/596

## 8.0.1 (2022-05-21)
### Fixed
- Dependency source extraction for V2+ lockfiles ([#568])

[#568]: https://github.com/RustSec/rustsec/pull/568

## 8.0.0 (2022-05-08) [YANKED]
NOTE: yanked due to bug fixed in v8.0.1.

### Added
- Expose `package::SourceKind` ([#557])

### Changed
- Flatten API ([#558])
- 2021 edition upgrade; MSRV 1.56 ([#559])
- Refactor error handling ([#560])

[#557]: https://github.com/RustSec/rustsec/pull/557
[#558]: https://github.com/RustSec/rustsec/pull/558
[#559]: https://github.com/RustSec/rustsec/pull/559
[#560]: https://github.com/RustSec/rustsec/pull/560

## 7.1.0 (2022-04-23)
### Added
- `SourceId::default()` ([#536])

### Changed
- MSRV is now 1.49 ([#524])

### Fixed
- V3 lockfile handling and tests ([#535])

[#524]: https://github.com/RustSec/rustsec/pull/524
[#535]: https://github.com/RustSec/rustsec/pull/535
[#536]: https://github.com/RustSec/rustsec/pull/536

## 7.0.1 (2021-07-05)
### Changed
- Bump `petgraph` dependency from 0.5.1 to 0.6.0 ([#396])

[#396]: https://github.com/RustSec/rustsec/pull/396

## 7.0.0 (2021-05-27) [YANKED]
### Added
- Support for V3 lockfile format ([#363])

### Changed
- Bump `semver` to v1.0.0 ([#378])

[#363]: https://github.com/RustSec/rustsec/pull/363
[#378]: https://github.com/RustSec/rustsec/pull/378

## 6.0.1 (2021-01-25)
### Changed
-  Rename default branch to `main`

## 6.0.0 (2020-09-25)
- Bump semver from 0.10.0 to 0.11.0

## 5.0.0 (2020-09-23)
- CLI: support for listing a single dependency
- Cargo-compatible serializer
- CLI: add `--dependencies` and `--sources` flags to `cargo lock list`
- CLI: implement `cargo lock tree` without arguments
- Add `dependency::Tree::roots()` method
- CLI: make `list` the default command
- Make `cli` feature non-default
- WASM support; MSRV 1.41+
- Bump `semver` dependency from v0.9 to v0.10

## 4.0.1 (2020-01-22)
- CLI: fix executable name

## 4.0.0 (2020-01-22)
- Command line interface
- Add helper methods for working with checksum metadata
- Use minified version of Cargo's `SourceId` type
- Overhaul encoding: use serde_derive, proper V1/V2 support
- Add support Cargo.lock `patch` and `root`
- Detect V1 vs V2 Cargo.lock files
- Update `petgraph` requirement from 0.4 to 0.5
- Add `package::Checksum`

## 3.0.0 (2019-10-01)
- Support `[package.dependencies]` without versions

## 2.0.0 (2019-09-25)
- Use two-pass dependency tree computation
- Remove `Lockfile::root_package()`

## 1.0.0 (2019-09-24)
- dependency/tree: Render trees to an `io::Write`
- metadata: Generalize into `Key` and `Value` types
- Refactor dependency handling

## 0.2.1 (2019-09-21)
- Allow empty `[metadata]` in Cargo.lock files

## 0.2.0 (2019-09-21)
- dependency_graph: Move `petgraph` types into a module

## 0.1.0 (2019-09-21)
- Initial release
