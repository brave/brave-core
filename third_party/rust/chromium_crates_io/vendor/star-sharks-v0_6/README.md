# Sharks

[![Build](https://github.com/brave/sta-rs/workflows/Tests/badge.svg?branch=main)](https://github.com/brave/sta-rs/actions)
[![Crates](https://img.shields.io/crates/v/star-sharks.svg)](https://crates.io/crates/star-sharks)
[![Docs](https://docs.rs/star-sharks/badge.svg)](https://docs.rs/star-sharks)

Fast, small and secure [Shamir's Secret Sharing](https://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing) library in Rust.
This is a fork of the original [sharks crate](https://crates.io/crates/sharks)
used with the STAR protocol.

Documentation:
-    [API reference (docs.rs)](https://docs.rs/star-sharks)

## Usage

Add this to your `Cargo.toml`:

```toml
[dependencies]
star-sharks = "0.6"
```

If your environment doesn't support `std`:

```toml
[dependencies]
star-sharks = { version = "0.6", default-features = false }
```

To get started using Sharks, see the [reference docs](https://docs.rs/star-sharks)

## Features

### Developer friendly
The API is simple and to the point, with minimal configuration.

### Fast and small
The code is as idiomatic and clean as possible, with minimum external dependencies.

### Secure by design
The implementation forbids the user to choose parameters that would result in an insecure application,
like generating more shares than what's allowed by the finite field length.

This implementation uses a [Sophie Germain prime](https://en.wikipedia.org/wiki/Safe_and_Sophie_Germain_primes) (2^128 + 12451).

## Testing

This crate contains both unit and benchmark tests (as well as the examples included in the docs).
You can run them with `cargo test` and `cargo bench`.

# Contributing

If you find a bug or would like a new feature, [open a new issue](https://github.com/brave/sta-rs/issues/new). Please see the [security page](https://github.com/brave/sta-rs/sharks/SECURITY.md) for information on reporting vulnerabilities.

# License

Sharks is distributed under the terms of both the MIT license and the
Apache License (Version 2.0).

See [LICENSE-APACHE](LICENSE-APACHE) and [LICENSE-MIT](LICENSE-MIT), and
[COPYRIGHT](COPYRIGHT) for details.
