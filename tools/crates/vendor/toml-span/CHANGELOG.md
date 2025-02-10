<!-- markdownlint-disable blanks-around-headings blanks-around-lists no-duplicate-heading -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

<!-- next-header -->
## [Unreleased] - ReleaseDate
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
[Unreleased]: https://github.com/EmbarkStudios/toml-span/compare/0.3.0...HEAD
[0.3.0]: https://github.com/EmbarkStudios/toml-span/compare/0.2.1...0.3.0
[0.2.1]: https://github.com/EmbarkStudios/toml-span/compare/0.2.0...0.2.1
[0.2.0]: https://github.com/EmbarkStudios/toml-span/compare/0.1.0...0.2.0
[0.1.0]: https://github.com/EmbarkStudios/toml-span/releases/tag/0.1.0
