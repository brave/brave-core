## serde-big-array

[![docs](https://docs.rs/serde-big-array/badge.svg)](https://docs.rs/crate/serde-big-array)
[![crates.io](https://img.shields.io/crates/v/serde-big-array.svg)](https://crates.io/crates/serde-big-array)
[![dependency status](https://deps.rs/repo/github/est31/serde-big-array/status.svg)](https://deps.rs/repo/github/est31/serde-big-array)

Big array helper for serde. The purpose of this crate is to make (de-)serializing arrays of sizes > 32 easy. This solution is needed until [serde adopts const generics support](https://github.com/serde-rs/serde/issues/1937).

Bases on [this](https://github.com/serde-rs/serde/issues/631#issuecomment-322677033) snippet.

```Rust
extern crate serde;
#[macro_use]
extern crate serde_derive;
extern crate serde_json;
#[macro_use]
extern crate serde_big_array;

big_array! { BigArray; }

#[derive(Serialize, Deserialize)]
struct S {
    #[serde(with = "BigArray")]
    arr: [u8; 64],
}

#[test]
fn test() {
    let s = S { arr: [1; 64] };
    let j = serde_json::to_string(&s).unwrap();
    let s_back = serde_json::from_str::<S>(&j).unwrap();
    assert!(&s.arr[..] == &s_back.arr[..]);
}
```

If you enable the `const-generics` feature, you won't have to invoke the `big_array` macro any more:

```Rust
#[macro_use]
extern crate serde_derive;
use serde_big_array::BigArray;

#[derive(Serialize, Deserialize)]
struct S {
    #[serde(with = "BigArray")]
    arr: [u8; 64],
}

#[test]
fn test() {
    let s = S { arr: [1; 64] };
    let j = serde_json::to_string(&s).unwrap();
    let s_back = serde_json::from_str::<S>(&j).unwrap();
    assert!(&s.arr[..] == &s_back.arr[..]);
}
```

Important links:

* Original serde issue [requesting large array support](https://github.com/serde-rs/serde/issues/631)
* [Const generics support issue on serde](https://github.com/serde-rs/serde/issues/1937)
* [serde PR](https://github.com/serde-rs/serde/pull/1860) to add const generics support
* Rust [const generics tracking issue](https://github.com/rust-lang/rust/issues/44580)
* Rust [complex generic constants tracking issue](https://github.com/rust-lang/rust/issues/76560)

### MSRV

The minimum supported Rust version (MSRV) is Rust 1.32.0.

### License
[license]: #license

This crate is distributed under the terms of both the MIT license
and the Apache License (Version 2.0), at your option.

See [LICENSE-APACHE](LICENSE-APACHE) and [LICENSE-MIT](LICENSE-MIT) for details.

#### License of your contributions

Unless you explicitly state otherwise, any contribution intentionally submitted for
inclusion in the work by you, as defined in the Apache-2.0 license,
shall be dual licensed as above, without any additional terms or conditions.
