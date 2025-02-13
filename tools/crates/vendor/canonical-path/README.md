# canonical-path.rs

[![Crate][crate-image]][crate-link]
[![Docs][docs-image]][docs-link]
[![Apache 2.0 Licensed][license-image]][license-link]
![Rust 1.35+][rustc-image]
[![Build Status][build-image]][build-link]

`std::fs::Path` and `PathBuf`-like types for representing canonical
filesystem paths.

In the same way a `str` "guarantees" a `&[u8]` contains only valid UTF-8 data,
`CanonicalPath` and `CanonicalPathBuf` guarantee that the paths they represent
are canonical, or at least, were canonical at the time they were created.

[Documentation][docs-link]

## Requirements

- Rust 1.35+

## License

Copyright © 2018-2019 iqlusion

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## Contribution

Unless you explicitly state otherwise, any contribution intentionally
submitted for inclusion in the work by you shall be dual licensed as above,
without any additional terms or conditions.

[//]: # (badges)

[crate-image]: https://img.shields.io/crates/v/canonical-path.svg
[crate-link]: https://crates.io/crates/canonical-path
[docs-image]: https://docs.rs/canonical-path/badge.svg
[docs-link]: https://docs.rs/canonical-path/
[license-image]: https://img.shields.io/badge/license-Apache2.0-blue.svg
[license-link]: https://github.com/iqlusioninc/crates/blob/develop/LICENSE
[rustc-image]: https://img.shields.io/badge/rustc-1.35+-blue.svg
[build-image]: https://travis-ci.com/iqlusioninc/crates.svg?branch=develop
[build-link]: https://travis-ci.com/iqlusioninc/crates/
