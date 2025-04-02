# Uint

## Description

Provides facilities to construct big unsigned integer types which use no allocations (stack-based, fixed bit length).
If you want to use a predefined `U128`, `U256` or `U512` type, take a look at the [`primitive-types`](https://github.com/paritytech/parity-common/tree/master/primitive-types) or [`ethereum-types`](https://github.com/paritytech/parity-common/tree/master/ethereum-types) crate.

The focus on the provided big unsigned integer types is performance and cross-platform availability.
Support a very similar API as the built-in primitive integer types.

## Usage

In your `Cargo.toml` paste

```
uint = "0.8"
```

Import the macro

```
use uint::construct_uint;
```

If you're using pre-edition Rust in your main file

```
#[macro_use]
extern crate uint;
```

Construct your own big unsigned integer type as follows.

```
// U1024 with 1024 bits consisting of 16 x 64-bit words
construct_uint! {
	pub struct U1024(16);
}
```

## Tests

### Basic tests

```
cargo test --release
```

### Basic tests + property tests

```
cargo test --release --features=quickcheck
```

### Benchmark tests

```
cargo bench
```

### Fuzz tests

see fuzz [README.md](fuzz/README.md)

## Crate Features

- `std`: Use Rust's standard library.
	- Enables `byteorder/std`, `rustc-hex/std`
	- Enabled by default.
- `quickcheck`: Enable quickcheck-style property testing
	- Use with `cargo test --release --features=quickcheck`.
- `arbitrary`: Allow for creation of an `uint` object from random unstructured input for use with fuzzers that use the `arbitrary` crate.
	- Disabled by default.
