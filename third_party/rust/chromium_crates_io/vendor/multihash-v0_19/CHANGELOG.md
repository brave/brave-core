
## [](https://github.com/multiformats/rust-multihash/compare/v0.19.1...v0.19.2) (2023-10-23)

### Dependency Updates

- `multihash` 0.19.2
  - update `unsigned-varint`
- `codetable` 0.1.4
  - update `strobe`
- `multihash-derive` 0.9.1
  - update `multihash-derive-impl`
- `multihash-derive-impl` 0.9.1
  - remove `proc-macro-error` dependency
  - update `synstructure`
  - update `syn` to v2

## [](https://github.com/multiformats/rust-multihash/compare/v0.19.0...v0.19.1) (2023-09-06)

### Bug Fixes

* make Serde (de)serialization no_std compatible ([#337](https://github.com/multiformats/rust-multihash/issues/337)) ([7ad5161](https://github.com/multiformats/rust-multihash/commit/7ad51614ad347bfa8c6f421986abc517e04091f6)), closes 336[#](https://github.com/multiformats/rust-multihash/issues/336). This was a regression introduced in v0.19.0.


## [](https://github.com/multiformats/rust-multihash/compare/v0.18.0...v0.19.0) (2023-06-06)


### ⚠ BREAKING CHANGES

* the Serde serialization format changed
* split crates into multiple to isolate breaking changes
* `identity` hasher was removed

See the migration section below for help on upgrading.

### Features

* **codetable:** remove `identity` hasher ([#289](https://github.com/multiformats/rust-multihash/issues/289)) ([8473e2f](https://github.com/multiformats/rust-multihash/commit/8473e2f7ecdc0838a3f35d0ecb1935b4c70797c2))
* Serde serialize Multihash in bytes representation ([#302](https://github.com/multiformats/rust-multihash/issues/302)) ([1023226](https://github.com/multiformats/rust-multihash/commit/10232266c01aa83190af62ad6aeebf63bb7a16c7))


### Bug Fixes

* avoid possible panic in error handling code ([#277](https://github.com/multiformats/rust-multihash/issues/277)) ([5dc1dfa](https://github.com/multiformats/rust-multihash/commit/5dc1dfac0235e63e9ad80572e6b73f8fcd301ec3))
* don't panic on non minimal varints ([#291](https://github.com/multiformats/rust-multihash/issues/291)) ([6ef6040](https://github.com/multiformats/rust-multihash/commit/6ef604012b84d5c15d4f3c66a28ead96afedf158)), closes [#282](https://github.com/multiformats/rust-multihash/issues/282)
* expose `MultihashDigest` trait in codetable ([#304](https://github.com/multiformats/rust-multihash/issues/304)) ([50b43cd](https://github.com/multiformats/rust-multihash/commit/50b43cdbba5492923ffb31bb197930d2f3e2cf14))


### Code Refactoring

* split crates into multiple to isolate breaking changes ([#272](https://github.com/multiformats/rust-multihash/issues/272)) ([954e523](https://github.com/multiformats/rust-multihash/commit/954e5233d273a2b7d682fd087178203628d131a4))

### Migrating

When upgrading to `v0.19`, consider the following:

- `Code` has moved from `multihash::Code` to `multihash_codetable::Code`. It's strongly recommended to define your own code table using `multihash_derive`. Check the [custom codetable example](codetable/examples/custom_table.rs) on how to use it. For the simplest migration, use the `multihash_codetable::Code`.

  **Before**

  ```rust
  use multihash::{Code, MultihashDigest};

  fn main() {
      let hash = Code::Sha2_256.digest(b"hello, world!");
      println!("{:?}", hash);
  }
  ```

  **After**

  ```rust
  use multihash_codetable::{Code, MultihashDigest};

  fn main() {
      let hash = Code::Sha2_256.digest(b"hello, world!");
      println!("{:?}", hash);
  }
  ```

  If you get compile errors, make sure you have the correct features enabled. In this case it would be the `sha2` and `digest` features.

- `multihash::Multihash` now requires the size of its internal buffer as a const-generic.
  You can migrate your existing code by defining the following type-alias:

  ```rust
  type Multihash = multihash::Multihash<64>;
  ```

- The `identity` hasher has been removed completely.

  **Before**

  ```rust
  use multihash::{Code, MultihashDigest};

  fn main() {
      let hash = Code::Identity.digest(b"hello, world!");
      println!("{:?}", hash);
  }
  ```

  **After**

  ```rust
  use multihash::Multihash;

  const IDENTITY_HASH_CODE: u64 = 0x00;

  fn main() {
      let hash = Multihash::<64>::wrap(IDENTITY_HASH_CODE, b"hello, world!").unwrap();
      println!("{:?}", hash);
  }
  ```

  Check the [identity example](examples/identity.rs) for more information on how to replicate the functionality.


## [v0.18.1](https://github.com/multiformats/rust-multihash/compare/v0.18.0...v0.18.1) (2023-04-14)


### Bug Fixes

* don't panic on non minimal varints ([#293](https://github.com/multiformats/rust-multihash/issues/293)) ([c3445fc](https://github.com/multiformats/rust-multihash/commit/c3445fc5041b0fc573945321ebd4b0cdffe0daa5)), closes [#282](https://github.com/multiformats/rust-multihash/issues/282)


## [0.18.0](https://github.com/multiformats/rust-multihash/compare/v0.17.0...v0.18.0) (2022-12-06)


### ⚠ BREAKING CHANGES

* update to Rust edition 2021
* `Multihash::write()` returns bytes written

    Prior to this change it returned an empty tuple `()`, now it returns
the bytes written.

### Features

* add `encoded_len` and bytes written ([#252](https://github.com/multiformats/rust-multihash/issues/252)) ([b3cc43e](https://github.com/multiformats/rust-multihash/commit/b3cc43ecb6f9c59da774b094853d6542430d55ad))


### Bug Fixes

* remove Nix support ([#254](https://github.com/multiformats/rust-multihash/issues/254)) ([ebf57dd](https://github.com/multiformats/rust-multihash/commit/ebf57ddb82be2d2fd0a2f00666b0f888d4c78e1b)), closes [#247](https://github.com/multiformats/rust-multihash/issues/247)
* update to Rust edition 2021 ([#255](https://github.com/multiformats/rust-multihash/issues/255)) ([da53376](https://github.com/multiformats/rust-multihash/commit/da53376e0d9cf2d82d6c0d10590a77991cb3a6b6))

