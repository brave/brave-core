# BLS Signatures

[![CircleCI][circleci-shield]][circleci] [![License][license-shield]][license]

> Implementation of BLS signatures in pure Rust.


## Development

### BLST Portability

To enable the portable feature when building blst dependencies, use the 'blst-portable' feature: `--features blst-portable`.

### Tests

```
> cargo test
```

### Benchmarks

```
> cargo bench
```

### Examples

```
# Verify 10,000 aggregated signatures
> cargo run --example verify --release
```

## LICENSE

MIT or Apache 2.0

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in bls-signatures by you, as defined in the Apache-2.0 license, shall be
dual licensed as above, without any additional terms or conditions.

[circleci-shield]: https://img.shields.io/circleci/project/github/filecoin-project/bls-signatures.svg?style=flat-square
[circleci]: https://circleci.com/gh/filecoin-project/bls-signatures
[license-shield]: https://img.shields.io/badge/License-MIT%2FApache2.0-green.svg?style=flat-square
[license]: https://github.com//filecoin-project/bls-signatures/blob/master/README.md#LICENSE
[crate-shield]: https://img.shields.io/crates/v/accumulators.svg?style=flat-square
[crate]: https://crates.io/crates/accumulators
