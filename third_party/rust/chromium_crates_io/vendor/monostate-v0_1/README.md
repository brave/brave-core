Monostate
=========

[<img alt="github" src="https://img.shields.io/badge/github-dtolnay/monostate-8da0cb?style=for-the-badge&labelColor=555555&logo=github" height="20">](https://github.com/dtolnay/monostate)
[<img alt="crates.io" src="https://img.shields.io/crates/v/monostate.svg?style=for-the-badge&color=fc8d62&logo=rust" height="20">](https://crates.io/crates/monostate)
[<img alt="docs.rs" src="https://img.shields.io/badge/docs.rs-monostate-66c2a5?style=for-the-badge&labelColor=555555&logo=docs.rs" height="20">](https://docs.rs/monostate)
[<img alt="build status" src="https://img.shields.io/github/actions/workflow/status/dtolnay/monostate/ci.yml?branch=master&style=for-the-badge" height="20">](https://github.com/dtolnay/monostate/actions?query=branch%3Amaster)

This library implements a type macro for a zero-sized type that is Serde
deserializable only from one specific value.

```toml
[dependencies]
monostate = "0.1"
```

<br>

## Examples

```rust
use monostate::MustBe;
use serde::Deserialize;

#[derive(Deserialize)]
struct Example {
    kind: MustBe!("success"),
    code: MustBe!(200),
}
```

The above struct would deserialize from `{"kind":"success", "code":200}` in
JSON, but would fail the deserialization if "kind" or "code" were any other
value.

This can sometimes be helpful in processing untagged enums in which the variant
identification is more convoluted than what is handled by Serde's externally
tagged and internally tagged representations, for example because the variant
tag has an inconsistent type or key.

```rust
use monostate::MustBe;
use serde::Deserialize;

#[derive(Deserialize)]
#[serde(untagged)]
pub enum ApiResponse {
    Success {
        success: MustBe!(true),
    },
    Error {
        kind: MustBe!("error"),
        message: String,
    },
}
```

<br>

#### License

<sup>
Licensed under either of <a href="LICENSE-APACHE">Apache License, Version
2.0</a> or <a href="LICENSE-MIT">MIT license</a> at your option.
</sup>

<br>

<sub>
Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in this crate by you, as defined in the Apache-2.0 license, shall
be dual licensed as above, without any additional terms or conditions.
</sub>
