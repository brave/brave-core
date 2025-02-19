# pairing [![Crates.io](https://img.shields.io/crates/v/pairing.svg)](https://crates.io/crates/pairing) #

`pairing` is a crate for using pairing-friendly elliptic curves.

`pairing` provides basic traits for pairing-friendly elliptic curve constructions.
Specific curves are implemented in separate crates:

- [`bls12_381`](https://crates.io/crates/bls12_381) - the BLS12-381 curve.

## [Documentation](https://docs.rs/pairing/)

Bring the `pairing` crate into your project just as you normally would.

## Security Warnings

This library does not make any guarantees about constant-time operations, memory
access patterns, or resistance to side-channel attacks.

## Minimum Supported Rust Version

Requires Rust **1.56** or higher.

Minimum supported Rust version can be changed in the future, but it will be done with a
minor version bump.

## License

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or
   http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally
submitted for inclusion in the work by you, as defined in the Apache-2.0
license, shall be dual licensed as above, without any additional terms or
conditions.
