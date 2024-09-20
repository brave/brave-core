# [v0.10.1](https://github.com/multiformats/rust-cid/compare/v0.10.0...v0.10.1) (2023-01-09)


### Bug Fixes

* the arb feature needs more multihash features ([#133](https://github.com/multiformats/rust-cid/issues/133)) ([ceca4d9](https://github.com/multiformats/rust-cid/commit/ceca4d93bd90f8ac30987bcc5814f6a655484787))


# [v0.10.0](https://github.com/multiformats/rust-cid/compare/v0.9.0...v0.10.0) (2022-12-22)


### chore

* upgrade to Rust edition 2021 and set MSRV ([#130](https://github.com/multiformats/rust-cid/issues/130)) ([91fd35e](https://github.com/multiformats/rust-cid/commit/91fd35e06f8ae24d66f6ba4598830d8dbc259c8a))


### Features

* add `encoded_len` and written bytes ([#129](https://github.com/multiformats/rust-cid/issues/129)) ([715771c](https://github.com/multiformats/rust-cid/commit/715771c48fd47969e733ed1faad8b82d9ddbd7ca))


### BREAKING CHANGES

* Return `Result<usize>` (instead of `Result<()>`) now from `Cid::write_bytes`.
* Rust edition 2021 is now used
