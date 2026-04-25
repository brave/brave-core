# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.5] - 2024-10-14

### Changed

 - When extracting audit data from WASM, nested sections are now also checked for audit data. This makes extraction work for WebAssembly components.

## [0.3.4] - 2024-05-08

### Changed

 - Upgraded to `wasmparser` 0.207.0, removing nearly all dependencies with `unsafe` code in them
 - Enabled the `wasm` feature by default now that it doesn't pull in `unsafe` code

### Added

 - This changelog file

## [0.3.3] - 2024-05-03

### Added

 - WebAssembly support, gated behind the non-default `wasm` feature
