# halo2_legacy_pdqsort [![Crates.io](https://img.shields.io/crates/v/halo2_legacy_pdqsort.svg)](https://crates.io/crates/halo2_legacy_pdqsort) #

## [Documentation](https://docs.rs/halo2_legacy_pdqsort)

## Description

A copy of the `core::slice::sort` module from the Rust 1.56.1 standard
library, modified to behave the same on 32-bit platforms as on 64-bit.
This is intended to work around a determinism bug in the
[`halo2_proofs`](https://github.com/zcash/halo2) crate.

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
