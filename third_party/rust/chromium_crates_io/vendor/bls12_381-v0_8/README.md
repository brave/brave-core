# bls12_381 [![Crates.io](https://img.shields.io/crates/v/bls12_381.svg)](https://crates.io/crates/bls12_381) #

This crate provides an implementation of the BLS12-381 pairing-friendly elliptic curve construction.

* **This implementation has not been reviewed or audited. Use at your own risk.**
* This implementation targets Rust `1.56` or later.
* This implementation does not require the Rust standard library.
* All operations are constant time unless explicitly noted.

## Features

* `bits` (on by default): Enables APIs for obtaining bit iterators for scalars.
* `groups` (on by default): Enables APIs for performing group arithmetic with G1, G2, and GT.
* `pairings` (on by default): Enables some APIs for performing pairings.
* `alloc` (on by default): Enables APIs that require an allocator; these include pairing optimizations.
* `nightly`: Enables `subtle/nightly` which tries to prevent compiler optimizations that could jeopardize constant time operations. Requires the nightly Rust compiler.
* `experimental`: Enables experimental features. These features have no backwards-compatibility guarantees and may change at any time; users that depend on specific behaviour should pin an exact version of this crate. The current list of experimental features:
  * Hashing to curves ([Internet Draft v12](https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-hash-to-curve-12))

## [Documentation](https://docs.rs/bls12_381)

## Curve Description

BLS12-381 is a pairing-friendly elliptic curve construction from the [BLS family](https://eprint.iacr.org/2002/088), with embedding degree 12. It is built over a 381-bit prime field `GF(p)` with...

* z = `-0xd201000000010000`
* p = (z - 1)<sup>2</sup>(z<sup>4</sup> - z<sup>2</sup> + 1) / 3 + z
	* = `0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab`
* q = z<sup>4</sup> - z<sup>2</sup> + 1
	* = `0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001`

... yielding two **source groups** G<sub>1</sub> and G<sub>2</sub>, each of 255-bit prime order `q`, such that an efficiently computable non-degenerate bilinear pairing function `e` exists into a third **target group** G<sub>T</sub>. Specifically, G<sub>1</sub> is the `q`-order subgroup of E(F<sub>p</sub>) : y<sup>2</sup> = x<sup>3</sup> + 4 and G<sub>2</sub> is the `q`-order subgroup of E'(F<sub>p<sup>2</sup></sub>) : y<sup>2</sup> = x<sup>3</sup> + 4(u + 1) where the extension field F<sub>p<sup>2</sup></sub> is defined as F<sub>p</sub>(u) / (u<sup>2</sup> + 1).

BLS12-381 is chosen so that `z` has small Hamming weight (to improve pairing performance) and also so that `GF(q)` has a large 2<sup>32</sup> primitive root of unity for performing radix-2 fast Fourier transforms for efficient multi-point evaluation and interpolation. It is also chosen so that it exists in a particularly efficient and rigid subfamily of BLS12 curves.

### Curve Security

Pairing-friendly elliptic curve constructions are (necessarily) less secure than conventional elliptic curves due to their small "embedding degree". Given a small enough embedding degree, the pairing function itself would allow for a break in DLP hardness if it projected into a weak target group, as weaknesses in this target group are immediately translated into weaknesses in the source group.

In order to achieve reasonable security without an unreasonably expensive pairing function, a careful choice of embedding degree, base field characteristic and prime subgroup order must be made. BLS12-381 uses an embedding degree of 12 to ensure fast pairing performance but a choice of a 381-bit base field characteristic to yield a 255-bit subgroup order (for protection against [Pollard's rho algorithm](https://en.wikipedia.org/wiki/Pollard%27s_rho_algorithm)) while reaching close to a 128-bit security level.

There are [known optimizations](https://ellipticnews.wordpress.com/2016/05/02/kim-barbulescu-variant-of-the-number-field-sieve-to-compute-discrete-logarithms-in-finite-fields/) of the [Number Field Sieve algorithm](https://en.wikipedia.org/wiki/General_number_field_sieve) which could be used to weaken DLP security in the target group by taking advantage of its structure, as it is a multiplicative subgroup of a low-degree extension field. However, these attacks require an (as of yet unknown) efficient algorithm for scanning a large space of polynomials. Even if the attack were practical it would only reduce security to roughly 117 to 120 bits. (This contrasts with 254-bit BN curves which usually have less than 100 bits of security in the same situation.)

### Alternative Curves

Applications may wish to exchange pairing performance and/or G<sub>2</sub> performance by using BLS24 or KSS16 curves which conservatively target 128-bit security. In applications that need cycles of elliptic curves for e.g. arbitrary proof composition, MNT6/MNT4 curve cycles are known that target the 128-bit security level. In applications that only need fixed-depth proof composition, curves of this form have been constructed as part of Zexe.

## Acknowledgements

Please see `Cargo.toml` for a list of primary authors of this codebase.

## License

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally
submitted for inclusion in the work by you, as defined in the Apache-2.0
license, shall be dual licensed as above, without any additional terms or
conditions.
