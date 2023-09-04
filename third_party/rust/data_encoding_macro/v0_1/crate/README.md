This library provides macros to define compile-time byte slices and arrays from
encoded strings (using common bases like base64, base32, or hexadecimal, and
also custom bases). It also provides a macro to define compile-time custom
encodings to be used with the [data-encoding] crate.

See the [documentation] for more details.

Up to Rust 1.50, you may need to add the following to your `.cargo/config.toml`
to use this library in no-std or no-alloc environments:

```toml
[unstable]
features = ["host_dep"]
```

From Rust 1.51, you may need to add the following to your `Cargo.toml`:

```toml
[package]
resolver = "2"
```

[data-encoding]: https://crates.io/crates/data-encoding
[documentation]: https://docs.rs/data-encoding-macro
