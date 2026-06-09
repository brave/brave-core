# corez

Safe, `no_std`-compatible `Read` and `Write` traits for the Zcash ecosystem.

This crate provides a minimal subset of `std::io` that works in `no_std`
environments without any `unsafe` code. When the `std` feature is enabled
(the default), all types are re-exported directly from `std::io`.

## Features

- `std` (default) — re-exports `std::io` types directly
- `alloc` — enables `Write for Vec<u8>`, `Box` forwarding impls, and richer
  error messages

## License

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or
   http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally
submitted for inclusion in the work by you, as defined in the Apache-2.0
license, shall be dual licensed as above, without any additional terms or
conditions.
