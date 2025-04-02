# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html). All versions prior
to 1.0.0 are beta releases.

## [0.2.1] - 2022-09-24
### Added
- `impl_no_dynamic_usage!()` helper macro to implement `DynamicUsage` for simple
  types that don't allocate.

## [0.2.0] - 2021-09-14
### Added
- `memuse::DynamicUsage` impls for the following types:
  - `()`
  - `str`
  - `[T: DynamicUsage]`
  - `Box<T: DynamicUsage>`
  - `Result<T: DynamicUsage, E: DynamicUsage>`

### Removed
- `memuse::DynamicUsage` impls for `&str` and `&[T]` (replaced by the impls on
  `str` and `[T]`).
- `memuse::NoDynamicUsage` trait (which was causing trait inference problems
  that prevented `&T` and `Box<T>` from working).

## [0.1.0] - 2021-09-05
Initial release!
