# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.31](https://github.com/Nullus157/async-compression/compare/compression-core-v0.4.30...compression-core-v0.4.31) - 2025-11-21

### Added

- Add uninitialized output buffer support ([#414](https://github.com/Nullus157/async-compression/pull/414))

## [0.4.30](https://github.com/Nullus157/async-compression/compare/compression-core-v0.4.29...compression-core-v0.4.30) - 2025-11-06

### Other

- Simplify `WriteBuffer`: Rm `unwritten_initialized_mut` ([#413](https://github.com/Nullus157/async-compression/pull/413))
- Implement new traits `DecodeV2`/`EncodeV2` ([#398](https://github.com/Nullus157/async-compression/pull/398))
- Disable nightly feature `doc_auto_cfg` on docsrs ([#392](https://github.com/Nullus157/async-compression/pull/392))

## [0.4.29](https://github.com/Nullus157/async-compression/compare/compression-core-v0.4.28...compression-core-v0.4.29) - 2025-08-28

### Other

- Update Deps.rs badge ([#380](https://github.com/Nullus157/async-compression/pull/380))
- move async-compression to crates/ ([#379](https://github.com/Nullus157/async-compression/pull/379))
- Refactor compression_core::util ([#373](https://github.com/Nullus157/async-compression/pull/373))

## [0.4.28](https://github.com/Nullus157/async-compression/compare/compression-core-v0.4.27...compression-core-v0.4.28) - 2025-08-23

### Fixed

- fix wasi ci testing and update doc in README ([#367](https://github.com/Nullus157/async-compression/pull/367))

### Other

- Have separate package.version field for compression-* ([#369](https://github.com/Nullus157/async-compression/pull/369))
- Separate codecs as a separate crate, allow direct configuration ([#363](https://github.com/Nullus157/async-compression/pull/363))
- Release async-compression 0.4.8 ([#265](https://github.com/Nullus157/async-compression/pull/265))
- prepare release 0.4.7
- prepare release 0.4.6
- prepare release 0.4.3
- *(async-compression)* prepare release 0.4.2
- prepare release 0.4.1 ([#236](https://github.com/Nullus157/async-compression/pull/236))
- prepare async-compression release 0.4.0
- add deps and license badge
- remove references to old `stream` crate feature
- update repo links
- Update references to master branch
- Enable testing with all possible feature sets
- Update links in readme for repo location
- Mention features for local testing
- Update README.md
- Add licenses and trivial readme
