# Hmac DRBG

[![Build Status](https://travis-ci.org/sorpaas/hmac-drbg-rs.svg?branch=master)](https://travis-ci.org/sorpaas/hmac-drbg-rs)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](./LICENSE)
[![Cargo](https://img.shields.io/crates/v/hmac-drbg.svg)](https://crates.io/crates/hmac-drbg)

Pure Rust [Hmac
DRBG](https://csrc.nist.gov/csrc/media/events/random-number-generation-workshop-2004/documents/hashblockcipherdrbg.pdf)
implementation with support of `no_std`.

## Usage

Add `hmac-drbg = "0.1"` dependency.

```
let mut drbg = HmacDRBG::<Sha256>::new(
    "totally random0123456789".as_bytes(),
    "secret nonce".as_bytes(),
    "my drbg".as_bytes());
assert_eq!(drbg.generate::<U32>(None).as_slice(), read_hex("018ec5f8e08c41e5ac974eb129ac297c5388ee1864324fa13d9b15cf98d9a157").unwrap().as_slice());
```
