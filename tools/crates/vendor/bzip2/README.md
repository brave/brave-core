# bzip2

[Documentation](https://docs.rs/bzip2)

A streaming compression/decompression library for rust with bindings to `libbz2`.

## Features

By default, `bzip2-rs` attempts to use the system `libbz2`. When `libbz2` is not available, the library 
is built from source. A from source build requires a functional C toolchain for your target, and may not 
work for all targets (in particular webassembly).

*`libbz2-rs-sys`*

Since version 0.5.0, this crate also supports using [libbz2-rs-sys](https://crates.io/crates/libbz2-rs-sys),
a drop-in compatible rust implementation of `libbz2`. With this feature enabled, cross-compilation should work
like any other rust code, and no C toolchain is needed to compile this crate or its dependencies.

```sh
bzip2 = { version = "0.5.1", default-features = false, features = ["libbz2-rs-sys"] }
```

*`static`*

Always build `libbz2` from C source, and statically link it. This flag is only meaningful when `bzip2-sys` is used,
and has no effect when `libbz2-rs-sys` is used as the bzip2 implementation.

## License

This project is licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or
   http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or
   http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in this repository by you, as defined in the Apache-2.0 license, shall be
dual licensed as above, without any additional terms or conditions.
