# Changelog

## [1.1.3](https://github.com/zip-rs/zip2/compare/v1.1.2...v1.1.3) - 2024-04-30

### <!-- 1 -->🐛 Bug Fixes
- Rare bug where find_and_parse would give up prematurely on detecting a false end-of-CDR header

## [1.1.2](https://github.com/Pr0methean/zip/compare/v1.1.1...v1.1.2) - 2024-04-28

### <!-- 1 -->🐛 Bug Fixes
- Alignment was previously handled incorrectly ([#33](https://github.com/Pr0methean/zip/pull/33))

### <!-- 2 -->🚜 Refactor
- deprecate `deflate-miniz` feature since it's now equivalent to `deflate` ([#35](https://github.com/Pr0methean/zip/pull/35))

## [1.1.1]

### Added

- `index_for_name`, `index_for_path`, `name_for_index`: get the index of a file given its path or vice-versa, without
  initializing metadata from the local-file header or needing to mutably borrow the `ZipArchive`.
- `add_symlink_from_path`, `shallow_copy_file_from_path`, `deep_copy_file_from_path`, `raw_copy_file_to_path`: copy a
  file or create a symlink using `AsRef<Path>` arguments

### Changed

- `add_directory_from_path` and `start_file_from_path` are no longer deprecated, and they now normalize `..` as well as
  `.`.

## [1.1.0]

### Added

- Support for decoding LZMA.

### Changed

- Eliminated a custom `AtomicU64` type by replacing it with `OnceLock` in the only place it's used.
- `FileOptions` now has the subtype `SimpleFileOptions` which implements `Copy` but has no extra data.

## [1.0.1]

### Changed

- The published package on crates.io no longer includes the tests or examples.

## [1.0.0]

### Changed

- Now uses boxed slices rather than `String` or `Vec` for metadata fields that aren't likely to grow.

## [0.11.0]

### Added

- Support for `DEFLATE64` (decompression only).
- Support for Zopfli compression levels up to `i64::MAX`.

### Changed

- `InvalidPassword` is now a kind of `ZipError` to eliminate the need for nested `Result` structs.
- Updated dependencies.

## [0.10.3]

### Changed

- Updated dependencies.
- MSRV increased to `1.67`.

### Fixed

- Fixed some rare bugs that could cause panics when trying to read an invalid ZIP file or using an incorrect password.

## [0.10.2]

### Changed

- Where possible, methods are now `const`. This improves performance, especially when reading.

## [0.10.1]

### Changed

- Date and time conversion methods now return `DateTimeRangeError` rather than `()` on error.

## [0.10.0]

### Changed

- Replaces the `flush_on_finish_file` parameter of `ZipWriter::new` and `ZipWriter::Append` with
  a `set_flush_on_finish_file` method.

### Fixed

- Fixes build errors that occur when all default features are disabled.
- Fixes more cases of a bug when ZIP64 magic bytes occur in filenames.

## [0.9.2]

### Added

- `zlib-ng` for fast Deflate compression. This is now the default for compression levels 0-9.
- `chrono` to convert zip::DateTime to and from chrono::NaiveDateTime

## [0.9.1]

### Added

- Zopfli for aggressive Deflate compression.

## [0.9.0]

### Added

 - `flush_on_finish_file` parameter for `ZipWriter`.

## [0.8.3]

### Changed

- Uses the `aes::cipher::KeyInit` trait from `aes` 0.8.2 where appropriate.

### Fixed

- Calling `abort_file()` no longer corrupts the archive if called on a
  shallow copy of a remaining file, or on an archive whose CDR entries are out
  of sequence. However, it may leave an unused entry in the archive.
- Calling `abort_file()` while writing a ZipCrypto-encrypted file no longer
  causes a crash.
- Calling `abort_file()` on the last file before `finish()` no longer produces
  an invalid ZIP file or garbage in the comment.

### Added

- `ZipWriter` methods `get_comment()` and `get_raw_comment()`.

## [0.8.2]

### Fixed

- Fixed an issue where code might spuriously fail during write fuzzing.

### Added

- New method `with_alignment` on `FileOptions`.

## [0.8.1]

### Fixed

- `ZipWriter` now once again implements `Send` if the underlying writer does.

## [0.8.0]

### Deleted

- Methods `start_file_aligned`, `start_file_with_extra_data`, `end_local_start_central_extra_data` and
  `end_extra_data` (see below).

### Changed

- Alignment and extra-data fields are now attributes of [`zip::unstable::write::FileOptions`], allowing them to be
  specified for `add_directory` and `add_symlink`.
- Extra-data fields are now formatted by the `FileOptions` method `add_extra_data`.
- Improved performance, especially for `shallow_copy_file` and `deep_copy_file` on files with extra data.

### Fixed

- Fixes a rare bug where the size of the extra-data field could overflow when `large_file` was set.
- Fixes more cases of a bug when ZIP64 magic bytes occur in filenames.

## [0.7.5]

### Fixed

- Fixed a bug that occurs when ZIP64 magic bytes occur twice in a filename or across two filenames.

## [0.7.4]

### Added

- Added experimental [`zip::unstable::write::FileOptions::with_deprecated_encryption`] API to enable encrypting
  files with PKWARE encryption.

## [0.7.3]

### Fixed

- Fixed a bug that occurs when a filename in a ZIP32 file includes the ZIP64 magic bytes.

## [0.7.2]

### Added

- Method `abort_file` - removes the current or most recently-finished file from the archive.

### Fixed

- Fixed a bug where a file could remain open for writing after validations failed.

## [0.7.1]

### Changed

- Bumped the version number in order to upload an updated README to crates.io.

## [0.7.0]

### Fixed

- Calling `start_file` with invalid parameters no longer closes the `ZipWriter`.
- Attempting to write a 4GiB file without calling `FileOptions::large_file(true)` now removes the file from the archive
  but does not close the `ZipWriter`.
- Attempting to write a file with an unrepresentable or invalid last-modified date will instead add it with a date of
  1980-01-01 00:00:00.

### Added

- Method `is_writing_file` - indicates whether a file is open for writing.

## [0.6.13]

### Fixed

- Fixed a possible bug in deep_copy_file.

## [0.6.12]

### Fixed

- Fixed a Clippy warning that was missed during the last release.

## [0.6.11]

### Fixed

- Fixed a bug that could cause later writes to fail after a `deep_copy_file` call.

## [0.6.10]

### Changed

- Updated dependency versions.

## [0.6.9]

### Fixed

- Fixed an issue that prevented `ZipWriter` from implementing `Send`.

## [0.6.8]

### Added

- Detects duplicate filenames.

### Fixed

- `deep_copy_file` could set incorrect Unix permissions.
- `deep_copy_file` could handle files incorrectly if their compressed size was u32::MAX bytes or less but their
  uncompressed size was not.
- Documented that `deep_copy_file` does not copy a directory's contents.

### Changed

- Improved performance of `deep_copy_file` by using a HashMap and eliminating a redundant search.

## [0.6.7]

### Added

- `deep_copy_file` method: more standards-compliant way to copy a file from within the ZipWriter

## [0.6.6]

### Fixed

- Unused flag `#![feature(read_buf)]` was breaking compatibility with stable compiler.

### Changed

- Updated `aes` dependency to `0.8.2` (https://github.com/zip-rs/zip/pull/354)
- Updated other dependency versions.

## [0.6.5]
### Changed

- Added experimental [`zip::unstable::write::FileOptions::with_deprecated_encryption`] API to enable encrypting files with PKWARE encryption.

### Added

- `shallow_copy_file` method: copy a file from within the ZipWriter


## [0.6.4]

### Changed

- [#333](https://github.com/zip-rs/zip/pull/333): disabled the default features of the `time` dependency, and also `formatting` and `macros`, as they were enabled by mistake.
- Deprecated [`DateTime::from_time`](https://docs.rs/zip/0.6/zip/struct.DateTime.html#method.from_time) in favor of [`DateTime::try_from`](https://docs.rs/zip/0.6/zip/struct.DateTime.html#impl-TryFrom-for-DateTime)
