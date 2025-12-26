# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.35](https://github.com/Nullus157/async-compression/compare/compression-codecs-v0.4.34...compression-codecs-v0.4.35) - 2025-12-11

### Other

- Add Crc checksum validation for gzip::header::Parser ([#432](https://github.com/Nullus157/async-compression/pull/432))

## [0.4.34](https://github.com/Nullus157/async-compression/compare/compression-codecs-v0.4.33...compression-codecs-v0.4.34) - 2025-12-07

### Other

- Optimize GzipEncoder to not allocate for header and footer ([#431](https://github.com/Nullus157/async-compression/pull/431))
- Optimize GzipDecoder ([#430](https://github.com/Nullus157/async-compression/pull/430))
- Optimize gzip::header remove heap allocation ([#428](https://github.com/Nullus157/async-compression/pull/428))

## [0.4.33](https://github.com/Nullus157/async-compression/compare/compression-codecs-v0.4.32...compression-codecs-v0.4.33) - 2025-11-21

### Added

- Add uninitialized output buffer support ([#414](https://github.com/Nullus157/async-compression/pull/414))

### Other

- Set msrv for codecs to 1.83 ([#422](https://github.com/Nullus157/async-compression/pull/422))

## [0.4.32](https://github.com/Nullus157/async-compression/compare/compression-codecs-v0.4.31...compression-codecs-v0.4.32) - 2025-11-06

### Added

- allow reading uncompressed size ([#396](https://github.com/Nullus157/async-compression/pull/396))

### Fixed

- `UnexpectedEof` on truncated input ([#412](https://github.com/Nullus157/async-compression/pull/412))

### Other

- Simplify `WriteBuffer`: Rm `unwritten_initialized_mut` ([#413](https://github.com/Nullus157/async-compression/pull/413))
- Implement new traits `DecodeV2`/`EncodeV2` ([#398](https://github.com/Nullus157/async-compression/pull/398))
- Re-export core in codecs ([#395](https://github.com/Nullus157/async-compression/pull/395))
- Disable nightly feature `doc_auto_cfg` on docsrs ([#392](https://github.com/Nullus157/async-compression/pull/392))

## [0.4.31](https://github.com/Nullus157/async-compression/compare/compression-codecs-v0.4.30...compression-codecs-v0.4.31) - 2025-09-25

### Other

- Use io::ErrorKind::OutOfMemory for xz2/bzip2 codecs ([#387](https://github.com/Nullus157/async-compression/pull/387))

## [0.4.30](https://github.com/Nullus157/async-compression/compare/compression-codecs-v0.4.29...compression-codecs-v0.4.30) - 2025-08-31

### Other

- rm unused dep from async-compression and compression-codecs ([#381](https://github.com/Nullus157/async-compression/pull/381))

## [0.4.29](https://github.com/Nullus157/async-compression/compare/compression-codecs-v0.4.28...compression-codecs-v0.4.29) - 2025-08-28

### Other

- Update Deps.rs badge ([#380](https://github.com/Nullus157/async-compression/pull/380))
- move async-compression to crates/ ([#379](https://github.com/Nullus157/async-compression/pull/379))

## [0.4.28](https://github.com/Nullus157/async-compression/compare/compression-codecs-v0.4.27...compression-codecs-v0.4.28) - 2025-08-23

### Fixed

- fix wasi ci testing and update doc in README ([#367](https://github.com/Nullus157/async-compression/pull/367))

### Other

- Have separate package.version field for compression-* ([#369](https://github.com/Nullus157/async-compression/pull/369))
- Fix docs.rs build for compression-codecs ([#365](https://github.com/Nullus157/async-compression/pull/365))
