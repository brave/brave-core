# RustSec: `cargo-lock` crate

[![Latest Version][crate-image]][crate-link]
[![Docs][docs-image]][docs-link]
[![Build Status][build-image]][build-link]
[![Safety Dance][safety-image]][safety-link]
![MSRV][rustc-image]
![Apache 2.0 OR MIT licensed][license-image]
[![Project Chat][zulip-image]][zulip-link]

Self-contained [serde]-powered `Cargo.lock` parser/serializer with support
for the V1/V2/V3/V4 formats, as well as optional dependency tree analysis features.
Used by [RustSec].

When the `dependency-tree` feature of this crate is enabled, it supports
computing a directed graph of the dependency tree, modeled using the
[`petgraph`] crate, along with support for printing dependency trees ala
the [`cargo-tree`] crate.

[Documentation][docs-link]

## Minimum Supported Rust Version

Rust **1.70** or higher.

Minimum supported Rust version can be changed in the future, but it will be
accompanied by a minor version bump.

## SemVer Policy

- MSRV is considered exempt from SemVer as noted above
- The `cargo lock` CLI interface is not considered to have a stable interface
  and is also exempted from SemVer. We reserve the right to make substantial
  changes to it at any time (for now)
- The `dependency-tree` feature depends on the pre-1.0 `petgraph` crate.
  We reserve the right to update `petgraph`, however when we do it will be
  accompanied by a minor version bump.

## Command Line Interface

This crate provides a `cargo lock` subcommand which can be installed with:

```text
cargo install cargo-lock --features=cli
```

It supports the following subcommands:

- `list`: list packages in `Cargo.lock`
- `translate`: translate `Cargo.lock` files between the V1 and V2 formats
- `tree`: print a dependency tree from `Cargo.lock` alone

See the [crate documentation][docs-link] for more detailed usage information.

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

[crate-image]: https://img.shields.io/crates/v/cargo-lock?logo=rust
[crate-link]: https://crates.io/crates/cargo-lock
[docs-image]: https://docs.rs/cargo-lock/badge.svg
[docs-link]: https://docs.rs/cargo-lock/
[build-image]: https://github.com/RustSec/rustsec/actions/workflows/cargo-lock.yml/badge.svg
[build-link]: https://github.com/RustSec/rustsec/actions/workflows/cargo-lock.yml
[license-image]: https://img.shields.io/badge/license-Apache2.0%2FMIT-blue.svg
[rustc-image]: https://img.shields.io/badge/rustc-1.70+-blue.svg
[safety-image]: https://img.shields.io/badge/unsafe-forbidden-success.svg
[safety-link]: https://github.com/rust-secure-code/safety-dance/
[zulip-image]: https://img.shields.io/badge/zulip-join_chat-blue.svg
[zulip-link]: https://rust-lang.zulipchat.com/#narrow/stream/146229-wg-secure-code/

[//]: # (general links)

[serde]: https://serde.rs/
[RustSec]: https://rustsec.org/
[`petgraph`]: https://github.com/petgraph/petgraph
[`cargo-tree`]: https://github.com/sfackler/cargo-tree
[LICENSE-APACHE]: https://github.com/RustSec/cargo-lock/blob/main/LICENSE-APACHE
[LICENSE-MIT]: https://github.com/RustSec/cargo-lock/blob/main/LICENSE-MIT
