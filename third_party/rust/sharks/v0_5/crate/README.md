# Sharks

[![Rust](https://github.com/c0dearm/sharks/workflows/Rust/badge.svg?branch=master)](https://github.com/c0dearm/sharks/actions)
[![Crates](https://img.shields.io/crates/v/sharks.svg)](https://crates.io/crates/sharks)
[![Docs](https://docs.rs/sharks/badge.svg)](https://docs.rs/sharks)
[![Codecov](https://codecov.io/gh/c0dearm/sharks/branch/master/graph/badge.svg)](https://codecov.io/gh/c0dearm/sharks)
[![License](https://camo.githubusercontent.com/47069b7e06b64b608c692a8a7f40bc6915cf629c/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f6c6963656e73652d417061636865322e302532464d49542d626c75652e737667)](https://github.com/c0dearm/sharks/blob/master/COPYRIGHT)

Fast, small and secure [Shamir's Secret Sharing](https://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing) library crate

Documentation:
-    [API reference (docs.rs)](https://docs.rs/sharks)

## Usage

Add this to your `Cargo.toml`:

```toml
[dependencies]
sharks = "0.4"
```

If your environment doesn't support `std`:

```toml
[dependencies]
sharks = { version = "0.4", default-features = false }
```

To get started using Sharks, see the [Rust docs](https://docs.rs/sharks)

## Features

### Developer friendly
The API is simple and to the point, with minimal configuration.

### Fast and small
The code is as idiomatic and clean as possible, with minimum external dependencies.

### Secure by design
The implementation forbids the user to choose parameters that would result in an insecure application,
like generating more shares than what's allowed by the finite field length.

## Limitations

Because the Galois finite field it uses is [GF256](https://en.wikipedia.org/wiki/Finite_field#GF(p2)_for_an_odd_prime_p),
only up to 255 shares can be generated for a given secret. A larger number would be insecure as shares would start duplicating.
Nevertheless, the secret can be arbitrarily long as computations are performed on single byte chunks.

## Testing

This crate contains both unit and benchmark tests (as well as the examples included in the docs).
You can run them with `cargo test` and `cargo bench`.

### Benchmark results [min mean max]

| CPU                                       | obtain_shares_dealer            | step_shares_dealer              | recover_secret                  | share_from_bytes                | share_to_bytes                  |
| ----------------------------------------- | ------------------------------- | ------------------------------- | ------------------------------- | ------------------------------- | ------------------------------- |
| Intel(R) Core(TM) i7-8550U CPU @ 1.80GHz  | [1.4321 us 1.4339 us 1.4357 us] | [1.3385 ns 1.3456 ns 1.3552 ns] | [228.77 us 232.17 us 236.23 us] | [24.688 ns 25.083 ns 25.551 ns] | [22.832 ns 22.910 ns 22.995 ns] |
| Intel(R) Core(TM) i7-8565U CPU @ 1.80GHz  | [1.3439 us 1.3499 us 1.3562 us] | [1.5416 ns 1.5446 ns 1.5481 ns] | [197.46 us 198.37 us 199.22 us] | [20.455 ns 20.486 ns 20.518 ns] | [18.726 ns 18.850 ns 18.993 ns] |
| Apple M1 ARM (Macbook Air)                | [3.3367 us 3.3629 us 3.4058 us] | [741.75 ps 742.65 ps 743.52 ps] | [210.14 us 210.23 us 210.34 us] | [27.567 ns 27.602 ns 27.650 ns] | [26.716 ns 26.735 ns 26.755 ns] |

# Contributing

If you find a vulnerability, bug or would like a new feature, [open a new issue](https://github.com/c0dearm/sharks/issues/new).

To introduce your changes into the codebase, submit a Pull Request.

Many thanks!

# License

Sharks is distributed under the terms of both the MIT license and the
Apache License (Version 2.0).

See [LICENSE-APACHE](LICENSE-APACHE) and [LICENSE-MIT](LICENSE-MIT), and
[COPYRIGHT](COPYRIGHT) for details.
