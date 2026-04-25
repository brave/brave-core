# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.9.0] - 2024-11-11

### Changed

- Upgraded to `auditable-serde` v0.8.x
- Upgraded to `miniz_oxide` v0.8.x and removed its types from the public API to simplify future upgrades

## [0.8.0] - 2024-07-30

### Changed

- Upgraded to `auditable-serde` v0.7.x

## [0.7.2] - 2024-05-08

### Changed

 - Upgraded to `auditable-extract` v0.3.4, removing nearly all dependencies with `unsafe` code in them pulled in by `wasm` feature
 - Enabled the `wasm` feature by default now that it doesn't pull in `unsafe` code

## [0.7.1] - 2024-05-03

### Added

 - Added WebAssembly support, gated behind the non-default `wasm` feature

## [0.7.0] - 2023-04-27

### Changed

 - Upgraded to `auditable-serde` v0.6.x, the only change is an upgrade to `cargo-lock` v9.x

### Added
 - This changelog file
