# ed25519-dalek [![](https://img.shields.io/crates/v/ed25519-dalek.svg)](https://crates.io/crates/ed25519-dalek) [![](https://docs.rs/ed25519-dalek/badge.svg)](https://docs.rs/ed25519-dalek) [![CI](https://github.com/dalek-cryptography/curve25519-dalek/actions/workflows/ed25519-dalek.yml/badge.svg?branch=main)](https://github.com/dalek-cryptography/curve25519-dalek/actions/workflows/ed25519-dalek.yml)

Fast and efficient Rust implementation of ed25519 key generation, signing, and
verification.

# Use

To import `ed25519-dalek`, add the following to the dependencies section of
your project's `Cargo.toml`:
```toml
ed25519-dalek = "2"
```

# Feature Flags

This crate is `#[no_std]` compatible with `default-features = false`.

| Feature                | Default? | Description |
| :---                   | :---     | :---        |
| `alloc`                | ✓        | When `pkcs8` is enabled, implements `EncodePrivateKey`/`EncodePublicKey` for `SigningKey`/`VerifyingKey`, respectively. |
| `std`                  | ✓        | Implements `std::error::Error` for `SignatureError`. Also enables `alloc`. |
| `zeroize`              | ✓        | Implements `Zeroize` and `ZeroizeOnDrop` for `SigningKey` |
| `rand_core`            |          | Enables `SigningKey::generate` |
| `batch`                |          | Enables `verify_batch` for verifying many signatures quickly. Also enables `rand_core`. |
| `digest`               |          | Enables `Context`, `SigningKey::{with_context, sign_prehashed}` and `VerifyingKey::{with_context, verify_prehashed, verify_prehashed_strict}` for Ed25519ph prehashed signatures |
| `asm`                  |          | Enables assembly optimizations in the SHA-512 compression functions |
| `pkcs8`                |          | Enables [PKCS#8](https://en.wikipedia.org/wiki/PKCS_8) serialization/deserialization for `SigningKey` and `VerifyingKey` |
| `pem`                  |          | Enables PEM serialization support for PKCS#8 private keys and SPKI public keys. Also enables `alloc`. |
| `legacy_compatibility` |          | **Unsafe:** Disables certain signature checks. See [below](#malleability-and-the-legacy_compatibility-feature) |
| `hazmat` |          | **Unsafe:** Exposes the `hazmat` module for raw signing/verifying. Misuse of these functions will expose the private key, as in the [signing oracle attack](https://github.com/MystenLabs/ed25519-unsafe-libs). |

# Major Changes

See [CHANGELOG.md](CHANGELOG.md) for a list of changes made in past version of this crate.

## Breaking Changes in 2.0.0

* Bump MSRV from 1.41 to 1.60.0
* Bump Rust edition
* Bump `signature` dependency to 2.0
* Make `digest` an optional dependency
* Make `zeroize` an optional dependency
* Make `rand_core` an optional dependency
* Adopt [curve25519-backend selection](https://github.com/dalek-cryptography/curve25519-dalek/#backends) over features
* Make all batch verification deterministic remove `batch_deterministic` ([#256](https://github.com/dalek-cryptography/ed25519-dalek/pull/256))
* Remove `ExpandedSecretKey` API ([#205](https://github.com/dalek-cryptography/ed25519-dalek/pull/205))
* Rename `Keypair` → `SigningKey` and `PublicKey` → `VerifyingKey`
* Make `hazmat` feature to expose, `ExpandedSecretKey`, `raw_sign()`, `raw_sign_prehashed()`, `raw_verify()`, and `raw_verify_prehashed()`

# Documentation

Documentation is available [here](https://docs.rs/ed25519-dalek).

# Compatibility Policies

All on-by-default features of this library are covered by [semantic versioning](https://semver.org/spec/v2.0.0.html) (SemVer).
SemVer exemptions are outlined below for MSRV and public API.

## Minimum Supported Rust Version

| Releases | MSRV   |
| :---     | :---   |
| 2.x      | 1.60   |
| 1.x      | 1.41   |

From 2.x and on, MSRV changes will be accompanied by a minor version bump.

## Public API SemVer Exemptions

Breaking changes to SemVer-exempted components affecting the public API will be accompanied by some version bump.

Below are the specific policies:

| Releases | Public API Component(s) | Policy |
| :---     | :---                    | :---   |
| 2.x      | Dependencies `digest`, `pkcs8` and `rand_core` | Minor SemVer bump |

# Safety

`ed25519-dalek` is designed to prevent misuse. Signing is constant-time, all signing keys are zeroed when they go out of scope (unless `zeroize` is disabled), detached public keys [cannot](https://github.com/MystenLabs/ed25519-unsafe-libs/blob/main/README.md) be used for signing, and extra functions like [`VerifyingKey::verify_strict`](#weak-key-forgery-and-verify_strict) are made available to avoid known gotchas.

Further, this crate has no—and in fact forbids—unsafe code. You can opt in to using some highly optimized unsafe code that resides in `curve25519-dalek`, though. See [below](#microarchitecture-specific-backends) for more information on backend selection.

# Performance

Performance is a secondary goal behind correctness, safety, and clarity, but we
aim to be competitive with other implementations.

## Benchmarks

Benchmarks are run using [criterion.rs](https://github.com/japaric/criterion.rs):

```sh
cargo bench --features "batch"
# Uses avx2 or ifma only if compiled for an appropriate target.
export RUSTFLAGS='-C target_cpu=native'
cargo +nightly bench --features "batch"
```

On an Intel 10700K running at stock comparing between the `curve25519-dalek` backends.

| Benchmark                       | u64       | simd +avx2         | fiat               |
| :---                            | :----     | :---               | :---               |
| signing                         | 15.017 µs | 13.906 µs -7.3967% | 15.877 μs +5.7268% |
| signature verification          | 40.144 µs | 25.963 µs -35.603% | 42.118 μs +4.9173% |
| strict signature verification   | 41.334 µs | 27.874 µs -32.660% | 43.985 μs +6.4136% |
| batch signature verification/4  | 109.44 µs | 81.778 µs -25.079% | 117.80 μs +7.6389% |
| batch signature verification/8  | 182.75 µs | 138.40 µs -23.871% | 195.86 μs +7.1737% |
| batch signature verification/16 | 328.67 µs | 251.39 µs -23.744% | 351.55 μs +6.9614% |
| batch signature verification/32 | 619.49 µs | 477.36 µs -23.053% | 669.41 μs +8.0582% |
| batch signature verification/64 | 1.2136 ms | 936.85 µs -22.543% | 1.3028 ms +7.3500% |
| batch signature verification/96 | 1.8677 ms | 1.2357 ms -33.936% | 2.0552 ms +10.039% |
| batch signature verification/128| 2.3281 ms | 1.5795 ms -31.996% | 2.5596 ms +9.9437% |
| batch signature verification/256| 4.1868 ms | 2.8864 ms -31.061% | 4.6494 μs +11.049% |
| keypair generation              | 13.973 µs | 13.108 µs -6.5062% | 15.099 μs +8.0584% |

## Batch Performance

If your protocol or application is able to batch signatures for verification,
the [`verify_batch`][func_verify_batch] function has greatly improved performance.

As you can see, there's an optimal batch size for each machine, so you'll likely
want to test the benchmarks on your target CPU to discover the best size.

## (Micro)Architecture Specific Backends

A _backend_ refers to an implementation of elliptic curve and scalar arithmetic. Different backends have different use cases. For example, if you demand formally verified code, you want to use the `fiat` backend (as it was generated from [Fiat Crypto][fiat]).

Backend selection details and instructions can be found in the [curve25519-dalek docs](https://github.com/dalek-cryptography/curve25519-dalek#backends).

# Contributing

See [CONTRIBUTING.md](../CONTRIBUTING.md)

# Batch Signature Verification

The standard variants of batch signature verification (i.e. many signatures made with potentially many different public keys over potentially many different messages) is available via the `batch` feature. It uses deterministic randomness, i.e., it hashes the inputs (using [`merlin`](https://merlin.cool/), which handles transcript item separation) and uses the result to generate random coefficients. Batch verification requires allocation, so this won't function in heapless settings.

# Validation Criteria

The _validation criteria_ of a signature scheme are the criteria that signatures and public keys must satisfy in order to be accepted. Unfortunately, Ed25519 has some underspecified parts, leading to different validation criteria across implementations. For a very good overview of this, see [Henry's post][validation].

In this section, we mention some specific details about our validation criteria, and how to navigate them.

## Malleability and the `legacy_compatibility` Feature

A signature scheme is considered to produce _malleable signatures_ if a passive attacker with knowledge of a public key _A_, message _m_, and valid signature _σ'_ can produce a distinct _σ'_ such that _σ'_ is a valid signature of _m_ with respect to _A_. A scheme is only malleable if the attacker can do this _without_ knowledge of the private key corresponding to _A_.

`ed25519-dalek` is not a malleable signature scheme.

Some other Ed25519 implementations are malleable, though, such as [libsodium with `ED25519_COMPAT` enabled](https://github.com/jedisct1/libsodium/blob/24211d370a9335373f0715664271dfe203c7c2cd/src/libsodium/crypto_sign/ed25519/ref10/open.c#L30), [ed25519-donna](https://github.com/floodyberry/ed25519-donna/blob/8757bd4cd209cb032853ece0ce413f122eef212c/ed25519.c#L100), [NaCl's ref10 impl](https://github.com/floodyberry/ed25519-donna/blob/8757bd4cd209cb032853ece0ce413f122eef212c/fuzz/ed25519-ref10.c#L4627), and probably a lot more.
If you need to interoperate with such implementations and accept otherwise invalid signatures, you can enable the `legacy_compatibility` flag. **Do not enable `legacy_compatibility`** if you don't have to, because it will make your signatures malleable.

Note: [CIRCL](https://github.com/cloudflare/circl/blob/fa6e0cca79a443d7be18ed241e779adf9ed2a301/sign/ed25519/ed25519.go#L358) has no scalar range check at all. We do not have a feature flag for interoperating with the larger set of RFC-disallowed signatures that CIRCL accepts.

## Weak key Forgery and `verify_strict()`

A _signature forgery_ is what it sounds like: it's when an attacker, given a public key _A_, creates a signature _σ_ and message _m_ such that _σ_ is a valid signature of _m_ with respect to _A_. Since this is the core security definition of any signature scheme, Ed25519 signatures cannot be forged.

However, there's a much looser kind of forgery that Ed25519 permits, which we call _weak key forgery_. An attacker can produce a special public key _A_ (which we call a _weak_ public key) and a signature _σ_ such that _σ_ is a valid signature of _any_ message _m_, with respect to _A_, with high probability. This attack is acknowledged in the [Ed25519 paper](https://ed25519.cr.yp.to/ed25519-20110926.pdf), and caused an exploitable bug in the Scuttlebutt protocol ([paper](https://eprint.iacr.org/2019/526.pdf), section 7.1). The [`VerifyingKey::verify()`][method_verify] function permits weak keys.

We provide [`VerifyingKey::verify_strict`][method_verify_strict] (and [`verify_strict_prehashed`][method_verify_strict_ph]) to help users avoid these scenarios. These functions perform an extra check on _A_, ensuring it's not a weak public key. In addition, we provide the [`VerifyingKey::is_weak`][method_is_weak] to allow users to perform this check before attempting signature verification.

## Batch verification

As mentioned above, weak public keys can be used to produce signatures for unknown messages with high probability. This means that sometimes a weak forgery attempt will fail. In fact, it can fail up to 7/8 of the time. If you call `verify()` twice on the same failed forgery, it will return an error both times, as expected. However, if you call `verify_batch()` twice on two distinct otherwise-valid batches, both of which contain the failed forgery, there's a 21% chance that one fails and the other succeeds.

Why is this? It's because `verify_batch()` does not do the weak key testing of `verify_strict()`, and it multiplies each verification equation by some random coefficient. If the failed forgery gets multiplied by 8, then the weak key (which is a low-order point) becomes 0, and the verification equation on the attempted forgery will succeed.

Since `verify_batch()` is intended to be high-throughput, we think it's best not to put weak key checks in it. If you want to prevent weird behavior due to weak public keys in your batches, you should call [`VerifyingKey::is_weak`][method_is_weak] on the inputs in advance.

[fiat]: https://github.com/mit-plv/fiat-crypto
[validation]: https://hdevalence.ca/blog/2020-10-04-its-25519am
[func_verify_batch]: https://docs.rs/ed25519-dalek/latest/ed25519_dalek/fn.verify_batch.html
[method_verify]: https://docs.rs/ed25519-dalek/latest/ed25519_dalek/struct.VerifyingKey.html#method.verify
[method_verify_strict]: https://docs.rs/ed25519-dalek/latest/ed25519_dalek/struct.VerifyingKey.html#method.verify_strict
[method_verify_strict_ph]: https://docs.rs/ed25519-dalek/latest/ed25519_dalek/struct.VerifyingKey.html#method.verify_strict_prehashed
[method_is_weak]: https://docs.rs/ed25519-dalek/latest/ed25519_dalek/struct.VerifyingKey.html#method.is_weak
