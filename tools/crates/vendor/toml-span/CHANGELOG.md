<!-- markdownlint-disable blanks-around-headings blanks-around-lists no-duplicate-heading -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

<!-- next-header -->
## [Unreleased] - ReleaseDate
## [0.6.0] - 2025-10-23
### Changed
- [PR#18](https://github.com/EmbarkStudios/toml-span/pull/18) updated `codespan-reporting` to 0.13.

## [0.5.2] - 2025-06-11
### Fixed
- [PR#16](https://github.com/EmbarkStudios/toml-span/pull/16) adding the missing `package.repository` field to point the workspace.

## [0.5.1] - 2025-05-20
### Fixed
- [PR#14](https://github.com/EmbarkStudios/toml-span/pull/14) fixed an issue where subtables were not correctly created with dotted keys. Thanks [@hacrvlq](https://github.com/hacrvlq)!

## [0.5.0] - 2025-04-03
### Changed
- [PR#12](https://github.com/EmbarkStudios/toml-span/pull/12) updated `codespan-reporting` to 0.12, and update to edition 2024.

## [0.4.1] - 2024-12-19
### Added
- [PR#11](https://github.com/EmbarkStudios/toml-span/pull/11) resolved [#10](https://github.com/EmbarkStudios/toml-span/issues/10) by adding span information for tables.

## [0.4.0] - 2024-12-16
### Changed
- [PR#9](https://github.com/EmbarkStudios/toml-span/pull/9) added `value` to `ErrorKind::UnexpectedValue`.

## [0.3.0] - 2024-06-26
### Changed
- [PR#7](https://github.com/EmbarkStudios/toml-span/pull/7) implemented `Borrow<str>` for `Key`, making the API much more ergonomic.

## [0.2.1] - 2024-06-13
### Changed
- [PR#6](https://github.com/EmbarkStudios/toml-span/pull/6) updates crates and fixed a lint.

### Fixed
- [PR#4](https://github.com/EmbarkStudios/toml-span/pull/4) added the missing `repository` field in the `toml-span` package manifest.
- [PR#5](https://github.com/EmbarkStudios/toml-span/pull/5) fixed the crate package missing the LICENSE-* files and CHANGELOG.md.

## [0.2.0] - 2024-02-22
### Added
- [PR#3](https://github.com/EmbarkStudios/toml-span/pull/3) actually added some documentation.
- [PR#3](https://github.com/EmbarkStudios/toml-span/pull/3) added `DeserError::merge`

### Fixed
- [PR#3](https://github.com/EmbarkStudios/toml-span/pull/3) `TableHelper::take` now appends the key to the `expected` array

### Changed
- [PR#3](https://github.com/EmbarkStudios/toml-span/pull/3) removed `TableHelper::with_default/parse/parse_opt`

## [0.1.0] - 2024-02-20
### Added
- Initial implementation

<!-- next-url -->
[Unreleased]: https://github.com/EmbarkStudios/toml-span/compare/0.6.0...HEAD
[0.6.0]: https://github.com/EmbarkStudios/toml-span/compare/0.5.2...0.6.0
[0.5.2]: https://github.com/EmbarkStudios/toml-span/compare/0.5.1...0.5.2
[0.5.1]: https://github.com/EmbarkStudios/toml-span/compare/0.5.0...0.5.1
[0.5.0]: https://github.com/EmbarkStudios/toml-span/compare/0.4.1...0.5.0
[0.4.1]: https://github.com/EmbarkStudios/toml-span/compare/0.4.0...0.4.1
[0.4.0]: https://github.com/EmbarkStudios/toml-span/compare/0.3.0...0.4.0
[0.3.0]: https://github.com/EmbarkStudios/toml-span/compare/0.2.1...0.3.0
[0.2.1]: https://github.com/EmbarkStudios/toml-span/compare/0.2.0...0.2.1
[0.2.0]: https://github.com/EmbarkStudios/toml-span/compare/0.1.0...0.2.0
[0.1.0]: https://github.com/EmbarkStudios/toml-span/releases/tag/0.1.0
