IPLD core
=========

[![Crates.io](https://img.shields.io/crates/v/ipld-core.svg)](https://crates.io/crates/ipld-core)
[![Documentation](https://docs.rs/ipld-core/badge.svg)](https://docs.rs/ipld-core)

This crate provides core types for interoperating with [IPLD]. Codecs are not part of this crate, they are independent, but rely on `ipld-core`.

The code is based on [libipld-core]. The major difference is that [Serde] is used a lot more for better interoperability with the rest of the Rust ecosystem.


Usage
-----

### Codec independent code

One of the main features of IPLD is that your Codec is independent of the data you are encoding. Hence it's common that you want to have  your code to be independent of a specific code, but rather be generic.

Here's a full example of a function that can encode data with both [serde_ipld_dagcbor] or [serde_ipld_dagjson]:

```rust
use std::str;

use ipld_core::codec::Codec;
use serde::{Deserialize, Serialize};
use serde_ipld_dagcbor::codec::DagCborCodec;
use serde_ipld_dagjson::codec::DagJsonCodec;

#[derive(Deserialize, Serialize)]
struct Tree {
    height: u8,
    age: u8,
}

fn encode_generic<C, T>(value: &T) -> Result<Vec<u8>, C::Error>
where
    C: Codec<T>,
{
    C::encode_to_vec(value)
}

fn main() {
    let tree = Tree {
        height: 12,
        age: 91,
    };

    let cbor_encoded = encode_generic::<DagCborCodec, _>(&tree);
    #[allow(clippy::format_collect)]
    let cbor_hex = cbor_encoded
        .unwrap()
        .iter()
        .map(|byte| format!("{:02x}", byte))
        .collect::<String>();
    // CBOR encoded: https://cbor.nemo157.com/#value=a2666865696768740c63616765185b
    println!("CBOR encoded: https://cbor.nemo157.com/#value={}", cbor_hex);
    let json_encoded = encode_generic::<DagJsonCodec, _>(&tree).unwrap();
    // JSON encoded: {"height":12,"age":91}
    println!("JSON encoded: {}", str::from_utf8(&json_encoded).unwrap());
}
```

### Extracting links

If you are only interested in the links (CIDs) of an encoded IPLD object, then you can extract them them directly with [`Codec::links()`]:

```rust
use ipld_core::{codec::{Codec, Links}, ipld, cid::Cid};
use serde_ipld_dagjson::codec::DagJsonCodec;

fn main() {
    let cid = Cid::try_from("bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy").unwrap();
    let data = ipld!({"some": {"nested": cid}, "or": [cid, cid], "more": true});

    let mut encoded = Vec::new();
    DagJsonCodec::encode(&mut encoded, &data).unwrap();

    let links = DagJsonCodec::links(&encoded).unwrap().collect::<Vec<_>>();
    // Extracted links: [Cid(bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy), Cid(bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy), Cid(bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy)]
    println!("Extracted links: {:?}", links);
}
```


Feature flags
-------------

 - `std` (enabled by default): Makes the error implement `std::error::Error` and the `Codec` trait available.
 - `codec` (enabled by default): Provides the `Codec` trait, which enables encoding and decoding independent of the IPLD Codec. The minimum supported Rust version (MSRV) can significantly be reduced to 1.64 by disabling this feature.
 - `serde`: Enables support for Serde serialization into/deserialization from the `Ipld` enum.
 - `arb`: Enables support for property based testing.


License
-------

Licensed under either of

 * Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or <http://www.apache.org/licenses/LICENSE-2.0>)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or <http://opensource.org/licenses/MIT>)

at your option.

[IPLD]: https://ipld.io/
[libipld-core]: https://crates.io/crates/libipld-core
[Serde]: https://serde.rs/
[serde_ipld_dagcbor]: https://crates.io/crates/serde_ipld_dagcbor
[serde_ipld_dagjson]: https://crates.io/crates/serde_ipld_dagjson
