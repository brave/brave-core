# RustCrypto: Argon2

[![crate][crate-image]][crate-link]
[![Docs][docs-image]][docs-link]
[![Build Status][build-image]][build-link]
![Apache2/MIT licensed][license-image]
![Rust Version][rustc-image]
[![Project Chat][chat-image]][chat-link]

Pure Rust implementation of the [Argon2] password hashing function.

[Documentation][docs-link]

# About

Argon2 is a memory-hard [key derivation function] chosen as the winner of
the [Password Hashing Competition] in July 2015.

It implements the following three algorithmic variants:

- **Argon2d**: maximizes resistance to GPU cracking attacks
- **Argon2i**: optimized to resist side-channel attacks
- **Argon2id**: (default) hybrid version combining both Argon2i and Argon2d

Support is provided for embedded (i.e. `no_std`) environments, including
ones without `alloc` support.

## Minimum Supported Rust Version

Rust **1.65** or higher.

Minimum supported Rust version can be changed in the future, but it will be
done with a minor version bump.

## SemVer Policy

- All on-by-default features of this library are covered by SemVer
- MSRV is considered exempt from SemVer as noted above

## License

Licensed under either of:

 * [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0)
 * [MIT license](http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall be
dual licensed as above, without any additional terms or conditions.

[//]: # (badges)

[crate-image]: https://buildstats.info/crate/argon2
[crate-link]: https://crates.io/crates/argon2
[docs-image]: https://docs.rs/argon2/badge.svg
[docs-link]: https://docs.rs/argon2/
[license-image]: https://img.shields.io/badge/license-Apache2.0/MIT-blue.svg
[rustc-image]: https://img.shields.io/badge/rustc-1.65+-blue.svg
[chat-image]: https://img.shields.io/badge/zulip-join_chat-blue.svg
[chat-link]: https://rustcrypto.zulipchat.com/#narrow/stream/260046-password-hashes
[build-image]: https://github.com/RustCrypto/password-hashes/workflows/argon2/badge.svg?branch=master&event=push
[build-link]: https://github.com/RustCrypto/password-hashes/actions?query=workflow%3Aargon2

[//]: # (general links)

[Argon2]: https://en.wikipedia.org/wiki/Argon2
[key derivation function]: https://en.wikipedia.org/wiki/Key_derivation_function
[Password Hashing Competition]: https://www.password-hashing.net/
