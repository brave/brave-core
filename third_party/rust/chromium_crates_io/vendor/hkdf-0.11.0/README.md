# RustCrypto: HKDF

[![crate][crate-image]][crate-link]
[![Docs][docs-image]][docs-link]
![Apache2/MIT licensed][license-image]
![Rust Version][rustc-image]
[![Project Chat][chat-image]][chat-link]
[![Build Status][build-image]][build-link]

[HMAC-based Extract-and-Expand Key Derivation Function (HKDF)](https://tools.ietf.org/html/rfc5869) for [Rust](http://www.rust-lang.org/).

Uses the Digest trait which specifies an interface common to digest functions, such as SHA-1, SHA-256, etc.

## Installation

From crates.io:

```toml
[dependencies]
hkdf = "0.7"
```

## Usage

See the example [examples/main.rs](examples/main.rs) or run it with `cargo run --example main`

## Authors

[![Vlad Filippov](https://avatars3.githubusercontent.com/u/128755?s=70)](http://vf.io/) | [![Brian Warner](https://avatars3.githubusercontent.com/u/27146?v=4&s=70)](http://www.lothar.com/blog/) 
---|---
[Vlad Filippov](http://vf.io/) | [Brian Warner](http://www.lothar.com/blog/)

[//]: # (badges)

[crate-image]: https://img.shields.io/crates/v/hkdf.svg
[crate-link]: https://crates.io/crates/hkdf
[docs-image]: https://docs.rs/hkdf/badge.svg
[docs-link]: https://docs.rs/hkdf/
[license-image]: https://img.shields.io/badge/license-Apache2.0/MIT-blue.svg
[rustc-image]: https://img.shields.io/badge/rustc-1.41+-blue.svg
[chat-image]: https://img.shields.io/badge/zulip-join_chat-blue.svg
[chat-link]: https://rustcrypto.zulipchat.com/#narrow/stream/260043-KDFs
[build-image]: https://github.com/RustCrypto/KDFs/workflows/hkdf/badge.svg?branch=master&event=push
[build-link]: https://github.com/RustCrypto/KDFs/actions?query=workflow:hkdf
