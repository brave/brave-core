strobe-rs
=========

[![CI](https://github.com/rozbb/strobe-rs/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/rozbb/strobe-rs/actions)
[![Version](https://img.shields.io/crates/v/strobe-rs.svg)](https://crates.io/crates/strobe-rs)
[![Docs](https://docs.rs/strobe-rs/badge.svg)](https://docs.rs/strobe-rs)

This is a pure Rust, `no_std` implementation of the [Strobe protocol framework](https://strobe.sourceforge.io/). The designer's description:
> Strobe is a new framework for cryptographic protocols. It can also be used for regular encryption. Its goals are to make cryptographic protocols much simpler to develop, deploy and analyze; and to fit into even tiny IoT devices. To that end, it uses only one block function — Keccak-f — to encrypt and authenticate messages.

This implementation currently only supports Keccak-f\[1600\] (the highest security level) as the internal permutation function.

Example
-------

A simple [example](https://github.com/rozbb/strobe-rs/blob/master/examples/basic.rs) that does authenticated encryption and decryption:

```rust
use strobe_rs::{SecParam, Strobe};

use rand::RngCore;

// NOTE: This is just a simple authenticated encryption scheme. For a robust AEAD construction,
// see the example at https://strobe.sourceforge.io/examples/aead/

fn main() {
    let mut rng = rand::thread_rng();

    // Sender and receiver
    let mut tx = Strobe::new(b"correctnesstest", SecParam::B256);
    let mut rx = Strobe::new(b"correctnesstest", SecParam::B256);

    // Key both sides with a predetermined key
    let k = b"the-combination-on-my-luggage";
    tx.key(k, false);
    rx.key(k, false);

    // Have the transmitter sample and send a nonce (192 bits) in the clear
    let mut nonce = [0u8; 24];
    rng.fill_bytes(&mut nonce);
    rx.recv_clr(&nonce, false);
    tx.send_clr(&nonce, false);

    // Have the transmitter send an authenticated ciphertext (with a 256 bit MAC)
    let orig_msg = b"groceries: kaymac, ajvar, cream, diced onion, red pepper, grilled meat";
    let mut msg_buf = *orig_msg;
    tx.send_enc(&mut msg_buf, false);
    let mut mac = [0u8; 32];
    tx.send_mac(&mut mac, false);

    // Rename for clarity. `msg_buf` has been encrypted in-place.
    let mut ciphertext = msg_buf;

    // Have the receiver receive the ciphertext and MAC
    rx.recv_enc(ciphertext.as_mut_slice(), false);
    let res = rx.recv_mac(&mac);

    // Check that the MAC verifies
    assert!(res.is_ok());
    // Check that the decrypted ciphertext equals the original plaintext
    let round_trip_msg = ciphertext;
    assert_eq!(&round_trip_msg, orig_msg);
}
```

Features
--------

Default features flags: _none_

Feature flag list:

* `std` — Implements `std::error::Error` for `AuthError`.
* `asm` — Enables optimized assembly for the Keccak permutation, if available. Assembly currently only exists for ARMv8.
* `serialize_secret_state` — Implements `serde`'s `Serialize` and `Deserialize` traits for the `Strobe` struct. **SECURITY NOTE**: Serializing Strobe state outputs security sensitive data that MUST be kept private. Treat the data as you would a private encryption/decryption key.

For info on how to omit or include feature flags, see the [cargo docs on features](https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html#choosing-features).

MSRV
----

The current minimum supported Rust version (MSRV) is 1.60.0 (2022-04-04).

Tests
-----

To run tests, execute
```shell
cargo test --features "std"
```

This includes known-answer tests, which test against JSON-encoded test vectors in the [kat/](kat/) directory. To verify these test vectors against the reference Python implementation, `cd` into `kat/`, run `python2 verify_test_vector.py` and follow the included instructions.

Benchmarks
----------

To benchmark, run
```shell
cargo bench
```

This will produce a summary with plots in `target/crieteron/report/index.html`. These won't be very interesting, since almost every function in STROBE has the same runtime.

License
-------

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE))
 * MIT license ([LICENSE-MIT](LICENSE-MIT))

at your option.

Warning
-------

This code has not been audited in any sense of the word. Use at your own discretion.
