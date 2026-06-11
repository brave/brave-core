# `#[visibility::make]`

[![Repository](https://img.shields.io/badge/repository-GitHub-brightgreen.svg)](https://github.com/danielhenrymantilla/visibility.rs)
[![Latest version](https://img.shields.io/crates/v/visibility.svg)](https://crates.io/crates/visibility)
[![Documentation](https://docs.rs/visibility/badge.svg)](https://docs.rs/visibility)
[![MSRV](https://img.shields.io/badge/MSRV-1.56.0-white)](https://gist.github.com/danielhenrymantilla/8e5b721b3929084562f8f65668920c33)
[![unsafe forbidden](https://img.shields.io/badge/unsafe-forbidden-success.svg)](https://github.com/rust-secure-code/safety-dance/)
[![License](https://img.shields.io/crates/l/visibility.svg)](https://github.com/danielhenrymantilla/visibility.rs/blob/master/LICENSE)
[![CI](https://github.com/danielhenrymantilla/visibility.rs/workflows/CI/badge.svg)](https://github.com/danielhenrymantilla/visibility.rs/actions)

#### Attribute to override the visibility of items (especially useful in conjunction with `#[cfg_attr(…)]`).

Since it is currently not possible to conditionally modify the visibility of an
item, but since it is possible to conditionally apply an attribute, this crate
features a trivial attribute that modifies the visibility of the decorated item.
This way, by conditionally applying it, one can achieve the desired goal:

## Example

```rust
/// Some fancy docs.
///
/// ## Example
///
/// ```rust
/// ::my_crate::module::foo();
/// ```
// Assuming `cargo test --doc --features integration-tests` is run:
#[cfg_attr(feature = "integration-tests", visibility::make(pub))]
mod module {
    pub fn foo() {}
}
```
