# Sinsemilla

Sinsemilla is a collision-resistant hash function and commitment scheme designed
to be efficient in algebraic circuit models that support lookups, such as PLONK
or Halo 2.

The security properties of Sinsemilla are similar to Pedersen hashes; it is not
designed to be used where a random oracle, PRF, or preimage-resistant hash is
required. The only claimed security property of the hash function is
collision-resistance for fixed-length inputs.

Sinsemilla is roughly 4 times less efficient than the algebraic hashes Rescue
and Poseidon inside a circuit, but around 19 times more efficient than Rescue
outside a circuit. Unlike either of these hashes, the collision resistance
property of Sinsemilla can be proven based on cryptographic assumptions that
have been well-established for at least 20 years. Sinsemilla can also be used as
a computationally binding and perfectly hiding commitment scheme.

The general approach is to split the message into k-bit pieces, and for each
piece, select from a table of 2k bases in our cryptographic group. We combine
the selected bases using a double-and-add algorithm. This ends up being provably
as secure as a vector Pedersen hash, and makes advantageous use of the lookup
facility supported by Halo 2.

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
