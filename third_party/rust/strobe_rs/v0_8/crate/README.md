strobe-rs
=========

[![CI](https://github.com/rozbb/strobe-rs/workflows/CI/badge.svg)](https://github.com/rozbb/strobe-rs/actions)
[![Coverage](https://codecov.io/gh/rozbb/strobe-rs/branch/master/graph/badge.svg)](https://codecov.io/gh/rozbb/strobe-rs)
[![Version](https://img.shields.io/crates/v/strobe-rs.svg)](https://crates.io/crates/strobe-rs)
[![Docs](https://docs.rs/strobe-rs/badge.svg)](https://docs.rs/strobe-rs)

This is a relatively barebones, `no_std` implementation of the [Strobe protocol framework][strobe] in pure Rust. It is intended to be used as a library to build other protocols and frameworks. This implementation currently only supports Keccak-f\[1600\] as the internal permutation function, which is the largest possible block size, so big deal.

[strobe]: https://strobe.sourceforge.io/

Example
-------

A simple [program](examples/basic.rs) that encrypts and decrypts a message:

```rust
use strobe_rs::{SecParam, Strobe};

fn main() {
    let mut rx = Strobe::new(b"correctnesstest", SecParam::B256);
    let mut tx = Strobe::new(b"correctnesstest", SecParam::B256);

    rx.key(b"the-combination-on-my-luggage", false);
    tx.key(b"the-combination-on-my-luggage", false);

    let mut msg = b"Attack at dawn".to_vec();
    rx.send_enc(msg.as_mut_slice(), false);

    // Rename for clarity. `msg` has been encrypted in-place.
    let mut ciphertext = msg;

    tx.recv_enc(ciphertext.as_mut_slice(), false);

    // And back again.
    let round_trip_msg = ciphertext;

    assert_eq!(&round_trip_msg, b"Attack at dawn");
}
```

Features
--------

Default features flags: [none]

Feature flag list:

* `std` - Implements `std::error::Error` for `AuthError`.
* `serialize_secret_state` - Implements `serde`'s `Serialize` and `Deserialize` traits for the `Strobe` struct. **SECURITY NOTE**: Serializing Strobe state outputs security sensitive data that MUST be kept private. Treat the data as you would a private encryption/decryption key.

For info on how to omit or include feature flags, see the [cargo docs on features](https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html#choosing-features).

MSRV
----

The current minimum supported Rust version (MSRV) is 1.51.0 (2021-03-25).

Tests
-----

To run tests, execute

    cargo test --all-features

This includes known-answer tests, which test against JSON-encoded test vectors in the [kat/](kat/) directory. To verify these test vectors against the reference Python implementation, `cd` into `kat/`, run `python2 verify_test_vector.py` and follow the included instructions.

Benchmarks
----------

To benchmark, run

    cargo bench

This will produce a summary with plots in `target/crieteron/report/index.html`. These won't be very interesting, since almost every function in  STROBE has the same runtime.

TODO
----

* Contribute an asm impelmentation of Keccak-f\[1600\] to tiny-keccak and expose a feature flag that lets `strobe-rs` users choose which implementation they prefer.

License
-------

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE))
 * MIT license ([LICENSE-MIT](LICENSE-MIT))

at your option.

Warning
-------

This code has not been audited in any sense of the word. Use at your own discretion.
