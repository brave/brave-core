# Changelog
All notable changes to this project will be documented in this file.

## [Unreleased] - ReleaseDate

### Changed

## [0.5.5] - 2023-10-31
### Changed
- Updated enum-as-inner to 0.6.0, to remove dependencies on both syn-1 and syn-2.
- Updated bitflags to minimum 2.0.

## [0.5.4] - 2022-12-09
### Changed
- Bumped byteorder crate to 1.4.3 due to failing tests.

## [0.5.3] - 2022-12-09
### Added
- Added accessor methods to destructure `CtlValue`.

## [0.5.2] - 2022-08-16
### Changed
- CI minimum version test failing. Adjust versions to fix.

## [0.5.1] - 2022-08-16
### Changed
- Remove crate version from example in readme.

## [0.5.0] - 2022-08-16
### Changed
- Improve iOS support with new Ctl variant.
- Increase minimum version of dependencies.

## [0.4.6] - 2022-08-07
### Changed
- Can't have more than 5 keywords in Cargo.toml. Remove the added iOS keyword.

## [0.4.5] - 2022-08-07
### Changed
- Enable use on iOS

## [0.4.4] - 2022-03-01
### Changed
- Use fmt to determine the exact type for CtlType::Int on MacOS

## [0.4.3] - 2021-11-01
### Changed
- Remove a leftover debug println.

## [0.4.2] - 2021-08-03
### Changed
- Add Cirrus CI for FreeBSD, macOS and Linux.
- Bump thiserror crate.
- Use sysctlnametomib(3) where available.
- Use sysctlbyname(3) on FreeBSD.
- Tell docs.rs to build docs for FreeBSD too.
- Don't include docs in package to reduce size.

## [0.4.1] - 2021-04-23
### Changed
- Replace deprecated failure crate with thiserror.
- Fix clippy lints.

## [0.4.0] - 2019-07-24
### Changed
- Add Linux support.
- Huge refactor.
- Improve BSD code to provide a cross platform compatible API.
- [BREAKING] Make static functions private, all calls now go through the Ctl object.

## [0.3.0] - 2019-01-07
### Changed
- Improve error handling.
- Publish CtlInfo struct.
- Add Cirrus CI script.

## [0.2.0] - 2018-05-28
### Changed
- Add iterator support (thanks to Fabian Freyer!).
- Add struct interface for control.
- Add documentation for macOS.
- Use failure create for error handling.

## [0.1.4] - 2018-01-04
### Changed
- Fix documentation link
- Fix test on FreeBSD

## [0.1.3] - 2018-01-04
### Added
- Macos support.

## [0.1.2] - 2017-05-23
### Added
- This changelog.
- API to get values by OID.
- Example value\_oid\_as.rs
- Node types can also contain data so treat Nodes same as Struct/Opaque.
