# zcash_spec

This crate provides low-level types for implementing Zcash specifications. When a common
function defined in [the Zcash Protocol Specification] is used in multiple protocols (for
example the Sapling and Orchard shielded protocols), a corresponding common type in this
crate can be shared between implementations (for example by the [`sapling-crypto`] and
[`orchard`] crates).

[the Zcash Protocol Specification]: https://zips.z.cash/protocol/protocol.pdf
[`sapling-crypto`]: https://github.com/zcash/sapling-crypto
[`orchard`]: https://github.com/zcash/orchard

## License

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or
   http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted for
inclusion in the work by you, as defined in the Apache-2.0 license, shall be dual licensed
as above, without any additional terms or conditions.
