# rust-multihash

[![](https://img.shields.io/badge/made%20by-Protocol%20Labs-blue.svg?style=flat-square)](http://ipn.io)
[![](https://img.shields.io/badge/project-multiformats-blue.svg?style=flat-square)](https://github.com/multiformats/multiformats)
[![](https://img.shields.io/badge/freenode-%23ipfs-blue.svg?style=flat-square)](https://webchat.freenode.net/?channels=%23ipfs)
[![](https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square)](https://github.com/RichardLitt/standard-readme)

[![Build Status](https://github.com/multiformats/rust-multihash/workflows/build/badge.svg)](https://github.com/multiformats/rust-multihash/actions)
[![Crates.io](https://img.shields.io/crates/v/multihash?style=flat-square)](https://crates.io/crates/multihash)
[![License](https://img.shields.io/crates/l/multihash?style=flat-square)](LICENSE)
[![Documentation](https://docs.rs/multihash/badge.svg?style=flat-square)](https://docs.rs/multihash)
[![Dependency Status](https://deps.rs/repo/github/multiformats/rust-multihash/status.svg)](https://deps.rs/repo/github/multiformats/rust-multihash)
[![Coverage Status]( https://img.shields.io/codecov/c/github/multiformats/rust-multihash?style=flat-square)](https://codecov.io/gh/multiformats/rust-multihash)

> [multihash](https://github.com/multiformats/multihash) implementation in Rust.

## Table of Contents
  - [Install](#install)
  - [Usage](#usage)
  - [Supported Hash Types](#supported-hash-types)
  - [Maintainers](#maintainers)
  - [Contribute](#contribute)
  - [License](#license)

## Install

First add this to your `Cargo.toml`

```toml
[dependencies]
multihash = "*"
```

Then run `cargo build`.

## MSRV

The minimum supported Rust version for this library is `1.64.0`.
This is only guaranteed without additional features activated.

## Usage

The `multihash` crate exposes a basic data structure for encoding and decoding multihash.
It does not provide any hashing functionality itself.
`Multihash` uses const-generics to define the internal buffer size.
You should set this to the maximum size of the digest you want to support.

```rust
use multihash::Multihash;

const SHA2_256: u64 = 0x12;

fn main() {
	let hash = Multihash::<64>::wrap(SHA2_256, b"my digest");
	println!("{:?}", hash);
}
```

### Using a custom code table

You can derive your own application specific code table using the `multihash-derive` crate.
The `multihash-codetable` provides predefined hasher implementations if you don't want to implement your own.

```rust
use multihash_derive::MultihashDigest;

#[derive(Clone, Copy, Debug, Eq, MultihashDigest, PartialEq)]
#[mh(alloc_size = 64)]
pub enum Code {
    #[mh(code = 0x01, hasher = multihash_codetable::Sha2_256)]
    Foo,
    #[mh(code = 0x02, hasher = multihash_codetable::Sha2_512)]
    Bar,
}

fn main() {
    let hash = Code::Foo.digest(b"my hash");
    println!("{:02x?}", hash);
}
```

## Supported Hash Types

* `SHA1`
* `SHA2-256`
* `SHA2-512`
* `SHA3`/`Keccak`
* `Blake2b-256`/`Blake2b-512`/`Blake2s-128`/`Blake2s-256`
* `Blake3`
* `Strobe`

## Maintainers

Captain: [@dignifiedquire](https://github.com/dignifiedquire).

## Contribute

Contributions welcome. Please check out [the issues](https://github.com/multiformats/rust-multihash/issues).

Check out our [contributing document](https://github.com/multiformats/multiformats/blob/master/contributing.md) for more information on how we work, and about contributing in general. Please be aware that all interactions related to multiformats are subject to the IPFS [Code of Conduct](https://github.com/ipfs/community/blob/master/code-of-conduct.md).

Small note: If editing the README, please conform to the [standard-readme](https://github.com/RichardLitt/standard-readme) specification.


## License

[MIT](LICENSE)
