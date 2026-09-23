# RustSec: Common Vulnerability Scoring System

[![Latest Version][crate-image]][crate-link]
[![Docs][docs-image]][docs-link]
[![Build Status][build-image]][build-link]
[![Safety Dance][safety-image]][safety-link]
![MSRV][rustc-image]
![Apache 2.0 OR MIT licensed][license-image]
[![Project Chat][zulip-image]][zulip-link]

Rust implementation of the [Common Vulnerability Scoring System (Version 3.1 and 4.0) Specification][spec].

[Documentation][docs-link]

## Minimum Supported Rust Version

Rust **1.60** or higher.

Minimum supported Rust version can be changed in the future, but it will be
done with a minor version bump.

## License

Licensed under either of:

- Apache License, Version 2.0 ([LICENSE-APACHE] or <https://www.apache.org/licenses/LICENSE-2.0>)
- MIT license ([LICENSE-MIT] or <https://opensource.org/licenses/MIT>)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you shall be dual licensed as above, without any
additional terms or conditions.

[//]: # (badges)

[crate-image]: https://img.shields.io/crates/v/cvss.svg?logo=rust
[crate-link]: https://crates.io/crates/cvss
[docs-image]: https://docs.rs/cvss/badge.svg
[docs-link]: https://docs.rs/cvss/
[build-image]: https://github.com/RustSec/rustsec/actions/workflows/cvss.yml/badge.svg
[build-link]: https://github.com/RustSec/rustsec/actions/workflows/cvss.yml
[safety-image]: https://img.shields.io/badge/unsafe-forbidden-success.svg
[safety-link]: https://github.com/rust-secure-code/safety-dance/
[rustc-image]: https://img.shields.io/badge/rustc-1.60+-blue.svg
[license-image]: https://img.shields.io/badge/license-Apache2.0%2FMIT-blue.svg
[zulip-image]: https://img.shields.io/badge/zulip-join_chat-blue.svg
[zulip-link]: https://rust-lang.zulipchat.com/#narrow/stream/146229-wg-secure-code/

[//]: # (general links)

[spec]: https://www.first.org/cvss/specification-document
[LICENSE-APACHE]: https://github.com/RustSec/cargo-audit/blob/main/LICENSE-APACHE
[LICENSE-MIT]: https://github.com/RustSec/cargo-audit/blob/main/LICENSE-MIT
