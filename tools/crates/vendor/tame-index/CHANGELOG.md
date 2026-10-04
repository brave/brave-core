<!-- markdownlint-disable blanks-around-headings blanks-around-lists no-duplicate-heading -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

<!-- next-header -->
## [Unreleased] - ReleaseDate
## [0.24.1] - 2025-10-23
### Changed
- [a6abdbb](https://github.com/EmbarkStudios/tame-index/commit/a6abdbbf0c2550402a2e8a61af5ebe5a9e1e050d) updated toml-span -> 0.6.

## [0.24.0] - 2025-10-23
### Changed
- [PR#96](https://github.com/EmbarkStudios/tame-index/pull/96) updated gix -> 0.74.

## [0.23.1] - 2025-10-11
### Fixed
- [PR#95](https://github.com/EmbarkStudios/tame-index/pull/95) resolved [#94](https://github.com/EmbarkStudios/tame-index/issues/94) by actually using an infinite timeout on Windows when timeout is `None`.

## [0.23.0] - 2025-08-08
### Changed
- [PR#93](https://github.com/EmbarkStudios/tame-index/pull/93) updated gix -> 0.73.

## [0.22.0] - 2025-06-11
### Changed
- [PR#90](https://github.com/EmbarkStudios/tame-index/pull/90) updated the `IndexConfig` to match the current cargo schema, thanks [nox](https://github.com/nox)!
- [PR#91](https://github.com/EmbarkStudios/tame-index/pull/91) updated the source replacement code to be able to find replacements in relative paths from the project, thanks [kornelski](https://github.com/kornelski)!

## [0.21.0] - 2025-05-22
### Changed
- [PR#88](https://github.com/EmbarkStudios/tame-index/pull/88) updated gix -> 0.72.

## [0.20.1] - 2025-04-07
### Fixed
- [PR#87](https://github.com/EmbarkStudios/tame-index/pull/87) resolved [#86](https://github.com/EmbarkStudios/tame-index/issues/86) by adding a missing `unsafe` that caused the Windows target to fail to compile.

## [0.20.0] - 2025-04-07
### Fixed
- [PR#85](https://github.com/EmbarkStudios/tame-index/pull/85) updated gix -> 0.71 and tokio to 1.44.2, addressing [RUSTSEC-2025-0021](https://rustsec.org/advisories/RUSTSEC-2025-0021) and [RUSTSEC-2025-0023](https://rustsec.org/advisories/RUSTSEC-2025-0023).

## [0.19.0] - 2025-04-03
### Changed
- [PR#84](https://github.com/EmbarkStudios/tame-index/pull/84) updated crates, and moved to edition 2024.

## [0.18.1] - 2025-02-24
### Fixed
- [PR#83](https://github.com/EmbarkStudios/tame-index/pull/83) adds an additional fix for non-crates.io urls not fixed in [PR#82](https://github.com/EmbarkStudios/tame-index/pull/82) as cargo now canonicalizes all URLs.

## [0.18.0] - 2025-02-20
### Changed
- [PR#82](https://github.com/EmbarkStudios/tame-index/pull/82) resolved [#81](https://github.com/EmbarkStudios/tame-index/issues/81), updating the hash calculation to match cargo 1.85.0. The decision of the hash calculation is based on the cargo version, which can be specified by the user via `IndexLocation::cargo_version`, defaulting to retrieving the version from the current environment if not specified allowing the calculation to work regardless of which cargo version is used.
- [PR#82](https://github.com/EmbarkStudios/tame-index/pull/82) added a re-export of `semver::Version`.

## [0.17.0] - 2025-01-19
### Changed
- [PR#75](https://github.com/EmbarkStudios/tame-index/pull/78) updated `gix` -> 0.70.
- [PR#80](https://github.com/EmbarkStudios/tame-index/pull/80) updated dependencies.

## [0.16.0] - 2024-11-28
### Changed
- [PR#75](https://github.com/EmbarkStudios/tame-index/pull/75) updated `gix` -> 0.68.

## [0.15.0] - 2024-11-11
### Changed
- [PR#74](https://github.com/EmbarkStudios/tame-index/pull/74) updated `gix` -> 0.67.

### Fixed
- [PR#74](https://github.com/EmbarkStudios/tame-index/pull/74) fixed a bug where the list of reserved crate names was not sorted correctly for binary searching.

## [0.14.0] - 2024-09-20
### Changed
- [PR#73](https://github.com/EmbarkStudios/tame-index/pull/73) updated `gix` -> 0.66.

## [0.13.2] - 2024-08-20
### Fixed
- [PR#71](https://github.com/EmbarkStudios/tame-index/pull/71) resolved [#70](https://github.com/EmbarkStudios/tame-index/issues/70) by using the correct feature flag for docs.rs.

### Changed
- [PR#72](https://github.com/EmbarkStudios/tame-index/pull/72) updated crates.

## [0.13.1] - 2024-08-01
### Fixed
- [PR#69](https://github.com/EmbarkStudios/tame-index/pull/69) resolved an issue where 32-bit targets would have a different ident hash from what cargo would have due to cargo being target dependent in the hash calculation.

## [0.13.0] - 2024-07-25
### Changed
- [PR#67](https://github.com/EmbarkStudios/tame-index/pull/67) updated `gix` -> 0.64.

## [0.12.2] - 2024-07-22
### Added
- [PR#66](https://github.com/EmbarkStudios/tame-index/pull/66) added the `gix-curl` feature, which is mutually exclusive with the `git` and `gix-reqwest` features.

## [0.12.1] - 2024-06-26
### Changed
- [PR#65](https://github.com/EmbarkStudios/tame-index/pull/65) updated `toml-span` -> 0.3.0.

## [0.12.0] - 2024-05-24
### Changed
- [PR#64](https://github.com/EmbarkStudios/tame-index/pull/64) updated `gix` -> 0.63.

## [0.11.1] - 2024-05-03
- [PR#61](https://github.com/EmbarkStudios/tame-index/pull/61) addressed [#60](https://github.com/EmbarkStudios/tame-index/issues/60) by adding `IndexUrl::for_registry_name` to read a registry's index url from the config/environment.

## [0.11.0] - 2024-04-23
### Changed
- [PR#59](https://github.com/EmbarkStudios/tame-index/pull/59) updated `gix` -> 0.62, `reqwest` -> 0.12, `http` -> 1.1.

## [0.10.0] - 2024-03-21
### Changed
- [PR#54](https://github.com/EmbarkStudios/tame-index/pull/54) updated `gix` -> 0.61.
- [PR#53](https://github.com/EmbarkStudios/tame-index/pull/53) updated `gix` -> 0.60.

## [0.9.9] - 2024-03-21
### Changed
- [PR#54](https://github.com/EmbarkStudios/tame-index/pull/54) updated `gix` -> 0.61.

## [0.9.8] - 2024-03-17
### Changed
- [PR#53](https://github.com/EmbarkStudios/tame-index/pull/53) updated `gix` -> 0.60.

## [0.9.7] - 2024-03-12
### Added
- [PR#52](https://github.com/EmbarkStudios/tame-index/pull/52) added `ComboIndexCache::cache_path` to retrieve the path of a particular crate's index entry.

## [0.9.6] - 2024-03-12
### Changed
- [PR#51](https://github.com/EmbarkStudios/tame-index/pull/51) updated dependencies.

## [0.9.5] - 2024-02-22
### Changed
- [PR#49](https://github.com/EmbarkStudios/tame-index/pull/49) updated `toml-span` -> 0.2

## [0.9.4] - 2024-02-20
### Changed
- [PR#48](https://github.com/EmbarkStudios/tame-index/pull/48) replaced `toml` with `toml-span` removing several dependencies.

## [0.9.3] - 2024-02-07
### Fixed
- [PR#47](https://github.com/EmbarkStudios/tame-index/pull/47) fixed [#46](https://github.com/EmbarkStudios/tame-index/issues/46) by ensuring one full DNS lookup and request response roundtrip is made before going wide to ensure that excessive DNS lookups and connections are not made.

## [0.9.2] - 2024-01-21
### Changed
- [PR#45](https://github.com/EmbarkStudios/tame-index/pull/45) bumped `gix` -> 0.58

## [0.9.1] - 2024-01-12
### Changed
- [PR#44](https://github.com/EmbarkStudios/tame-index/pull/44) bumped `gix` -> 0.57

## [0.9.0] - 2023-12-13
### Fixed
- [PR#43](https://github.com/EmbarkStudios/tame-index/pull/43) fixed the file lock options from `LockOptions::cargo_package_lock` to be `exclusive` to more closely match Cargo's behavior. This would not have been a problem in practice, but is more correct now.

### Changed
- [PR#43](https://github.com/EmbarkStudios/tame-index/pull/43) bumped `gix` -> 0.56

## [0.8.0] - 2023-11-06
### Fixed
- [PR#41](https://github.com/EmbarkStudios/tame-index/pull/41) resolved [#29](https://github.com/EmbarkStudios/tame-index/issues/29) by force disabling gpg signing in test.
- Commit e3c6ff1 bumped the patch version of `windows-targets` to .5 to prevent using older versions that don't compile (See [#40](https://github.com/EmbarkStudios/tame-index/issues/40))

### Changed
- [PR#41](https://github.com/EmbarkStudios/tame-index/pull/41) bumped `gix` -> 0.55

## [0.7.2] - 2023-10-18
### Fixed
- [PR#39](https://github.com/EmbarkStudios/tame-index/pull/39) resolved [#38](https://github.com/EmbarkStudios/tame-index/issues/38) by ensuring all parent directories are created before attempting a clone with `gix`.

## [0.7.1] - 2023-09-29
### Fixed
- [PR#34](https://github.com/EmbarkStudios/tame-index/pull/33) resolved a compile issue when targeting `musl` libc.

## [0.7.0] - 2023-09-29
### Changed
- [PR#32](https://github.com/EmbarkStudios/tame-index/pull/32) resolved [#31](https://github.com/EmbarkStudios/tame-index/issues/31) by reducing the size of `Error`.
- [PR#33](https://github.com/EmbarkStudios/tame-index/pull/33) updated dependencies, notably `gix` -> 0.54.
- [PR#33](https://github.com/EmbarkStudios/tame-index/pull/33) added a `tame_index::utils::flock::FileLock` parameter to all methods on indices that perform disk operations.

### Added
- [PR#33](https://github.com/EmbarkStudios/tame-index/pull/33) added `tame_index::utils::flock`, which contains a `FileLock` for holding an OS file lock for a particular path, as well as `LockOptions` for creating them.

### Fixed
- [PR#33](https://github.com/EmbarkStudios/tame-index/pull/33) resolved [#30](https://github.com/EmbarkStudios/tame-index/issues/30) by removing the usage of `gix::lock` in favor of the aforementioned `FileLock`
- [PR#33](https://github.com/EmbarkStudios/tame-index/pull/33) resolved [#17](https://github.com/EmbarkStudios/tame-index/issues/17) by adding `LockOptions::cargo_package_lock` to easily create a lock file compatible with cargo's own ($CARGO_HOME global) package lock.

## [0.6.0] - 2023-09-11
### Changed
- [PR#27](https://github.com/EmbarkStudios/tame-index/pull/27) updated `gix` to 0.53.1. Thanks [@Byron](https://github.com/Byron)!

## [0.5.6] - 2023-09-11 **yanked**
### Changed
- [PR#27](https://github.com/EmbarkStudios/tame-index/pull/27) updated `gix` to 0.53.1. Thanks [@Byron](https://github.com/Byron)!

## [0.5.5] - 2023-09-06
### Changed
- [PR#26](https://github.com/EmbarkStudios/tame-index/pull/26) changed sparse index request creation to not use HTTP/2 for the version to support corporate potato proxies. This results in a slight but noticeable degradation in throughput when making many requests to a sparse index.

## [0.5.4] - 2023-08-24
### Fixed
- [PR#24](https://github.com/EmbarkStudios/tame-index/pull/24) resolved [#23](https://github.com/EmbarkStudios/tame-index/issues/23) by fixing a bug where index cache paths were not lower cased as cargo does.

## [0.5.3] - 2023-08-23
### Fixed
- [PR#22](https://github.com/EmbarkStudios/tame-index/pull/22) fixed an issue where ssh index urls would be mapped to the incorrect local directory. This issue was raised in [cargo-deny](https://github.com/EmbarkStudios/cargo-deny/issues/548).

## [0.5.2] - 2023-08-23
### Fixed
- [`d9cb55f`] fixed and issue with docs.rs documentation building.

## [0.5.1] - 2023-08-23
### Added
- [PR#20](https://github.com/EmbarkStudios/tame-index/pull/20) publicly exposed `tame_index::external::http` for easier downstream usage.

## [0.5.0] - 2023-08-23
### Fixed
- [PR#18](https://github.com/EmbarkStudios/tame-index/pull/18) resolved [#16](https://github.com/EmbarkStudios/tame-index/issues/16) by marking `ComboIndexCache` and `ComboIndex` as `#[non_exhaustive]`. This avoids build breaks if the `local` feature is enabled in one transitive dependency and not in another, as much as I hate `non_exhaustive`.

### Changed
- [PR#18](https://github.com/EmbarkStudios/tame-index/pull/18) changed `SparseIndex::make_remote_request` to take an optional [ETag](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/ETag), completely avoiding disk I/O, which allows `SparseIndex` to be used for making and parsing requests without worrying about cargo's global package lock.

## [0.4.1] - 2023-08-21
### Added
- [PR#15](https://github.com/EmbarkStudios/tame-index/pull/15) added the `native-certs` feature to be able to use the OS certificate store instead of `webpki-roots`. Thanks [@Shnatsel](https://github.com/Shnatsel)!

## [0.4.0] - 2023-08-18
### Changed
- [PR#14](https://github.com/EmbarkStudios/tame-index/pull/14) added the ability to specify the repository lock policy when calling `RemoteGitIndex::with_options`. Thanks [@Shnatsel](https://github.com/Shnatsel)!

## [0.3.2] - 2023-08-15
### Fixed
- [PR#13](https://github.com/EmbarkStudios/tame-index/pull/13) fixed a bug where git repository url canonicalization was incorrect if the url was not a github.com url that ended with .git.

## [0.3.1] - 2023-08-04
### Added
- [PR#11](https://github.com/EmbarkStudios/tame-index/pull/11) added `RemoteSparseIndex::krates`, `AsyncRemoteSparseIndex::krates`, and `AsyncRemoteSparseIndex::krates_blocking` as helper methods for improving throughput when fetching index entries for many crates.

## [0.3.0] - 2023-08-03
### Changed
- [PR#10](https://github.com/EmbarkStudios/tame-index/pull/10) unfortunately had to [relax the constraint](https://github.com/rustsec/rustsec/issues/759) that crate versions in an index are always parsable as `semver::Version`.

## [0.2.5] - 2023-08-02
### Fixed
- [PR#9](https://github.com/EmbarkStudios/tame-index/pull/9) resolved [#8](https://github.com/EmbarkStudios/tame-index/issues/8) by ensuring (valid) non-official cargo build version output can also be parsed.

## [0.2.4] - 2023-07-28
### Fixed
- [PR#7](https://github.com/EmbarkStudios/tame-index/pull/7) fixed an issue where `RemoteGitIndex::fetch` could fail in environments where the git committer was not configured.

### Changed
- [PR#7](https://github.com/EmbarkStudios/tame-index/pull/7) change how `RemoteGitIndex` looks up blobs. Previously fetching would actually update references, now however we write a `FETCH_HEAD` similarly to git/libgit2, and uses that (or other) reference to find the commit to use, rather than updating the HEAD to point to the same commit as the remote HEAD.

## [0.2.3] - 2023-07-26
### Fixed
- [PR#6](https://github.com/EmbarkStudios/tame-index/pull/6) fixed two bugs with git registries.
    1. `cargo` does not set remotes for git registry indices, the previous code assumed there was a remote, thus failed to fetch updates
    2. Updating reflogs after a fetch would fail in CI-like environments without a global git config that set the committer, `committer.name` is now set to `tame-index`

## [0.2.2] - 2023-07-26
### Changed
- [PR#5](https://github.com/EmbarkStudios/tame-index/pull/5) relaxed `rust-version` to 1.67.0.

## [0.2.1] - 2023-07-26
### Added
- [PR#4](https://github.com/EmbarkStudios/tame-index/pull/4) added `GitError::is_spurious` and `GitError::is_locked` to detect fetch errors that could potentially succeed in the future if retried.

### Changed
- [PR#4](https://github.com/EmbarkStudios/tame-index/pull/4) now re-exports `reqwest` and `gix` from `tame_index::externals` for easier downstream usage.

## [0.2.0] - 2023-07-25
### Added
- [PR#3](https://github.com/EmbarkStudios/tame-index/pull/3) added support for [`Local Registry`](https://doc.rust-lang.org/cargo/reference/source-replacement.html#local-registry-sources)
- [PR#3](https://github.com/EmbarkStudios/tame-index/pull/3) added [`LocalRegistry`] as an option for `ComboIndexCache`
- [PR#3](https://github.com/EmbarkStudios/tame-index/pull/3) added `KrateName::cargo` and `KrateName::crates_io` options for validating crates names against the (current) constraints of cargo and crates.io respectively.

### Changed
- [PR#3](https://github.com/EmbarkStudios/tame-index/pull/3) refactored how index initialization is performed by splitting out the individual pieces into a cleaner API, adding the types `IndexUrl`, `IndexPath`, and `IndexLocation`

### Fixed
- [PR#3](https://github.com/EmbarkStudios/tame-index/pull/3) fixed an issue where the .cache entries for a git index were not using the same cache version of cargo, as of 1.65.0+. cargo in those versions now uses the object id of the blob the crate is read from, rather than the `HEAD` commit hash, for more granular change detection.

## [0.1.0] - 2023-07-05
### Added
- [PR#1](https://github.com/EmbarkStudios/tame-index/pull/1) added the initial working implementation for this crate

## [0.0.1] - 2023-06-19
### Added
- Initial crate squat

<!-- next-url -->
[Unreleased]: https://github.com/EmbarkStudios/tame-index/compare/0.24.1...HEAD
[0.24.1]: https://github.com/EmbarkStudios/tame-index/compare/0.24.0...0.24.1
[0.24.0]: https://github.com/EmbarkStudios/tame-index/compare/0.23.1...0.24.0
[0.23.1]: https://github.com/EmbarkStudios/tame-index/compare/0.23.0...0.23.1
[0.23.0]: https://github.com/EmbarkStudios/tame-index/compare/0.22.0...0.23.0
[0.22.0]: https://github.com/EmbarkStudios/tame-index/compare/0.21.0...0.22.0
[0.21.0]: https://github.com/EmbarkStudios/tame-index/compare/0.20.1...0.21.0
[0.20.1]: https://github.com/EmbarkStudios/tame-index/compare/0.20.0...0.20.1
[0.20.0]: https://github.com/EmbarkStudios/tame-index/compare/0.19.0...0.20.0
[0.19.0]: https://github.com/EmbarkStudios/tame-index/compare/0.18.1...0.19.0
[0.18.1]: https://github.com/EmbarkStudios/tame-index/compare/0.18.0...0.18.1
[0.18.0]: https://github.com/EmbarkStudios/tame-index/compare/0.17.0...0.18.0
[0.17.0]: https://github.com/EmbarkStudios/tame-index/compare/0.16.0...0.17.0
[0.16.0]: https://github.com/EmbarkStudios/tame-index/compare/0.15.0...0.16.0
[0.15.0]: https://github.com/EmbarkStudios/tame-index/compare/0.14.0...0.15.0
[0.14.0]: https://github.com/EmbarkStudios/tame-index/compare/0.13.2...0.14.0
[0.13.2]: https://github.com/EmbarkStudios/tame-index/compare/0.13.1...0.13.2
[0.13.1]: https://github.com/EmbarkStudios/tame-index/compare/0.13.0...0.13.1
[0.13.0]: https://github.com/EmbarkStudios/tame-index/compare/0.12.2...0.13.0
[0.12.2]: https://github.com/EmbarkStudios/tame-index/compare/0.12.1...0.12.2
[0.12.1]: https://github.com/EmbarkStudios/tame-index/compare/0.12.0...0.12.1
[0.12.0]: https://github.com/EmbarkStudios/tame-index/compare/0.11.1...0.12.0
[0.11.1]: https://github.com/EmbarkStudios/tame-index/compare/0.11.0...0.11.1
[0.11.0]: https://github.com/EmbarkStudios/tame-index/compare/0.10.0...0.11.0
[0.10.0]: https://github.com/EmbarkStudios/tame-index/compare/0.9.9...0.10.0
[0.9.9]: https://github.com/EmbarkStudios/tame-index/compare/0.9.8...0.9.9
[0.9.8]: https://github.com/EmbarkStudios/tame-index/compare/0.9.7...0.9.8
[0.9.7]: https://github.com/EmbarkStudios/tame-index/compare/0.9.6...0.9.7
[0.9.6]: https://github.com/EmbarkStudios/tame-index/compare/0.9.5...0.9.6
[0.9.5]: https://github.com/EmbarkStudios/tame-index/compare/0.9.4...0.9.5
[0.9.4]: https://github.com/EmbarkStudios/tame-index/compare/0.9.3...0.9.4
[0.9.3]: https://github.com/EmbarkStudios/tame-index/compare/0.9.2...0.9.3
[0.9.2]: https://github.com/EmbarkStudios/tame-index/compare/0.9.1...0.9.2
[0.9.1]: https://github.com/EmbarkStudios/tame-index/compare/0.9.0...0.9.1
[0.9.0]: https://github.com/EmbarkStudios/tame-index/compare/0.8.0...0.9.0
[0.8.0]: https://github.com/EmbarkStudios/tame-index/compare/0.7.2...0.8.0
[0.7.2]: https://github.com/EmbarkStudios/tame-index/compare/0.7.1...0.7.2
[0.7.1]: https://github.com/EmbarkStudios/tame-index/compare/0.7.0...0.7.1
[0.7.0]: https://github.com/EmbarkStudios/tame-index/compare/0.6.0...0.7.0
[0.6.0]: https://github.com/EmbarkStudios/tame-index/compare/0.5.6...0.6.0
[0.5.6]: https://github.com/EmbarkStudios/tame-index/compare/0.5.5...0.5.6
[0.5.5]: https://github.com/EmbarkStudios/tame-index/compare/0.5.4...0.5.5
[0.5.4]: https://github.com/EmbarkStudios/tame-index/compare/0.5.3...0.5.4
[0.5.3]: https://github.com/EmbarkStudios/tame-index/compare/0.5.2...0.5.3
[0.5.2]: https://github.com/EmbarkStudios/tame-index/compare/0.5.1...0.5.2
[0.5.1]: https://github.com/EmbarkStudios/tame-index/compare/0.5.0...0.5.1
[0.5.0]: https://github.com/EmbarkStudios/tame-index/compare/0.4.1...0.5.0
[0.4.1]: https://github.com/EmbarkStudios/tame-index/compare/0.4.0...0.4.1
[0.4.0]: https://github.com/EmbarkStudios/tame-index/compare/0.3.2...0.4.0
[0.3.2]: https://github.com/EmbarkStudios/tame-index/compare/0.3.1...0.3.2
[0.3.1]: https://github.com/EmbarkStudios/tame-index/compare/0.3.0...0.3.1
[0.3.0]: https://github.com/EmbarkStudios/tame-index/compare/0.2.5...0.3.0
[0.2.5]: https://github.com/EmbarkStudios/tame-index/compare/0.2.4...0.2.5
[0.2.4]: https://github.com/EmbarkStudios/tame-index/compare/0.2.3...0.2.4
[0.2.3]: https://github.com/EmbarkStudios/tame-index/compare/0.2.2...0.2.3
[0.2.2]: https://github.com/EmbarkStudios/tame-index/compare/0.2.1...0.2.2
[0.2.1]: https://github.com/EmbarkStudios/tame-index/compare/0.2.0...0.2.1
[0.2.0]: https://github.com/EmbarkStudios/tame-index/compare/0.1.0...0.2.0
[0.1.0]: https://github.com/EmbarkStudios/tame-index/compare/0.0.1...0.1.0
[0.0.1]: https://github.com/EmbarkStudios/tame-index/releases/tag/0.0.1
