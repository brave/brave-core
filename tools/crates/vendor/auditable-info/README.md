High-level crate to extract the dependency trees embedded in binaries by [`cargo auditable`](https://crates.io/crates/cargo-auditable).

Deserializes them to a JSON string or Rust data structures, at your option.

### Features

 - Binary parsing designed from the ground up for resilience to malicious inputs.
 - 100% memory-safe Rust, including all dependencies. (There is some `unsafe` in `serde_json` and its dependencies, but only in serialization, which isn't used here).
 - Cross-platform, portable, easy to cross-compile. Runs on [any Rust target with `std`](https://doc.rust-lang.org/stable/rustc/platform-support.html).
 - Parses binaries from any supported platform, not just the platform it's running on.
 - Supports setting size limits for both input and output, to protect against [OOMs](https://en.wikipedia.org/wiki/Out_of_memory) and [zip bombs](https://en.wikipedia.org/wiki/Zip_bomb).

### Usage

```rust
// Uses the default limits: 1GiB input file size, 8MiB audit data size
let info = audit_info_from_file(&PathBuf::from("path/to/file"), Default::default())?;
```
Functions to load the data from a `Read` instance or from `&[u8]` are also provided,
see the [documentation](https://docs.rs/auditable-info).

### Alternatives

[`rust-audit-info`](https://crates.io/crates/rust-audit-info) is a command-line interface to this crate.

If you need a lower-level interface than the one provided by this crate,
use the [`auditable-extract`](http://docs.rs/auditable-extract/) and
[`auditable-serde`](http://docs.rs/auditable-serde/) crates.
