# Change Log: enum-as-inner

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## 0.6.0

- Previous changes deserved a minor version bump, 0.5.2 yanked, 0.6.0 released

## 0.5.2

- Add is_* impl for non unit variants, #91 by goolmoos
- Fully qualify both Option and Result in generated code, #96 by kepler-5

## 0.5.1

- Generated functions now marked `#[inline]` to help with lto across crate boundaries for libraries, #87 by zxch3n

## 0.5.0

- Changed all references in generated functions to use `Self` where applicable.

## 0.4.0

### Changed

- Change unit variants to use predicate functions (@CeleritasCelery) #76

## 0.3.4

### Changed

- updated `heck` to 0.4 #82

## 0.3.3

### Added

- Support for Generic enum members (@Jan561) #19

## 0.3.2

### Added

- Added `as_mut_*` for mutable access to inners (@yuyoyuppe) #14

## 0.3.1

### Added

- this CHANGELOG.md

### Changes

- Updated docs for new `use` syntax of macros

## 0.3

### Changes

- Upgrade to work with 1.0 versions of proc-macro2, quote, and syn

## 0.2

### Fixes

- some bug fixes for edge conditions

## 0.1

### Added

- first release
