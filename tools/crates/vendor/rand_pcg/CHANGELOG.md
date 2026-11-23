# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.10.3 - 2026-11-15
### Changed
- Add a warning about power-of-two jumps ([#126])

[#126]: https://github.com/rust-random/rngs/pull/126

## 0.10.2 - 2026-04-11
### Changed
- Add methods to get the internal state of the generators (`state` and
  `stream` except for `Mcg128Xsl64`) and constructors to reproducibly
  restore them from that state (#108).

## 0.10.1 - 2026-02-10
### Changed
- The crate is moved from [`rust-random/rand`] to [`rust-random/rngs`].

[`rust-random/rand`]: https://github.com/rust-random/rand
[`rust-random/rngs`]: https://github.com/rust-random/rngs

## [0.10.0] - 2026-02-08
### Changes
- Use Edition 2024 and MSRV 1.85 (#1653)
- Update to `rand_core` v0.10.0 (#1729)
- Remove feature `os_rng` (#1674)
- Use `postcard` instead of `bincode` to test the serde feature (#1693)

## [0.9.0] - 2025-01-27
### Dependencies and features
- Update to `rand_core` v0.9.0 (#1558)
- Rename feature `serde1` to `serde` (#1477)
- Rename feature `getrandom` to `os_rng` (#1537)

### Other changes
- Add `Lcg128CmDxsm64` generator compatible with NumPy's `PCG64DXSM` (#1202)
- Add examples for initializing the RNGs (#1352)
- Revise crate docs (#1454)

## [0.3.1] - 2021-06-15
- Add `advance` methods to RNGs (#1111)
- Document dependencies between streams (#1122)

## [0.3.0] - 2020-12-08
- Bump `rand_core` version to 0.6.0
- Bump MSRV to 1.36 (#1011)
- Derive PartialEq+Eq for Lcg64Xsh32, Lcg128Xsl64, and Mcg128Xsl64 (#979)

## [0.2.1] - 2019-10-22
- Bump `bincode` version to 1.1.4 to fix minimal-dependency builds
- Removed unused `autocfg` build dependency.

## [0.2.0] - 2019-06-12
- Add `Lcg128Xsl64` aka `Pcg64`
- Bump minor crate version since rand_core bump is a breaking change
- Switch to Edition 2018

## [0.1.2] - 2019-02-23
- require `bincode` 1.1.2 for i128 auto-detection
- make `bincode` a dev-dependency again #663
- clean up tests and Serde support

## [0.1.1] - 2018-10-04
- make `bincode` an explicit dependency when using Serde

## [0.1.0] - 2018-10-04
Initial release, including:

- `Lcg64Xsh32` aka `Pcg32`
- `Mcg128Xsl64` aka `Pcg64Mcg`

[0.10.0]: https://github.com/rust-random/rand/compare/0.9.0...0.10.0
[0.9.0]: https://github.com/rust-random/rand/compare/rand_pcg-0.3.1...0.9.0
[0.3.1]: https://github.com/rust-random/rand/compare/rand_pcg-0.3.0...rand_pcg-0.3.1
[0.3.0]: https://github.com/rust-random/rand/compare/rand_pcg-0.2.1...rand_pcg-0.3.0
[0.2.1]: https://github.com/rust-random/rand/compare/rand_pcg-0.2.0...rand_pcg-0.2.1
[0.2.0]: https://github.com/rust-random/rand/compare/rand_pcg-0.1.2...rand_pcg-0.2.0
[0.1.2]: https://github.com/rust-random/rand/compare/6d9e7ac9c6980897d190ede70607f18501d99f3b...rand_pcg-0.1.2
[0.1.1]: https://github.com/rust-random/small-rngs/compare/rand_pcg-0.1.0...rand_pcg-0.1.1
[0.1.0]: https://github.com/rust-random/small-rngs/compare/8ae22ced3f1cfdb888e639f93ca24ef1ea5811c2...rand_pcg-0.1.0
