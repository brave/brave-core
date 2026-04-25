# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.6.1] - 2023-08-17

 - Fix no_std build
 - Fix preallocation calculation
 - Test and benchmark code cleanup
 - Fix false positive issue with the `recover` fuzzing harness
 - Minor documentation, code, and ci improvements

## [0.6.0] - 2023-07-18

- Fork adapted to the needs of the STAR protocol
- Use a smaller (129-bit) field for better performance
- Choose Sophie Germain prime (2^128 + 12451)
- Various code cleanup and formatting
- Update dependencies

## [0.5.0] - 2021-03-14
### Added
- Zeroize memory on drop for generated secret shares

## [0.4.3] - 2021-02-04
### Changed
- Upgraded project dependencies

## [0.4.2] - 2020-08-03
### Fixed
- Small fix in docs

## [0.4.1] - 2020-04-23
### Added
- Fuzz tests

### Fixed
- Unexpected panic when trying to recover secret from different length shares
- Unexpected panic when trying to convert less than 2 bytes to `Share`

## [0.4.0] - 2020-04-02
### Added
- It is now possible to compile without `std` with `--no-default-features`

## [0.3.3] - 2020-03-23
### Changed
- Fix codecov badge

## [0.3.2] - 2020-03-09
### Changed
- Share structs now derives the `Clone` trait

## [0.3.1] - 2020-01-23
### Changed
- Sharks recover method now accepts any iterable collection

## [0.3.0] - 2020-01-22
### Added
- Share struct which allows to convert from/to byte vectors

### Changed
- Methods use the new Share struct, instead of (GF245, Vec<GF256>) tuples

## [0.2.0] - 2020-01-21
### Added
- Computations performed over GF256 (much faster)
- Secret can now be arbitrarily long

### Changed
- Some method names and docs
- Maximum number of shares enforced by Rust static types instead of conditional branching

### Removed
- Modular arithmetic around Mersenne primes

## [0.1.1] - 2020-01-13
### Fixed
- Typo in cargo description

### Removed
- Maintenance badges in cargo file

## [0.1.0] - 2020-01-13
### Added
- Initial version
