# 0.8.0 - [2023-11-01]

- Update `asynchronous-codec` to `0.7` (#71)

# 0.7.2 [2023-09-XX]

- Update `tokio-util` to `0.7` (#59)

# 0.7.1 [2021-02-08]

- Allow conversions from crate errors to std::io equivalents (#52).
- Reject non-minimally encoded varints (additional non-significant zero bytes) (#56).

# 0.7.0 [2021-02-04]

- Update `asynchronous-codec` to `v0.6`.

# 0.6.0 [2021-01-11]

- Switch `futures_codec` to `asynchronous-codec`.

- Upgrade dependencies.

# 0.5.1 [2020-09-08]

- Improve docs (#36).

# 0.5.0 [2020-07-31]

- Add support for `no_std` (#32, #33).
  A new feature flag `std` has been added. It is not enabled by default but
  implicitly for `codec`, `futures` and `futures-codec` features. It must
  be explicitly provided in the absence of those features if the `io` module
  should be enabled.

# 0.4.0 [2020-05-18]

- Update `tokio-util` and `futures_codec` (#31).

# 0.3.3 [2020-04-14]

- Optional support for nom has been added (#27).

# 0.3.2 [2020-03-04]

- Replace the optional `futures` dependency with a `futures` feature that
only includes `futures-io` and `futures-util` as dependencies (#26).

# 0.3.1 [2020-02-17]

- Add modules `io` and `aio` to support direct reading of an unsigned-varint
  value from a `std::io::Read` or `futures::io::AsyncRead` type.

# 0.3.0 [2020-01-02]

- Update to `bytes` v0.5.
- Add support for `tokio-util` v0.2.
- Remove support for `tokio-codec` v0.1.
- Use `#[non_exhaustive]` in `decode::Error` and remove `__Nonexhaustive`.

# 0.2.3 [2019-10-07]

- In addition to `tokio-codec`, `futures_codec` is now supported (#18).
- `decode::Error` now implements `Clone` (#19).
- Code quality improvements (#20, #21).

# 0.2.2 [2019-01-31]

- Add package metadata for docs.rs to generate documentation for all features.

# 0.2.1 [2018-09-05]

- Ensure `codec::Uvi<T>` is `Send` when `T` is.

# 0.2.0 [2018-09-03]

- Change default value for `UviBytes::max` from `usize::MAX` to 128 MiB.

# 0.1.0 [2018-08-08]

Initial release
