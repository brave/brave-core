<!-- Allow this file to not have a first line heading -->
<!-- markdownlint-disable-file MD041 no-emphasis-as-heading -->

<!-- inline html -->
<!-- markdownlint-disable-file MD033 -->

<div align="center">

# `📇 tame-index`

**Small crate for interacting with [cargo registry indices](https://doc.rust-lang.org/nightly/cargo/reference/registry-index.html)**

[![Embark](https://img.shields.io/badge/embark-open%20source-blueviolet.svg)](https://embark.dev)
[![Embark](https://img.shields.io/badge/discord-ark-%237289da.svg?logo=discord)](https://discord.gg/dAuKfZS)
[![Crates.io](https://img.shields.io/crates/v/tame-index.svg)](https://crates.io/crates/tame-index)
[![Docs](https://docs.rs/tame-index/badge.svg)](https://docs.rs/tame-index)
[![dependency status](https://deps.rs/repo/github/EmbarkStudios/tame-index/status.svg)](https://deps.rs/repo/github/EmbarkStudios/tame-index)
[![Build status](https://github.com/EmbarkStudios/tame-index/workflows/CI/badge.svg)](https://github.com/EmbarkStudios/tame-index/actions)
</div>

## Differences from [`crates-index`][0]

1. The API exposes enough pieces where an alternative git implementation can be used if `gix` is not to your liking.
1. Sparse index support via [`reqwest`](https://crates.io/crates/reqwest) is optional, gated behind the `sparse` feature flag.
1. Local cache files are always supported regardless of features enabled
1. `ComboIndexCache` (local cache files only) and `ComboIndex` (cache + remote capabilities) are provided to wrap git indices, sparse indices, or local registries depending on the the index URL.
1. Functionality for writing cache entries to the local index cache is exposed in the public API
1. [`Local Registry`](https://doc.rust-lang.org/cargo/reference/source-replacement.html#local-registry-sources) support is available behind the `local` feature flag
1. Building of local registries is available behind the `local-builder` feature flag
1. File-based locking compatible with Cargo is available to ensure `tame-index` and Cargo can play nicely together.

## Contributing

[![Contributor Covenant](https://img.shields.io/badge/contributor%20covenant-v1.4-ff69b4.svg)](CODE_OF_CONDUCT.md)

We welcome community contributions to this project.

Please read our [Contributor Guide](CONTRIBUTING.md) for more information on how to get started.
Please also read our [Contributor Terms](CONTRIBUTING.md#contributor-terms) before you make any contributions.

Any contribution intentionally submitted for inclusion in an Embark Studios project, shall comply with the Rust standard licensing model (MIT OR Apache 2.0) and therefore be dual licensed as described below, without any additional terms or conditions:

### License

This contribution is dual licensed under EITHER OF

- Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or <http://www.apache.org/licenses/LICENSE-2.0>)
- MIT license ([LICENSE-MIT](LICENSE-MIT) or <http://opensource.org/licenses/MIT>)

at your option.

For clarity, "your" refers to Embark or any other licensee/user of the contribution.

[0]: https://crates.io/crates/crates-index
