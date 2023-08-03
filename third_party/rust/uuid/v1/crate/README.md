# `uuid`

[![Latest Version](https://img.shields.io/crates/v/uuid.svg)](https://crates.io/crates/uuid)
![Minimum rustc version](https://img.shields.io/badge/rustc-1.46.0+-yellow.svg)
[![Continuous integration](https://github.com/uuid-rs/uuid/actions/workflows/ci.yml/badge.svg)](https://github.com/uuid-rs/uuid/actions/workflows/ci.yml)

Here's an example of a UUID:

```text
67e55044-10b1-426f-9247-bb680e5fe0c8
```

A UUID is a unique 128-bit value, stored as 16 octets, and regularly
formatted as a hex string in five groups. UUIDs are used to assign unique
identifiers to entities without requiring a central allocating authority.

They are particularly useful in distributed systems, though can be used in
disparate areas, such as databases and network protocols.  Typically a UUID
is displayed in a readable string form as a sequence of hexadecimal digits,
separated into groups by hyphens.

The uniqueness property is not strictly guaranteed, however for all
practical purposes, it can be assumed that an unintentional collision would
be extremely unlikely.

## Getting started

Add the following to your `Cargo.toml`:

```toml
[dependencies.uuid]
version = "1.3.0"
features = [
    "v4",                # Lets you generate random UUIDs
    "fast-rng",          # Use a faster (but still sufficiently random) RNG
    "macro-diagnostics", # Enable better diagnostics for compile-time UUIDs
]
```

When you want a UUID, you can generate one:

```rust
use uuid::Uuid;

let id = Uuid::new_v4();
```

If you have a UUID value, you can use its string literal form inline:

```rust
use uuid::{uuid, Uuid};

const ID: Uuid = uuid!("67e55044-10b1-426f-9247-bb680e5fe0c8");
```

You can also parse UUIDs without needing any crate features:

```rust
use uuid::{Uuid, Version};

let my_uuid = Uuid::parse_str("67e55044-10b1-426f-9247-bb680e5fe0c8")?;

assert_eq!(Some(Version::Random), my_uuid.get_version());
```

If you'd like to parse UUIDs _really_ fast, check out the [`uuid-simd`](https://github.com/nugine/uuid-simd)
library.

For more details on using `uuid`, [see the library documentation](https://docs.rs/uuid/1.3.0/uuid).

## Minimum Supported Rust Version (MSRV)

The minimum supported Rust version for `uuid` is documented in
CI. It may be bumped in minor releases as necessary.

## References

* [`uuid` library docs](https://docs.rs/uuid/1.3.0/uuid).
* [Wikipedia: Universally Unique Identifier](http://en.wikipedia.org/wiki/Universally_unique_identifier).
* [RFC4122: A Universally Unique IDentifier (UUID) URN Namespace](http://tools.ietf.org/html/rfc4122).

---
# License

Licensed under either of

* Apache License, Version 2.0, (LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0)
* MIT license (LICENSE-MIT or https://opensource.org/licenses/MIT)

at your option.


[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fuuid-rs%2Fuuid.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fuuid-rs%2Fuuid?ref=badge_large)

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall
be dual licensed as above, without any additional terms or conditions.