A Rust implementation of the [xxHash] algorithm.

[![Crates.io][crates-badge]][crates-url]
[![Documentation][docs-badge]][docs-url]
[![Build Status][actions-badge]][actions-url]

[xxHash]: https://github.com/Cyan4973/xxHash

[crates-badge]: https://img.shields.io/crates/v/twox-hash.svg
[crates-url]: https://crates.io/crates/twox-hash
[docs-badge]: https://img.shields.io/docsrs/twox-hash
[docs-url]: https://docs.rs/twox-hash/
[actions-badge]: https://github.com/shepmaster/twox-hash/actions/workflows/ci.yml/badge.svg?branch=main
[actions-url]: https://github.com/shepmaster/twox-hash/actions/workflows/ci.yml?query=branch%3Amain

# Examples

These examples use [`XxHash64`][] but the same ideas can be
used for [`XxHash32`][], [`XxHash3_64`][], or [`XxHash3_128`][].

## Hashing arbitrary data

### When all the data is available at once

```rust
use twox_hash::XxHash64;

let seed = 1234;
let hash = XxHash64::oneshot(seed, b"some bytes");
assert_eq!(0xeab5_5659_a496_d78b, hash);
```

### When the data is streaming

```rust
use std::hash::Hasher as _;
use twox_hash::XxHash64;

let seed = 1234;
let mut hasher = XxHash64::with_seed(seed);
hasher.write(b"some");
hasher.write(b" ");
hasher.write(b"bytes");
let hash = hasher.finish();
assert_eq!(0xeab5_5659_a496_d78b, hash);
```

## In a [`HashMap`][]

### With a default seed

```rust
use std::{collections::HashMap, hash::BuildHasherDefault};
use twox_hash::XxHash64;

let mut hash = HashMap::<_, _, BuildHasherDefault<XxHash64>>::default();
hash.insert(42, "the answer");
assert_eq!(hash.get(&42), Some(&"the answer"));
```

### With a random seed

```rust
use std::collections::HashMap;
use twox_hash::xxhash64;

let mut hash = HashMap::<_, _, xxhash64::RandomState>::default();
hash.insert(42, "the answer");
assert_eq!(hash.get(&42), Some(&"the answer"));
```

### With a fixed seed

```rust
use std::collections::HashMap;
use twox_hash::xxhash64;

let mut hash = HashMap::with_hasher(xxhash64::State::with_seed(0xdead_cafe));
hash.insert(42, "the answer");
assert_eq!(hash.get(&42), Some(&"the answer"));
```

# Feature Flags

| name        | description                                                                                                                   |
|-------------|-------------------------------------------------------------------------------------------------------------------------------|
| xxhash32    | Include the [`XxHash32`][] algorithm                                                                                          |
| xxhash64    | Include the [`XxHash64`][] algorithm                                                                                          |
| xxhash3_64  | Include the [`XxHash3_64`][] algorithm                                                                                        |
| xxhash3_128 | Include the [`XxHash3_128`][] algorithm                                                                                       |
| random      | Create random instances of the hashers                                                                                        |
| serialize   | Serialize and deserialize hasher state with Serde                                                                             |
| std         | Use the Rust standard library. Enable this if you want SIMD support in [`XxHash3_64`][] or [`XxHash3_128`][]                  |
| alloc       | Use the Rust allocator library. Enable this if you want to create [`XxHash3_64`][] or [`XxHash3_128`][]  with dynamic secrets |

# Benchmarks

See benchmarks in the [comparison][] README.

[comparison]: https://github.com/shepmaster/twox-hash/tree/main/comparison

# Contributing

1. Fork it (<https://github.com/shepmaster/twox-hash/fork>)
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Add a failing test.
4. Add code to pass the test.
5. Commit your changes (`git commit -am 'Add some feature'`)
6. Ensure tests pass.
7. Push to the branch (`git push origin my-new-feature`)
8. Create a new Pull Request


[`Hashmap`]: std::collections::HashMap
[`XxHash32`]: crate::XxHash32
[`XxHash64`]: crate::XxHash64
[`XxHash3_64`]: crate::XxHash3_64
[`XxHash3_128`]: crate::XxHash3_128
