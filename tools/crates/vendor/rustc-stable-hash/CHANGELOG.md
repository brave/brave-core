# Unreleased

# 0.1.2

- Fix `<StableSipHasher128 as Hasher>::finish` not being platform agnostic [#12][pr12]

[pr12]: https://github.com/rust-lang/rustc-stable-hash/pull/12

# 0.1.1

- feat: derive `Clone` for `StableHasher` [#11][pr11]

[pr11]: https://github.com/rust-lang/rustc-stable-hash/pull/11

# 0.1.0

- Rename `StableHasherResult` to `FromStableHash` [#8][pr8]
- Use new-type for returned-hash of `SipHasher128`(`Hash`) [#8][pr8]
- Introduce multi hasher support [#8][pr8]
- `StableHasher::finish` now returns a small hash instead of being fatal [#6][pr6]
- Remove `StableHasher::finalize` [#4][pr4]
- Import stable hasher implementation from rustc ([db8aca48129](https://github.com/rust-lang/rust/blob/db8aca48129d86b2623e3ac8cbcf2902d4d313ad/compiler/rustc_data_structures/src/))

[pr8]: https://github.com/rust-lang/rustc-stable-hash/pull/8
[pr6]: https://github.com/rust-lang/rustc-stable-hash/pull/6
[pr4]: https://github.com/rust-lang/rustc-stable-hash/pull/4
