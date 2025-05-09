# [v0.11.1](https://github.com/multiformats/rust-cid/compare/v0.11.0...v0.11.1) (2024-03-01)


### Bug Fixes

* wrong serde-codec feature gate ([#156](https://github.com/multiformats/rust-cid/issues/156)) ([9699963](https://github.com/multiformats/rust-cid/commit/96999637942c4bd4c1d69fa2a8fcfb8c225ff27c))


# [v0.11.0](https://github.com/multiformats/rust-cid/compare/v0.10.1...v0.11.0) (2023-11-14)


### Bug Fixes

* `varint_read_u64` panics on unwrap in `no_std` environment ([#145](https://github.com/multiformats/rust-cid/issues/145)) ([86c7912](https://github.com/multiformats/rust-cid/commit/86c79126d851316350ad106d0df3e4ae69071874))
* features accidentally pull in optional dependency ([#147](https://github.com/multiformats/rust-cid/issues/147)) ([942b70e](https://github.com/multiformats/rust-cid/commit/942b70ebd970b9c4a2f330a109227282e7596d29)), closes [#142](https://github.com/multiformats/rust-cid/issues/142)


### Features

* update to multihash 0.19 ([#140](https://github.com/multiformats/rust-cid/issues/140)) ([27b112d](https://github.com/multiformats/rust-cid/commit/27b112d2e6a8a1532f5a1c4ead2cc2e5a68b5dd5))


### BREAKING CHANGES

* Re-exported multihash changed. The multihash v0.19 release split it into several smaller crates, the `multihash` crate now has less functionality.


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
