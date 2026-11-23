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

# Portability

The xxHash algorithms produce consistent output given consistent
input. Inputs to the algorithms include the raw bytes being hashed as
well as any configured seed or secret. The output does not depend on
the platform; 32- and 64-bit systems produce the same output, as do
little- and big-endian systems. The Rust implementation is verified
against the reference C implementation.

The types in this crate implement the [`Hasher`][] trait, used in
conjunction with the [`Hash`][] trait. The `Hash` trait [does **not**
guarantee][hash-port] that implementors feed data into the `Hasher` in
a platform-independent way. Notably, common types like [`Vec<T>`][] /
[`&[T]`](prim@slice) or [`BTreeMap`][] hash their lengths in a
platform-*dependent* manner, producing different results between 32-
and 64-bit systems.

In addition, types from the standard library explicitly do not
guarantee that they will stay consistent from version to version.

If you need a long-term level of consistency for hashing generic
types, you may want to create your own hashing trait where you control
all implementations. You can then implement this trait for all of the
types you need to hash and ensure that platform differences are
handled and stability is maintained over time.

In other cases, it may be enough to write a wrapper around the hasher
that deals with simple platform specifics, such as by adapting
[`Hasher::write_usize`][] to a fixed-size integer.

[`Hasher`]: std::hash::Hasher
[`Hasher::write_usize`]: std::hash::Hasher::write_usize
[hash-port]: std::hash::Hash#portability
[`BTreeMap`]: std::collections::BTreeMap

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
