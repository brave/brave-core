# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 3.0.1 (2022-07-20)
### Added
- New tier 3 targets ([#614])

## 3.0.0 (2022-04-23)
### Added
- `target_endian` and `target_pointer_width` fields on `Platform` ([#516])

### Changed
- Auto-generate platforms registry from rustc ([#516])
- Make `Platform` struct `#[non_exhaustive]` to allow new fields ([#516])
- Make `Platform::env` field non-optional ([#516])

### Fixed
- `serde` deserializers ([#527])

[#516]: https://github.com/rustsec/rustsec/pull/516
[#527]: https://github.com/rustsec/rustsec/pull/527

## 2.0.0 (2021-11-15)
### Added
- New tier 3 targets ([#357])

### Changed
- Sync with Rust platform support documentation ([#353])
- Follow `upper_case_acronyms` conventions ([#473])
- Make tier modules non-`pub` ([#483])
- Make `Platform::ALL` an inherent constant ([#484])

[#353]: https://github.com/rustsec/rustsec/pull/353
[#357]: https://github.com/rustsec/rustsec/pull/357
[#473]: https://github.com/rustsec/rustsec/pull/473
[#483]: https://github.com/rustsec/rustsec/pull/483
[#484]: https://github.com/rustsec/rustsec/pull/484

## 1.1.0 (2020-12-28)
### Added
- `aarch64-apple-darwin` platform definition

## 1.0.3 (2020-10-29)
### Changed
- Source `Platform::guess_current` from `$TARGET` environment variable when
  available

## 1.0.2 (2020-09-14)
### Removed
- `const fn` on `Platforms::all`

## 1.0.1 (2020-09-14) [YANKED]
### Changed
- Make `Platform::all()` a `const fn`
- Refactor `Platform::find` and `::guess_current`
- Rename `ALL_PLATFORMS` to `Platform::all()`

## 1.0.0 (2020-09-13) [YANKED]
### Added
- Ensure all types have `FromStr`, `Display`, and `serde` impls
- `aarch64-pc-windows-msvc` platform

### Changed
- Make extensible enums `non_exhaustive`; MSRV 1.40+

## 0.2.1 (2019-09-24)

- Initial GitHub Actions config
- Properly set up `target::os::TARGET_OS` const for unknown OS

## 0.2.0 (2019-01-13)

- Update platforms to match RustForge
- Update to Rust 2018 edition

## 0.1.4 (2018-07-29)

- `x86_64-apple-darwin`: fix typo in target triple name
- Have markdown-table-gen output links to Platform structs on docs.rs

## 0.1.3 (2018-07-28)

- Fix Travis CI badge in Cargo.toml

## 0.1.2 (2018-07-27)

- Add table of supported platforms to README.md using Markdown generator

## 0.1.1 (2018-07-27)

- Impl `Display` and `std::error::Error` traits for `packages::Error`

## 0.1.0 (2018-07-26)

- Add `guess_current()`
- Optional serde support

## 0.0.1 (2018-07-26)

- Initial release
