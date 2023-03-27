# rust-multibase

[![](https://img.shields.io/badge/made%20by-Protocol%20Labs-blue.svg?style=flat-square)](http://ipn.io)
[![](https://img.shields.io/badge/project-multiformats-blue.svg?style=flat-square)](https://github.com/multiformats/multiformats)
[![](https://img.shields.io/badge/freenode-%23ipfs-blue.svg?style=flat-square)](https://webchat.freenode.net/?channels=%23ipfs)
[![](https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square)](https://github.com/RichardLitt/standard-readme)

[![Build Status](https://github.com/multiformats/rust-multibase/workflows/build/badge.svg)](https://github.com/multiformats/rust-multibase/actions)
[![License](https://img.shields.io/crates/l/multibase?style=flat-square)](LICENSE)
[![Crates.io](https://img.shields.io/crates/v/multibase?style=flat-square)](https://crates.io/crates/multibase)
[![Documentation](https://docs.rs/multibase/badge.svg?style=flat-square)](https://docs.rs/multibase)
[![Dependency Status](https://deps.rs/repo/github/multiformats/rust-multibase/status.svg)](https://deps.rs/repo/github/multiformats/rust-multibase)
[![Coverage Status]( https://img.shields.io/codecov/c/github/multiformats/rust-multibase?style=flat-square)](https://codecov.io/gh/multiformats/rust-multibase)

> [multibase](https://github.com/multiformats/multibase) implementation in Rust.

## Table of Contents

- [Install](#install)
- [Usage](#usage)
- [Maintainers](#maintainers)
- [Contribute](#contribute)
- [License](#license)

## Install

First add this to your `Cargo.toml`

```toml
[dependencies]
multibase = "0.9"
```

For `no_std`
```
[dependencies]
multibase = { version ="0.9", default-features = false }
```

**note**: This crate relies on the [currently unstable](https://github.com/rust-lang/cargo/issues/7915) `host_dep` feature to [compile proc macros with the proper dependencies](https://docs.rs/data-encoding-macro/0.1.10/data_encoding_macro/), thus **requiring nightly rustc** to use.

Then run `cargo build`.

## Usage

```rust
use multibase::Base;

let base64 = multibase::encode(Base::Base64, b"hello world");
let (base, data) = multibase::decode(base64);
```

**Note**: `base32` and `base64` are orders of magnitude faster due to byte alignment. Don't
be surprised if using a different base turns into a performance bottleneck. You
were warned!

## Maintainers

Captain: [@dignifiedquire](https://github.com/dignifiedquire).

## Contribute

Contributions welcome. Please check out [the issues](https://github.com/multiformats/rust-multibase/issues).

Check out our [contributing document](https://github.com/multiformats/multiformats/blob/master/contributing.md) for more information on how we work, and about contributing in general. Please be aware that all interactions related to multiformats are subject to the IPFS [Code of Conduct](https://github.com/ipfs/community/blob/master/code-of-conduct.md).

Small note: If editing the README, please conform to the [standard-readme](https://github.com/RichardLitt/standard-readme) specification.


## License

[MIT](LICENSE) © 2017 Friedel Ziegelmayer
