Serde IPLD DAG-CBOR
===================

[![Crates.io](https://img.shields.io/crates/v/serde_ipld_dag_cbor.svg)](https://crates.io/crates/serde_ipld_dagcbor)
[![Documentation](https://docs.rs/serde_ipld_dag_cbor/badge.svg)](https://docs.rs/serde_ipld_dag_cbor)

This is a [Serde] implementation for [DAG-CBOR]. It can be use in conjunction with [ipld-core].

The underlying library for CBOR encoding/decoding is [cbor4ii] and the Serde implementation is also heavily based on their code.

This crate started as a fork of [serde_cbor], thanks everyone involved there.

[Serde]: https://github.com/serde-rs/serde
[DAG-CBOR]: https://ipld.io/specs/codecs/dag-cbor/spec/
[ipld-core]: https://github.com/ipld/rust-ipld-core
[cbor4ii]: https://github.com/quininer/cbor4ii
[serde_cbor]: https://github.com/pyfisch/cbor


Usage
-----

Storing and loading Rust types is easy and requires only
minimal modifications to the program code.

```rust
use serde_derive::{Deserialize, Serialize};
use std::error::Error;
use std::fs::File;
use std::io::BufReader;

// Types annotated with `Serialize` can be stored as DAG-CBOR.
// To be able to load them again add `Deserialize`.
#[derive(Debug, Serialize, Deserialize)]
struct Mascot {
    name: String,
    species: String,
    year_of_birth: u32,
}

fn main() -> Result<(), Box<dyn Error>> {
    let ferris = Mascot {
        name: "Ferris".to_owned(),
        species: "crab".to_owned(),
        year_of_birth: 2015,
    };

    let ferris_file = File::create("examples/ferris.cbor")?;
    // Write Ferris to the given file.
    // Instead of a file you can use any type that implements `io::Write`
    // like a HTTP body, database connection etc.
    serde_ipld_dagcbor::to_writer(ferris_file, &ferris)?;

    let tux_file = File::open("examples/tux.cbor")?;
    let tux_reader = BufReader::new(tux_file);
    // Load Tux from a file.
    // Serde IPLD DAG-CBOR performs roundtrip serialization meaning that
    // the data will not change in any way.
    let tux: Mascot = serde_ipld_dagcbor::from_reader(tux_reader)?;

    println!("{:?}", tux);
    // prints: Mascot { name: "Tux", species: "penguin", year_of_birth: 1996 }

    Ok(())
}
```


Features
--------

### `codec`

The `codec` feature is enabled by default, it provides the `Codec` trait, which enables encoding and decoding independent of the IPLD Codec. The minimum supported Rust version (MSRV) can significantly be reduced to 1.64 by disabling this feature.


### `no-cid-as-bytes`

Sometimes it is desired that a CID is not accidentally deserialized into bytes. This can happen because the intermediate serde data model does not retain enough information to be able to differentiate between a bytes container and a CID container when there is a conflicting choice to be made, as in the case of some enum cases. The `no-cid-as-bytes` feature can be enabled in order to error at runtime in such cases.

The problem with that feature is, that it breaks Serde's derive attributes for [internally tagged enums](https://serde.rs/enum-representations.html#internally-tagged) (`#[serde(tag = "sometag")]`) and [untagged enums](https://serde.rs/enum-representations.html#untagged) (`#serde(untagged)`). If this feature is enabled and you still need similar functionality, you could implement a deserializer manually. Examples of how to do that are in the [enum example](examples/enums.rs).


License
-------

Licensed under either of

 * Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.
