<!--
	This readme is created with https://github.com/livioribeiro/cargo-readme

	Edit `src/lib.rs` and use `cargo readme > README.md` to update it.
-->

# fs-err

[![Crates.io](https://img.shields.io/crates/v/fs-err.svg)](https://crates.io/crates/fs-err)
[![GitHub Actions](https://github.com/andrewhickman/fs-err/workflows/CI/badge.svg)](https://github.com/andrewhickman/fs-err/actions?query=workflow%3ACI)

fs-err is a drop-in replacement for [`std::fs`][std::fs] that provides more
helpful messages on errors. Extra information includes which operations was
attempted and any involved paths.

## Error Messages

Using [`std::fs`][std::fs], if this code fails:

```rust
let file = File::open("does not exist.txt")?;
```

The error message that Rust gives you isn't very useful:

```txt
The system cannot find the file specified. (os error 2)
```

...but if we use fs-err instead, our error contains more actionable information:

```txt
failed to open file `does not exist.txt`: The system cannot find the file specified. (os error 2)
```

## Usage

fs-err's API is the same as [`std::fs`][std::fs], so migrating code to use it is easy.

```rust
// use std::fs;
use fs_err as fs;

let contents = fs::read_to_string("foo.txt")?;

println!("Read foo.txt: {}", contents);

```

fs-err uses [`std::io::Error`][std::io::Error] for all errors. This helps fs-err
compose well with traits from the standard library like
[`std::io::Read`][std::io::Read] and crates that use them like
[`serde_json`][serde_json]:

```rust
use fs_err::File;

let file = File::open("my-config.json")?;

// If an I/O error occurs inside serde_json, the error will include a file path
// as well as what operation was being performed.
let decoded: Vec<String> = serde_json::from_reader(file)?;

println!("Program config: {:?}", decoded);

```

## Feature flags

* `expose_original_error`: when enabled, the [`Error::source()`](https://doc.rust-lang.org/stable/std/error/trait.Error.html#method.source) method of errors returned by this crate return the original `io::Error`. To avoid duplication in error messages,
  this also suppresses printing its message in their `Display` implementation, so make sure that you are printing the full error chain.
* `debug`: Debug filesystem errors faster by exposing more information. When a filesystem command
  fails, the error message might say "file does not exist." But it won't say **why** it doesn't exist.
  Perhaps the programmer misspelled the filename, perhaps that directory doesn't exist, or if it does,
  but the current user doesn't have permissions to see the contents. This feature analyzes the filesystem
  to output various "facts" that will help a developer debug the root of the current error.
  * Warning: Exposes filesystem metadata. This feature exposes additional metadata about your filesystem
    such as directory contents and permissions, which may be sensitive. Only enable `debug` when
    error messages won't be displayed to the end user, or they have access to filesystem metadata some
    other way.
  * Warning: This may slow down your program. This feature will trigger additional filesystem calls when
    errors occur, which may cause performance issues. Do not use if filesystem errors are common on a
    performance-sensitive "hotpath." Use in scenarios where developer hours are more expensive than
    compute time.
  * To mitigate performance and security concerns, consider only enabling this feature in `dev-dependencies`:
  * Requires Rust 1.79 or later

```toml
[dev-dependencies]
fs-err = { features = ["debug"] }
```

To use with the `tokio` feature, use `debug_tokio`:

```toml
[dependencies]
fs-err = { features = ["debug_tokio", "tokio"] }
```

## Minimum Supported Rust Version

The oldest rust version this crate is tested on is **1.40**.

This crate will generally be conservative with rust version updates. It uses the [`autocfg`](https://crates.io/crates/autocfg) crate to allow wrapping new APIs without incrementing the MSRV.

If the `tokio` feature is enabled, this crate will inherit the MSRV of the selected [`tokio`](https://crates.io/crates/tokio) version.

[std::fs]: https://doc.rust-lang.org/stable/std/fs/
[std::io::Error]: https://doc.rust-lang.org/stable/std/io/struct.Error.html
[std::io::Read]: https://doc.rust-lang.org/stable/std/io/trait.Read.html
[serde_json]: https://crates.io/crates/serde_json

## License

Licensed under either of

* Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or https://www.apache.org/licenses/LICENSE-2.0)
* MIT license ([LICENSE-MIT](LICENSE-MIT) or https://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally
submitted for inclusion in the work by you, as defined in the Apache-2.0
license, shall be dual licensed as above, without any additional terms or
conditions.
