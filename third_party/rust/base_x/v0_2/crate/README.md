# base-x

[![Build Status](https://travis-ci.org/OrKoN/base-x-rs.svg?branch=master)](https://travis-ci.org/OrKoN/base-x-rs)

This is a Rust fork of https://github.com/cryptocoinjs/base-x

**WARNING:** This module is **NOT RFC3548** compliant,  it cannot be used for base16 (hex), base32, or base64 encoding in a standards compliant manner. 

And this my very first Rust project: please review the source code!

## Installation

Add this to `Cargo.toml` file:

```toml
[dependencies]
base-x = "0.2.0"
```

## Usage

```rust
extern crate base_x;

fn main() {
    let decoded = base_x::decode("01", "11111111000000001111111100000000").unwrap();
    let encoded = base_x::encode("01", &decoded);
    assert_eq!(encoded, "11111111000000001111111100000000");
}
```

## Changelog

- 0.2.0

  Breaking change: alphabet has to be provided as an array of bytes instead of a string.

- 0.1.0

  initial version

## Contributors

- [Friedel Ziegelmayer](https://github.com/dignifiedquire)
- [Maciej Hirsz](https://github.com/maciejhirsz)
