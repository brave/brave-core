[![Build Status](https://github.com/jni-rs/jni-rs/actions/workflows/ci.yml/badge.svg)](https://github.com/jni-rs/jni-rs/actions)
[![Docs](https://docs.rs/jni/badge.svg)](https://docs.rs/jni)
[![Crates.io](https://img.shields.io/crates/v/jni.svg)](https://crates.io/crates/jni)

# JNI Bindings for Rust

This project provides complete JNI bindings for Rust, allowing to:

- Implement native Java methods for JVM and Android in Rust
- Call Java code from Rust
- Embed JVM in Rust applications and use any Java libraries

See the [docs](https://docs.rs/jni) for more details.
See the [examples](https://github.com/jni-rs/jni-rs/tree/master/crates/jni/examples) directory.

## Shared-library Example

```shell
cd crates/jni/mylib-example
make
```

## Contribution

See the [Contribution Guide](CONTRIBUTING.md) for details.

## License

Licensed under either of

* Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or https://www.apache.org/licenses/LICENSE-2.0)
* MIT license ([LICENSE-MIT](LICENSE-MIT) or https://opensource.org/license/mit)

at your option.
