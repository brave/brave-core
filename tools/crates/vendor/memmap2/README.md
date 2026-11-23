# memmap2

[![Build Status](https://github.com/RazrFalcon/memmap2-rs/actions/workflows/main.yml/badge.svg)](https://github.com/RazrFalcon/memmap2-rs/actions)
[![crates.io](https://img.shields.io/crates/v/memmap2.svg)](https://crates.io/crates/memmap2)
[![Documentation](https://docs.rs/memmap2/badge.svg)](https://docs.rs/memmap2)
[![MSRV 1.65.0](https://img.shields.io/badge/msrv-1.65.0-dea584.svg?logo=rust)](https://github.com/rust-lang/rust/releases/tag/1.65.0)

A Rust library for cross-platform memory mapped IO.

This is a **fork** of the [memmap](https://github.com/danburkert/memmap-rs)
crate.

## Features

- [x] file-backed memory maps
- [x] anonymous memory maps
- [x] synchronous and asynchronous flushing
- [x] copy-on-write memory maps
- [x] read-only memory maps
- [x] stack support (`MAP_STACK` on Unix)
- [x] executable memory maps
- [x] huge page support (Linux only)

A list of supported/tested targets can be found in
[Actions](https://github.com/RazrFalcon/memmap2-rs/actions).

## License

`memmap2` is primarily distributed under the terms of both the MIT license and
the Apache License (Version 2.0).

See [LICENSE-APACHE](LICENSE-APACHE), [LICENSE-MIT](LICENSE-MIT) for details.

Copyright (c) 2020 Yevhenii Reizner

Copyright (c) 2015 Dan Burkert
