# challenge-bypass-ristretto [![](https://img.shields.io/crates/v/challenge-bypass-ristretto.svg)](https://crates.io/crates/challenge-bypass-ristretto) [![](https://docs.rs/challenge-bypass-ristretto/badge.svg)](https://docs.rs/challenge-bypass-ristretto) [![Build Status](https://github.com/brave-intl/challenge-bypass-ristretto/workflows/CI/badge.svg)](https://github.com/brave-intl/challenge-bypass-ristretto/actions)

**A rust implemention of the
[privacy pass cryptographic protocol](https://www.petsymposium.org/2018/files/papers/issue3/popets-2018-0026.pdf)
using the [Ristretto group.](https://ristretto.group/)**

This library utilizes the wonderful [curve25519-dalek](https://github.com/dalek-cryptography/curve25519-dalek)
which is a pure-Rust implementation of group operations on Ristretto.

It is only an implementation of the cryptographic protocol,
it does not provide a service or FFI for use by other languages.

**This crate is still a work in progress and is not yet recommended for external use.**

# FFI

This library exposes some functions intended to assist FFI creation but does
not implement a FFI itself.

For FFI see [challenge-bypass-ristretto-ffi](https://github.com/brave-intl/challenge-bypass-ristretto-ffi).

# Blinded Tokens

As originally implemented in the challenge bypass
[server](https://github.com/privacypass/challenge-bypass-server) and
[extension](https://github.com/privacypass/challenge-bypass-extension)
repositories, blinded tokens enable internet users to anonymously
bypass internet challenges (CAPTCHAs).

In this use case, upon completing a CAPTCHA a user is issued tokens which can be
redeemed in place of completing further CAPTCHAs. The issuer
can verify that the tokens are valid but cannot determine which user they
were issued to.

This method of token creation is generally useful as it allows for
authorization in a way that is unlinkable. This library is intended for
use in applications where these combined properties may be useful.

---

A short description of the protocol follows, [a more detailed writeup is also available].

The blinded token protocol has two parties and two stages. A client and
issuer first perform the signing stage, after which the client is
able to derive tokens which can later be used in the redemption phase.

## Signing

The client prepares random tokens, blinds
those tokens such that the issuer cannot determine the original token value,
and sends them to the issuer. The issuer signs the tokens using a secret key
and returns them to the client. The client then reverses the original blind to yield
a signed token.

## Redemption

The client proves the validity of their signed token to the server. The
server marks the token as spent so it cannot be used again.

# Use

**WARNING** this library has not been audited, use at your own risk!

## Example Usage
See [`tests/e2e.rs`].

## Benchmarks

Run `cargo bench`

## Security Contract

This software attempts to ensure the following:

1. The signing server / issuer cannot link the blinded token it sees during
   signing with the token preimage or other info that is used at the time of 
   redemption.
1. The client cannot create a valid signed token without performing the VOPRF
   protocol with the server. Each protocol run produces a single valid token
   which cannot be used to create additional valid tokens.

Given that:

1. The client keeps the blind secret, at time of issuance only sends the
   blinded token and at time of redemption only sends the payload, verification
   signature and token preimage. The client verifies the DLEQ proof that tokens
   were signed by a public key which was committed to previously and not a key
   unique to the user. The client ensures that other out of band markers like IP 
   addresses cannot be used to uniquely link issuance and verification.
1. The server keeps the signing key secret. The server marks a token preimage
   as spent after the first successful redemption.

## Features

By default this crate uses `std` and the `u64_backend` of [curve25519-dalek](https://github.com/dalek-cryptography/curve25519-dalek). However it is `no-std` compatible and the other `curve25519-dalek` backends can be selected.

The optional features include `base64` and `serde`.

* `base64` exposes methods for base64 encoding / decoding of the various structures.
* `serde` implements the [serde](https://serde.rs) `Serialize` / `Deserialize` traits.

# Development

Install rust.

## Building

Run `cargo build`

## Testing

Run `cargo test`

[`tests/e2e.rs`]: tests/e2e.rs
[a more detailed writeup is also available]: https://docs.rs/challenge-bypass-ristretto#cryptographic-protocol
