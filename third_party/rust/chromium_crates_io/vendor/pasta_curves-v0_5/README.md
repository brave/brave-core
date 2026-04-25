# `pasta_curves`

This crate provides an implementation of the Pasta elliptic curve constructions,
Pallas and Vesta. More details about the Pasta curves can be found
[in this blog post](https://electriccoin.co/blog/the-pasta-curves-for-halo-2-and-beyond/).

## [Documentation](https://docs.rs/pasta_curves)

## Minimum Supported Rust Version

Requires Rust **1.56** or higher.

Minimum supported Rust version can be changed in the future, but it will be done with a
minor version bump.

## Curve Descriptions

- Pallas: y<sup>2</sup> = x<sup>3</sup> + 5 over
  `GF(0x40000000000000000000000000000000224698fc094cf91b992d30ed00000001)`.

- Vesta:  y<sup>2</sup> = x<sup>3</sup> + 5 over
  `GF(0x40000000000000000000000000000000224698fc0994a8dd8c46eb2100000001)`.

The Pasta curves form a cycle with one another: the order of each curve is exactly the
base field of the other. This property is critical to the efficiency of recursive proof
systems. They are designed to be highly 2-adic, meaning that a large power-of-two
multiplicative subgroup exists in each field. This is important for the performance of
polynomial arithmetic over their scalar fields and is essential for protocols similar
to PLONK.

These curves can be reproducibly obtained
[using a curve search utility we’ve published](https://github.com/zcash/pasta).

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
