zip
========

[![Build Status](https://github.com/zip-rs/zip2/actions/workflows/ci.yaml/badge.svg)](https://github.com/Pr0methean/zip/actions?query=branch%3Amaster+workflow%3ACI)
[![Crates.io version](https://img.shields.io/crates/v/zip.svg)](https://crates.io/crates/zip)

[Documentation](https://docs.rs/zip/latest/zip/)

Info
----


A zip library for rust which supports reading and writing of simple ZIP files. Formerly hosted at 
https://github.com/zip-rs/zip2.

Supported compression formats:

* stored (i.e. none)
* deflate
* deflate64 (decompression only)
* bzip2
* zstd
* lzma (decompression only)

Currently unsupported zip extensions:

* Multi-disk

Features
--------

The features available are:

* `aes-crypto`: Enables decryption of files which were encrypted with AES. Supports AE-1 and AE-2 methods.
* `deflate`: Enables decompressing the deflate compression algorithm, which is the default for zip files.
* `deflate-zlib`: Enables deflating files with the `zlib` library (used when compression quality is 0..=9).
* `deflate-zlib-ng`: Enables deflating files with the `zlib-ng` library (used when compression quality is 0..=9).
  This is the fastest `deflate` implementation available.
* `deflate-zopfli`: Enables deflating files with the `zopfli` library (used when compression quality is 10..=264). This
  is the most effective `deflate` implementation available.
* `deflate64`: Enables the deflate64 compression algorithm. Only decompression is supported.
* `lzma`: Enables the LZMA compression algorithm. Only decompression is supported.
* `bzip2`: Enables the BZip2 compression algorithm.
* `time`: Enables features using the [time](https://github.com/rust-lang-deprecated/time) crate.
* `chrono`: Enables converting last-modified `zip::DateTime` to and from `chrono::NaiveDateTime`.
* `zstd`: Enables the Zstandard compression algorithm.

By default `aes-crypto`, `deflate`, `deflate-zlib-ng`, `deflate-zopfli`, `bzip2`, `time` and `zstd` are enabled.

The following feature flags are deprecated:

* `deflate-miniz`: Use `flate2`'s default backend for compression. Currently the same as `deflate`.

MSRV
----

Our current Minimum Supported Rust Version is **1.70**. When adding features,
we will follow these guidelines:

- We will always support the latest four minor Rust versions. This gives you a 6
  month window to upgrade your compiler.
- Any change to the MSRV will be accompanied with a **minor** version bump.

Examples
--------

See the [examples directory](examples) for:
   * How to write a file to a zip.
   * How to write a directory of files to a zip (using [walkdir](https://github.com/BurntSushi/walkdir)).
   * How to extract a zip file.
   * How to extract a single file from a zip.
   * How to read a zip from the standard input.
   * How to append a directory to an existing archive

Fuzzing
-------

Fuzzing support is through [cargo fuzz](https://github.com/rust-fuzz/cargo-fuzz). To install cargo fuzz:

```bash
cargo install cargo-fuzz
```

To list fuzz targets:

```bash
cargo +nightly fuzz list
```

To start fuzzing zip extraction:

```bash
cargo +nightly fuzz run fuzz_read
```

To start fuzzing zip creation:

```bash
cargo +nightly fuzz run fuzz_write
```
