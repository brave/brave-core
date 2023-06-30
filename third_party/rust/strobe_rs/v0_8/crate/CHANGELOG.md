# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.8.1] - 2022-10-10

### Changes
* Improved docs and fixed docsrs build

## [0.8.0] - 2022-10-10

### Additions
* Made `Strobe` struct impl `Zeroize`
* Implemented `std::error::Error` for `AuthError`, when `std` is set

### Changes
* Made `Strobe::version_str` return a `[u8]` rather than a `String`, thus removing the need for allocation
* Updated deps

## [0.7.1] - 2022-02-13

### Additions
* Made `Strobe` struct impl `ZeroizeOnDrop` (PR [#4](https://github.com/rozbb/strobe-rs/pull/4))

## [0.7.0] - 2021-12-29

### Additions
* Added `serialize_secret_state` feature, which impls `serde::Serialize` and `serde::Deserialize` for `Strobe` (PR [#3](https://github.com/rozbb/strobe-rs/pull/3))

### Changes
* Some light refactoring and improvement to tests
