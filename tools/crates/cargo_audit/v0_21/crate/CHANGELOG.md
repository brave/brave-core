# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.21.0 (2024-10-29)

### Added
- V4 lockfile support ([#1275])

### Changed
- Bump `auditable-info` => 0.8; `auditable-serde` => v0.7 ([#1229])
- Bump `abscissa` to v0.8 ([#1262])
- Bump `rustsec` to v0.30; adds V4 lockfile support ([#1275])

[#1229]: https://github.com/rustsec/rustsec/pull/1229
[#1262]: https://github.com/rustsec/rustsec/pull/1262
[#1275]: https://github.com/rustsec/rustsec/pull/1275

## 0.20.1 (2024-08-16)

TODO

## 0.20.0 (2024-02-16)

### Changed

 - Completely rewritten `cargo audit fix` subcommand ([#1113])
   - Now it edits `Cargo.lock` as opposed to `Cargo.toml`, and performs only semver-compatible upgrades.
   - Fixes are performed by calling `cargo update`, migrating away from the unmaintained `cargo-edit-9` crate.
   - The subcommand is still experimental, and its behavior may change in the future. It still requires enabling the non-default `fix` feature.
 - Require `tame-index` 0.9.3 or later, fixing [issues with some enterprise firewalls](https://github.com/rustsec/rustsec/issues/1058). ([#1103])

[#1103]: https://github.com/rustsec/rustsec/pull/1103
[#1113]: https://github.com/rustsec/rustsec/pull/1113

## 0.19.0 (2024-02-03)

### Fixed

 - Fix `--color=auto` always printing terminal escape sequences ([#1057])

### Changed

 - Display the chain of errors instead of just the top-level error for more complete error reporting ([#1063])
 - Upgrade to clap 4.x and abscissa-core 0.7.x. This may have introduced minor changes to the command-line interface. This is the reason for the minor version bump. ([#1092])

[#1057]: https://github.com/rustsec/rustsec/pull/1057
[#1063]: https://github.com/rustsec/rustsec/pull/1063
[#1092]: https://github.com/rustsec/rustsec/pull/1092

## 0.18.3 (2023-10-24)

### Fixed

 - Fix a deadlock when the `Cargo.lock` file is missing. It only occurs with `rustsec` v0.28.3 or later. ([#1051])

[#1051]: https://github.com/rustsec/rustsec/pull/1051

## 0.18.2 (2023-09-25)

### Fixed

- Fix [RUSTSEC-2023-0064](https://rustsec.org/advisories/RUSTSEC-2023-0064.html) security issue by requiring `rustsec` 0.28.2 or higher.

[#980]: https://github.com/rustsec/rustsec/pull/980

## 0.18.1 (2023-08-31)

### Fixed

- Release workflow: don't enable `fix` and `vendored-openssl` features ([#980])

[#980]: https://github.com/rustsec/rustsec/pull/980

## 0.18.0 (2023-08-31)

### Added

 - Implement proper attribution for advisories licensed under CC-BY ([#955])
 - `cargo audit bin` no longer shows warnings not applicable to the binary type (e.g. no more reports of Windows-only unsoundness in ELF binaries). Previously this was implemented for vulnerabilities, but not warnings. ([#964])

### Changed

 - Upgraded to `rustsec` v0.28, bringing performance, security and compatibility improvements, but also temporarily dropping support for CPU platforms other than x86 and ARM. See the [rustsec changelog](https://github.com/rustsec/rustsec/blob/main/rustsec/CHANGELOG.md) for details.

[#955]: https://github.com/rustsec/rustsec/pull/955
[#964]: https://github.com/rustsec/rustsec/pull/964

## 0.17.6 (2023-05-10)

### Added

 - Upgraded to `cargo-lock` v9.0.0, which enables support for sparse registries.
 - When scanning binary files, the binary's platform is taken into account. This prevents scenarios such as Windows-only vulnerabilities being reported on Linux binaries ([#814])

### Fixed

 - Advisories about `cargo audit` itself are no longer printed multiple times when scanning multiple files ([#848])

[#848]: https://github.com/rustsec/rustsec/pull/848

## 0.17.5 (2023-03-23)

### Added

 - Vulnerability severity is now included in the `cargo audit` output, if known ([#825])

### Changed

 - Advisories marked `informational = unsound` are now reported by default, but only as warnings ([#819]). They do not cause the audit to fail, i.e. the exit code of the process is still 0. This behavior can be suppressed through the configuration file.

### Fixed

 - The help text now correctly refers to the command as `cargo audit` instead of `cargo audit audit` ([#824])
 - The `--version` argument now works correctly, reporting the current version ([#838])

[#819]: https://github.com/rustsec/rustsec/pull/819
[#824]: https://github.com/rustsec/rustsec/pull/824
[#825]: https://github.com/rustsec/rustsec/pull/825
[#838]: https://github.com/rustsec/rustsec/pull/838

## 0.17.4 (2022-11-08)
### Fixed

 - Checks for yanked crates were broken since 0.17.0. This release restores them and adds tests to prevent future regressions.

### Changed
 - Binary scanning is enabled by default and documented as such. It can still be disabled by disabling the `binary-scanning` feature.

## 0.17.3 (2022-11-01)
### Added

 - `cargo audit bin` now attempts to detect dependencies in binaries not built with [`cargo auditable`](https://github.com/rust-secure-code/cargo-auditable) by parsing the panic messages ([#729]). This only detects about a half of the dependency list and never detects C code such as OpenSSL, but works on any Rust binaries built with `cargo`.
 - Added integration tests for the `--deny=warnings` flag.

### Fixed

 - `cargo audit bin --deny=warnings` no longer exits after finding the first binary with warnings.

### Changed

 - Up to 5x faster `cargo audit bin` when scanning multiple files thanks to caching crates.io index lookups (implemented in `rustsec` crate).
 - Notices about `cargo audit` or `rustsec` will now result in a scanning error being reported (exit code 2) as opposed to reporting them as vulnerabilities in the scanned binary (exit code 1). They are treated as warnings by default, so `--deny=warnings` is required to observe the new behavior.
 - The `binary-scanning` feature that adds the `cargo audit bin` subcommand is now enabled by default, but is not documented as such.

[#729]: https://github.com/rustsec/rustsec/pull/729

## 0.17.2 (2022-10-07)
### Changed

 - Fixed the screenshot URL in README.md

## 0.17.1 (2022-10-07)
### Added

 - Initial support for scanning binaries built with [`cargo auditable`](https://github.com/rust-secure-code/cargo-auditable)

## 0.17.0 (2022-05-23)
### Changed
- Update Abscissa to 0.6; replace `gumdrop` with `clap` v3 ([#525])
- 2021 edition upgrade ([#539])
- MSRV 1.57 ([#539], [#574])
- Bump `rustsec` to v0.26 ([#574])

### Fixed
- Terminal output fixups ([#570])

### Removed
- Unused `lazy_static` from dependencies ([#500])
- Deprecated `--deny-warnings` CLI option ([#545])

[#500]: https://github.com/rustsec/rustsec/pull/500
[#525]: https://github.com/rustsec/rustsec/pull/525
[#539]: https://github.com/rustsec/rustsec/pull/539
[#545]: https://github.com/rustsec/rustsec/pull/54qgq5
[#570]: https://github.com/rustsec/rustsec/pull/570
[#574]: https://github.com/rustsec/rustsec/pull/574

## 0.16.0 (2021-11-15)
### Changed
- Bump `rustsec` dependency to v0.25; MSRV 1.52 ([#480])

### Fixed
- Parse `--color=auto` correctly ([#436])

[#436]: https://github.com/rustsec/rustsec/pull/436
[#480]: https://github.com/rustsec/rustsec/pull/480

## 0.15.2 (2021-09-11)
### Added
- `vendored-libgit2` feature ([#432])

[#432]: https://github.com/rustsec/rustsec/pull/432

## 0.15.1 (2021-09-10)
### Changed
- Pin `thiserror` and `zeroize` to avoid MSRV breakages ([#415])

[#415]: https://github.com/rustsec/rustsec/pull/415

## 0.15.0 (2021-07-01)
### Added
- New exit status (`2`) for Cargo.lock parsing errors ([#368])

### Changed
- Bump `rustsec` crate dependency to v0.24 ([#388])

[#368]: https://github.com/RustSec/cargo-audit/pull/368
[#388]: https://github.com/RustSec/cargo-audit/pull/388

## 0.14.1 (2021-04-29)
### Added
- Generate release builds with github actions ([#337])

### Changed
- Bump rustsec from 0.23.2 to 0.23.3 ([#333])

[#333]: https://github.com/RustSec/cargo-audit/pull/333
[#337]: https://github.com/RustSec/cargo-audit/pull/337

## 0.14.0 (2021-03-07)
### Changed
- When running in no-fetch mode, allow accessing a non-git repo ([#315])
- Enable informational warnings with deny ([#320])
- Bump `rustsec` dependency to v0.23 ([#327]) 
- MSRV 1.46+ ([#327])

[#315]: https://github.com/RustSec/cargo-audit/pull/315
[#320]: https://github.com/RustSec/cargo-audit/pull/320
[#327]: https://github.com/RustSec/cargo-audit/pull/327

## 0.13.1 (2020-10-27)
### Changed
- Split `-D`/`--deny` and `--deny-warnings` ([#278])
- Bump `rustsec` crate to v0.22.2 ([#277])

### Fixed
- JSON serialization ([#277])

[#278]: https://github.com/RustSec/cargo-audit/pull/278
[#a277]: https://github.com/RustSec/cargo-audit/pull/277

## 0.13.0 (2020-10-26) [YANKED]
### Added
- Support for project specific config directories ([#252]) 

### Changed
- Bump `rustsec` crate to v0.22; MSRV 1.41+ ([#271])
- JSON report format changes ([#271])
- Presenter improvements ([#268])
- Make warning types an argument ([#206])

### Fixed
- `fix --dry-run` no longer requires argument ([#231])

[#271]: https://github.com/RustSec/cargo-audit/pull/258
[#268]: https://github.com/RustSec/cargo-audit/pull/268
[#255]: https://github.com/RustSec/cargo-audit/pull/255
[#252]: https://github.com/RustSec/cargo-audit/pull/252
[#231]: https://github.com/RustSec/cargo-audit/pull/231
[#206]: https://github.com/RustSec/cargo-audit/pull/206

## 0.12.1 (2020-09-22)
- Pin `smol_str` to v0.1.16 to ensure MSRV 1.41 compatibility ([#255], [#258])

[#258]: https://github.com/RustSec/cargo-audit/pull/258
[#255]: https://github.com/RustSec/cargo-audit/pull/255

## 0.12.0 (2020-05-06)
- Update `rustsec` crate to v0.20 ([#221])
- Regenerate lockfile after `cargo audit fix` ([#219])
- Update dependencies; MSRV 1.40+ ([#216])

[#221]: https://github.com/RustSec/cargo-audit/pull/221
[#219]: https://github.com/RustSec/cargo-audit/pull/219
[#216]: https://github.com/RustSec/cargo-audit/pull/216

## 0.11.2 (2020-02-07)
- Improve yanked crate auditing messages and config ([#200])
- Fix `-c`/`--color` command line argument ([#199])

[#200]: https://github.com/RustSec/cargo-audit/pull/200
[#199]: https://github.com/RustSec/cargo-audit/pull/199

## 0.11.1 (2020-01-24)
- Add `vendored-openssl` feature ([#193])

[#193]: https://github.com/RustSec/cargo-audit/pull/193

## 0.11.0 (2020-01-22)
- Update `rustsec` crate to v0.17 release; MSRV 1.39+ ([#186], [#188])
- Warn for yanked crates ([#180])
- Respect sources of dependencies when auditing ([#175])
- Upgrade to `abscissa` v0.5 ([#174])
- `cargo audit fix` subcommand ([#157], [#166], [#181])

[#188]: https://github.com/RustSec/cargo-audit/pull/188
[#186]: https://github.com/RustSec/cargo-audit/pull/186
[#181]: https://github.com/RustSec/cargo-audit/pull/181
[#180]: https://github.com/RustSec/cargo-audit/pull/180
[#175]: https://github.com/RustSec/cargo-audit/pull/175
[#174]: https://github.com/RustSec/cargo-audit/pull/174
[#166]: https://github.com/RustSec/cargo-audit/pull/166
[#157]: https://github.com/RustSec/cargo-audit/pull/157

## 0.10.0 (2019-10-13)
- Upgrade `rustsec` to v0.16; new self-audit system ([#155])
- Upgrade to Abscissa v0.4; MSRV 1.36 ([#154])

[#155]: https://github.com/RustSec/cargo-audit/pull/155
[#154]: https://github.com/RustSec/cargo-audit/pull/154

## 0.9.3 (2019-10-08)
- Update to `rustsec` crate v0.15.2 ([#149])
- presenter: Cleanups for informational advisories ([#148])
- presenter: Print better message when no solution is available ([#144])

[#149]: https://github.com/RustSec/cargo-audit/pull/149
[#148]: https://github.com/RustSec/cargo-audit/pull/148
[#144]: https://github.com/RustSec/cargo-audit/pull/144

## 0.9.2 (2019-10-01)
- Update to `rustsec` crate v0.15 ([#138])

[#138]: https://github.com/RustSec/cargo-audit/pull/138

## 0.9.1 (2019-09-25)
- Update to `rustsec` crate v0.14.1 ([#134])

[#134]: https://github.com/RustSec/cargo-audit/pull/134

## 0.9.0 (2019-09-25)
- Add `--deny-warnings` option ([#128])
- Upgrade to `rustsec` crate v0.14 ([#126])
- Configuration file: `~/.cargo/audit.toml` ([#123], [#125])
- Fix `--help` ([#113])
- Warn for outdated `rustsec` crate versions ([#112])
- Display warnings for select informational advisories ([#110])
- Display dependency trees with each advisory ([#109])

[#128]: https://github.com/RustSec/cargo-audit/pull/128
[#126]: https://github.com/RustSec/cargo-audit/pull/126
[#125]: https://github.com/RustSec/cargo-audit/pull/125
[#123]: https://github.com/RustSec/cargo-audit/pull/123
[#113]: https://github.com/RustSec/cargo-audit/pull/113
[#112]: https://github.com/RustSec/cargo-audit/pull/112
[#110]: https://github.com/RustSec/cargo-audit/pull/110
[#109]: https://github.com/RustSec/cargo-audit/pull/109

## 0.8.1 (2019-08-25)
- Fix `--version` ([#101])

[#101]: https://github.com/RustSec/cargo-audit/pull/101

## 0.8.0 (2019-08-16)
- Use the Abscissa application framework ([#85], [#87], [#92], [#94])
- Implement `--no-fetch` ([#97])
- Add support for reading lockfiles from STDIN ([#98])

[#98]: https://github.com/RustSec/cargo-audit/pull/98
[#97]: https://github.com/RustSec/cargo-audit/pull/97
[#94]: https://github.com/RustSec/cargo-audit/pull/94
[#92]: https://github.com/RustSec/cargo-audit/pull/92
[#87]: https://github.com/RustSec/cargo-audit/pull/87
[#85]: https://github.com/RustSec/cargo-audit/pull/85

## 0.7.0 (2019-07-15)
- Switch from `term` to `termcolor` crate ([#83])
- Update `gumdrop` to v0.6, `rustsec` crate to v0.12; min Rust 1.32+ ([#82])
- Produce valid JSON when no vulnerabilities are detected ([#77])
- Implement `--ignore` option ([#75])

[#83]: https://github.com/RustSec/cargo-audit/pull/83
[#82]: https://github.com/RustSec/cargo-audit/pull/82
[#77]: https://github.com/RustSec/cargo-audit/pull/77
[#75]: https://github.com/RustSec/cargo-audit/pull/75

## 0.6.1 (2018-12-16)
- Fix option parsing ([#64])

[#64]: https://github.com/RustSec/cargo-audit/pull/64

## 0.6.0 (2018-12-15)
- Update to Rust 2018 edition ([#61])
- Update to `rustsec` crate v0.10 ([#59])
- Prevent `--help` from exiting with error ([#57])
- Add `--json` flag for JSON output ([#41])

[#61]: https://github.com/RustSec/cargo-audit/pull/61
[#59]: https://github.com/RustSec/cargo-audit/pull/59
[#57]: https://github.com/RustSec/cargo-audit/pull/57
[#41]: https://github.com/RustSec/cargo-audit/pull/41

## 0.5.2 (2018-07-29)
- Have `cargo audit version` exit with status `0` ([#38])

[#38]: https://github.com/RustSec/cargo-audit/pull/38

## 0.5.1 (2018-07-29)
- Refactoring and UI improvements ([#37])

[#37]: https://github.com/RustSec/cargo-audit/pull/37

## 0.5.0 (2018-07-29)
- Upgrade `rustsec` crate to 0.9 ([#36])

[#36]: https://github.com/RustSec/cargo-audit/pull/36

## 0.4.0 (2018-07-24)
- Honor the `affected_platforms` attribute ([#35])
- Update `rustsec` crate dependency to `0.8` series ([#34])
- Update `term` crate dependency to `0.5` series ([#31])

[#35]: https://github.com/RustSec/cargo-audit/pull/35
[#34]: https://github.com/RustSec/cargo-audit/pull/34
[#31]: https://github.com/RustSec/cargo-audit/pull/31

## 0.3.2 (2018-07-23)
- README.md: Use `<img>` tag for screenshot so it renders on crates.io ([#28])

[#28]: https://github.com/RustSec/cargo-audit/pull/28

## 0.3.1 (2018-07-23)
- Use ` OR ` delimiter to display patched versions ([#25])
- Fix `cargo audit --version` ([#24])

[#25]: https://github.com/RustSec/cargo-audit/pull/25
[#24]: https://github.com/RustSec/cargo-audit/pull/24

## 0.3.0 (2018-07-23)
- Near rewrite of cargo-audit using `rustsec` 0.7.0 ([#22])

[#22]: https://github.com/RustSec/cargo-audit/pull/22

## 0.2.1 (2017-09-24)
- Use crate isatty to resolve Windows build errors ([#14])

[#14]: https://github.com/RustSec/cargo-audit/pull/14

## 0.2.0 (2017-03-05)
- Upgrade to rustsec 0.6.0 crate ([#12])
- Configurable colors ([#10])
- Avoid panicking if there are no dependencies ([#8])
- Handle error and instruct the user to generate a lockfile before audit ([#6])

[#12]: https://github.com/RustSec/cargo-audit/pull/12
[#10]: https://github.com/RustSec/cargo-audit/pull/10
[#8]: https://github.com/RustSec/cargo-audit/pull/8
[#6]: https://github.com/RustSec/cargo-audit/pull/6

## 0.1.1 (2017-02-27)
- Make cargo-audit a proper cargo subcommand ([#2])

[#2]: https://github.com/RustSec/cargo-audit/pull/2

## 0.1.0 (2017-02-27)
- Initial release
