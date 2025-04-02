# addchain: Rust crate for generating addition chains

## Usage

To find a short addition chain:

```rust
let chain = addchain::find_shortest_chain(num_bigint::BigUint::from(87u32));
```

To build the steps for an addition chain:

```rust
let steps = addchain::build_addition_chain(num_bigint::BigUint::from(87u32));
```

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

