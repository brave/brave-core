<div align="center">

  <h1>Walrus 🌊🐘</h1>

  <strong>Walrus is a WebAssembly transformation library</strong>

  <p>
    <a href="https://crates.io/crates/walrus"><img src="https://img.shields.io/crates/v/walrus.svg?style=flat-square" alt="Crates.io version" /></a>
    <a href="https://crates.io/crates/walrus"><img src="https://img.shields.io/crates/d/walrus.svg?style=flat-square" alt="Download" /></a>
    <a href="https://docs.rs/walrus"><img src="https://img.shields.io/badge/docs-latest-blue.svg?style=flat-square" alt="docs.rs docs" /></a>
  </p>

  <h3>
    <a href="https://docs.rs/walrus">API Docs</a>
    <span> | </span>
    <a href="https://github.com/rustwasm/walrus/blob/master/CONTRIBUTING.md">Contributing</a>
    <span> | </span>
    <a href="https://discordapp.com/channels/442252698964721669/443151097398296587">Chat</a>
  </h3>

  <sub>Built with 🦀🕸 by <a href="https://rustwasm.github.io/">The Rust and WebAssembly Working Group</a></sub>
</div>

## About

The `walrus` crate is a Rust library for performing WebAssembly transformations
in a robust and ergonomic fashion. The crate is still in its early days but is
currently used to power the [`wasm-bindgen`] CLI tool and its own internal
transformations.

[`wasm-bindgen`]: https://github.com/rustwasm/wasm-bindgen

Using `walrus` will, in the long term, also allow transforming WebAssembly while
preserving DWARF debug information to ensure that debugging the final module is
just as nice as debugging the intermediate module.

Stay tuned for more information in the future!

## Examples

* Check out `examples/build-wasm-from-scratch.rs` for a quick intro to building
  a Wasm module from scratch with `walrus`.
* Check out the [`wasm-snip`](https://github.com/rustwasm/wasm-snip) project for
  a relatively simple and self-contained but still Real World example of using
  `walrus`.

## License

This project is licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or
   http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or
   http://opensource.org/licenses/MIT)

at your option.

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in this project by you, as defined in the Apache-2.0 license,
shall be dual licensed as above, without any additional terms or conditions.
