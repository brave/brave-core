# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 2.1.0 (2025-06-06)
### Added
- CVSS 4.0 support ([#1285])

[#1285]: https://github.com/rustsec/rustsec/pull/1285

## 2.0.0 (2022-05-01)
### Added
- `no_std` support ([#549])

### Changed
- Structured `Error` type ([#546])
- Upgrade to 2021 edition; MSRV 1.56 ([#547])
- Flatten module structure ([#548])

### Fixed
- Computation for "no impact" vectors ([#399])

[#399]: https://github.com/RustSec/rustsec/pull/399
[#546]: https://github.com/RustSec/rustsec/pull/546
[#547]: https://github.com/RustSec/rustsec/pull/547
[#548]: https://github.com/RustSec/rustsec/pull/548
[#549]: https://github.com/RustSec/rustsec/pull/549

## 1.0.2 (2021-05-10)
### Fixed
- Dangling link in rustdoc ([#360])

[#360]: https://github.com/RustSec/rustsec/pull/360

## 1.0.1 (2021-01-25)
### Changed
- Rename default branch to `main`

## 1.0.0 (2019-09-23)
- Migrate to GitHub Actions

## 0.3.0 (2019-09-06)
- CVSS v3.0 support
- severity: Add `FromStr` and `serde` support

## 0.2.0 (2019-08-28)
- Add `Base::exploitability` and `impact` methods; docs
- `serde` support

## 0.1.0 (2019-08-27)
- Initial release
