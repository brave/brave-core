# bip0039

[![ga-svg]][ga-url]
[![crates-svg]][crates-url]
[![docs-svg]][docs-url]
[![msrv-svg]][msrv-url]
[![codecov-svg]][codecov-url]
[![deps-svg]][deps-url]

[ga-svg]: https://github.com/koushiro/bip0039/workflows/test/badge.svg
[ga-url]: https://github.com/koushiro/bip0039/actions
[crates-svg]: https://img.shields.io/crates/v/bip0039
[crates-url]: https://crates.io/crates/bip0039
[docs-svg]: https://docs.rs/bip0039/badge.svg
[docs-url]: https://docs.rs/bip0039
[msrv-svg]: https://img.shields.io/badge/rustc-1.56+-blue.svg
[msrv-url]: https://blog.rust-lang.org/2021/10/21/Rust-1.56.0.html
[codecov-svg]: https://img.shields.io/codecov/c/github/koushiro/bip0039
[codecov-url]: https://codecov.io/gh/koushiro/bip0039
[deps-svg]: https://deps.rs/repo/github/koushiro/bip0039/status.svg
[deps-url]: https://deps.rs/repo/github/koushiro/bip0039

Another Rust implementation of [BIP-0039](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki) standard.

## Usage

Generate a random BIP-0039 mnemonic in English.

```rust
use bip0039::{Count, Mnemonic};

/// Generates an English mnemonic with 12 words randomly
let mnemonic = Mnemonic::generate(Count::Words12);
/// Gets the phrase
let phrase = mnemonic.phrase();
/// Generates the HD wallet seed from the mnemonic and the passphrase.
let seed = mnemonic.to_seed("");
```

## Documentation

See documentation and examples at https://docs.rs/bip0039.

## Features

- [x] Support all languages in the [BIP-0039 Word Lists](https://github.com/bitcoin/bips/blob/master/bip-0039/bip-0039-wordlists.md)
  - [x] English
  - [x] Japanese
  - [x] Korean
  - [x] Spanish
  - [x] Chinese (Simplified)
  - [x] Chinese (Traditional)
  - [x] French
  - [x] Italian
  - [x] Czech
  - [x] Portuguese
- [x] Support `no_std` environment

## Alternatives

- [bip39](https://github.com/rust-bitcoin/rust-bip39)
- [tiny-bip39](https://github.com/maciejhirsz/tiny-bip39)

## License

Licensed under either of

- [Apache License, Version 2.0](LICENSE-APACHE)
- [MIT License](LICENSE-MIT)

at your option.

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall be
dual licensed as above, without any additional terms or conditions.
