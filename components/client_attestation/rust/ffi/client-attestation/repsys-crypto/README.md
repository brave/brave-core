## repsys-crypto

The `repsys-crypto` is the crate that implements the crypto primitives of the
privacy-preserving 2PC protocol for equality checks. The crate is used by both
participants (client and server) of the protocol.

The crate relying on primitives from [curve25519_dalek]() and
[elgamal_ristretto](https://github.com/iquerejeta/elgamal) crates.
