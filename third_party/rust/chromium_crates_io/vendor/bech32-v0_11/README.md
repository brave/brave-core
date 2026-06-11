Rust Bech32
===========

[![Docs.rs badge](https://docs.rs/bech32/badge.svg)](https://docs.rs/bech32/)
[![Continuous Integration](https://github.com/rust-bitcoin/rust-bech32/workflows/Continuous%20Integration/badge.svg)](https://github.com/rust-bitcoin/rust-bech32/actions?query=workflow%3A%22Continuous+Integration%22)

Rust implementation of the Bech32 encoding format described in [BIP-0173](https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki)
and Bech32m encoding format described in [BIP-0350](https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki).

You can find some usage examples in the [documentation](https://docs.rs/bech32/).

Bitcoin-specific address encoding is handled by the `bitcoin-bech32` crate.


## MSRV

This library should always compile with any combination of features on **Rust 1.48.0**.


## Githooks

To assist devs in catching errors _before_ running CI we provide some githooks. If you do not
already have locally configured githooks you can use the ones in this repository by running, in the
root directory of the repository:
```
git config --local core.hooksPath githooks/
```

Alternatively add symlinks in your `.git/hooks` directory to any of the githooks we provide.


## Benchmarks

We use a custom Rust compiler configuration conditional to guard the benchmark code. To run the
benchmarks use: `RUSTFLAGS='--cfg=bench' cargo +nightly bench`.


## API changes

All PRs that change the public API of `rust-bech32` must include a patch to the
`api/` text files. For PRs that include API changes, add a separate patch to the PR
that is the diff created by running `contrib/check-for-api-changes.sh`.
