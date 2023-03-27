# blake2s_simd [![GitHub](https://img.shields.io/github/tag/oconnor663/blake2_simd.svg?label=GitHub)](https://github.com/oconnor663/blake2_simd) [![crates.io](https://img.shields.io/crates/v/blake2s_simd.svg)](https://crates.io/crates/blake2s_simd) [![Actions Status](https://github.com/oconnor663/blake2_simd/workflows/tests/badge.svg)](https://github.com/oconnor663/blake2_simd/actions)

An implementation of the BLAKE2s and BLAKE2sp hash functions. See also
[`blake2b_simd`](../blake2b).

This crate includes:

- 100% stable Rust.
- SIMD implementations based on Samuel Neves' [`blake2-avx2`](https://github.com/sneves/blake2-avx2).
  These are very fast. For benchmarks, see [the Performance section of the
  README](https://github.com/oconnor663/blake2_simd#performance).
- Portable, safe implementations for other platforms.
- Dynamic CPU feature detection. Binaries include multiple implementations by default and
  choose the fastest one the processor supports at runtime.
- All the features from the [the BLAKE2 spec](https://blake2.net/blake2.pdf), like adjustable
  length, keying, and associated data for tree hashing.
- `no_std` support. The `std` Cargo feature is on by default, for CPU feature detection and
  for implementing `std::io::Write`.
- Support for computing multiple BLAKE2s hashes in parallel, matching the efficiency of
  BLAKE2sp. See the [`many`](many/index.html) module.

# Example

```
use blake2s_simd::{blake2s, Params};

let expected = "08d6cad88075de8f192db097573d0e829411cd91eb6ec65e8fc16c017edfdb74";
let hash = blake2s(b"foo");
assert_eq!(expected, &hash.to_hex());

let hash = Params::new()
    .hash_length(16)
    .key(b"Squeamish Ossifrage")
    .personal(b"Shaftoe")
    .to_state()
    .update(b"foo")
    .update(b"bar")
    .update(b"baz")
    .finalize();
assert_eq!("28325512782cbf5019424fa65da9a6c7", &hash.to_hex());
```
