# rustc-stable-hash

[![crates.io](https://img.shields.io/crates/v/rustc-stable-hash.svg)](https://crates.io/crates/rustc-stable-hash)
[![Documentation](https://docs.rs/rustc-stable-hash/badge.svg)](https://docs.rs/rustc-stable-hash)

A stable hashing algorithm used by `rustc`: cross-platform, deterministic, not secure.

This crate provides facilities with the `StableHasher` structure to create stable hashers over *unstable* hashers by abstracting over them the handling of endian-ness and the target `usize`/`isize` bit size difference.

Currently, this crate provides it's own implementation of 128-bit `SipHasher`: `SipHasher128`; with `StableSipHasher128` for the stable variant. 

## Usage

```rust
use rustc_stable_hash::hashers::{StableSipHasher128, SipHasher128Hash};
use rustc_stable_hash::FromStableHash;
use std::hash::Hasher;

struct Hash128([u64; 2]);
impl FromStableHash for Hash128 {
    type Hash = SipHasher128Hash;

    fn from(SipHasher128Hash(hash): SipHasher128Hash) -> Hash128 {
        Hash128(hash)
    }
}

let mut hasher = StableSipHasher128::new();
hasher.write_usize(0xFA);

let hash: Hash128 = hasher.finish();
```
