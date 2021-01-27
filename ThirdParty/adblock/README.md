# adblock-rust

This folder contains a precompiled `xcframework` of `adblock-rust-ffi`.

### Wrapper

The `AdblockRust` contains Swift code that wraps the `adblock-rust` library. If more of the `adblock-rust` header is needed to be used, you must update the Swift wrapper to use it in `Client`.

### Updating

To update this library, ensure you have **rust 1.47.0** installed. Any version higher than 1.47.0 as of adblock-rust 0.2.10 will cause a runtime panic and requires an update to the library.

To setup rust, first [install `rustup`](https://rustup.rs), then run `rustup default 1.47.0`, and then finally ensure you install `cbindgen` and `cargo lipo` by running:

```
cargo install cbindgen
cargo install cargo-lipo
```

### Version

To check what version this framework is made from, check the `VERSION` file which includes the version and SHA id from `adblock-rust-ffi`