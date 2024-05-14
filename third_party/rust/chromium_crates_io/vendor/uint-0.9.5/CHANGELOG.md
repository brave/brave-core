# Changelog

The format is based on [Keep a Changelog].

[Keep a Changelog]: http://keepachangelog.com/en/1.0.0/

## [Unreleased]

## [0.9.5] - 2022-11-29
- Implemented bitwise assign traits. [#690](https://github.com/paritytech/parity-common/pull/690)

## [0.9.4] - 2022-09-20
- Made `one` const. [#650](https://github.com/paritytech/parity-common/pull/650)
- Made `max_value` const. [#652](https://github.com/paritytech/parity-common/pull/652)
- Made `is_zero` const. [#639](https://github.com/paritytech/parity-common/pull/639)
- Added `abs_diff`. [#665](https://github.com/paritytech/parity-common/pull/665)

## [0.9.3] - 2022-02-04
- Simplified and faster `div_mod`. [#478](https://github.com/paritytech/parity-common/pull/478)
- Fixed `overflowing_neg`. [#611](https://github.com/paritytech/parity-common/pull/611)

## [0.9.2] - 2022-01-28
- Migrated to 2021 edition, enforcing MSRV of `1.56.1`. [#601](https://github.com/paritytech/parity-common/pull/601)
- Display formatting support. [#603](ttps://github.com/paritytech/parity-common/pull/603)

## [0.9.1] - 2021-06-30
- Added `integer_sqrt` method. [#554](https://github.com/paritytech/parity-common/pull/554)

## [0.9.0] - 2021-01-05
- Allow `0x` prefix in `from_str`. [#487](https://github.com/paritytech/parity-common/pull/487)
### Breaking
- Optimized FromStr, made it no_std-compatible. [#468](https://github.com/paritytech/parity-common/pull/468)

## [0.8.5] - 2020-08-12
- Make const matching work again. [#421](https://github.com/paritytech/parity-common/pull/421)

## [0.8.4] - 2020-08-03
- Added a manual impl of `Eq` and `Hash`. [#390](https://github.com/paritytech/parity-common/pull/390)
- Removed some unsafe code and added big-endian support. [#407](https://github.com/paritytech/parity-common/pull/407)
- Added `checked_pow`. [#417](https://github.com/paritytech/parity-common/pull/417)

## [0.8.3] - 2020-04-27
- Added `arbitrary` feature. [#378](https://github.com/paritytech/parity-common/pull/378)
- Fixed UB in `from_big_endian`. [#381](https://github.com/paritytech/parity-common/pull/381)

## [0.8.2] - 2019-10-24
### Fixed
- Fixed 2018 edition imports. [#237](https://github.com/paritytech/parity-common/pull/237)
- Removed `uninitialized` usage. [#238](https://github.com/paritytech/parity-common/pull/238)
### Dependencies
- Updated dependencies. [#239](https://github.com/paritytech/parity-common/pull/239)
### Changed
- Modified AsRef impl. [#196](https://github.com/paritytech/parity-common/pull/196)
