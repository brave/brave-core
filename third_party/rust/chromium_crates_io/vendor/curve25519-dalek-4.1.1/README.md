
# curve25519-dalek [![](https://buildstats.info/crate/curve25519-dalek)](https://crates.io/crates/curve25519-dalek) [![](https://img.shields.io/docsrs/curve25519-dalek)](https://docs.rs/curve25519-dalek) [![CI](https://github.com/dalek-cryptography/curve25519-dalek/actions/workflows/curve25519-dalek.yml/badge.svg?branch=main)](https://github.com/dalek-cryptography/curve25519-dalek/actions/workflows/curve25519-dalek.yml)

<p align="center">
<img
 alt="dalek-cryptography logo: a dalek with edwards curves as sparkles coming out of its radar-schnozzley blaster thingies"
 width="200px"
 src="https://cdn.jsdelivr.net/gh/dalek-cryptography/curve25519-dalek/docs/assets/dalek-logo-clear.png"/>
</p>

**A pure-Rust implementation of group operations on Ristretto and Curve25519.**

`curve25519-dalek` is a library providing group operations on the Edwards and
Montgomery forms of Curve25519, and on the prime-order Ristretto group.

`curve25519-dalek` is not intended to provide implementations of any particular
crypto protocol.  Rather, implementations of those protocols (such as
[`x25519-dalek`][x25519-dalek] and [`ed25519-dalek`][ed25519-dalek]) should use
`curve25519-dalek` as a library.

`curve25519-dalek` is intended to provide a clean and safe _mid-level_ API for use
implementing a wide range of ECC-based crypto protocols, such as key agreement,
signatures, anonymous credentials, rangeproofs, and zero-knowledge proof
systems.

In particular, `curve25519-dalek` implements Ristretto, which constructs a
prime-order group from a non-prime-order Edwards curve.  This provides the
speed and safety benefits of Edwards curve arithmetic, without the pitfalls of
cofactor-related abstraction mismatches.

# Use

## Stable

To import `curve25519-dalek`, add the following to the dependencies section of
your project's `Cargo.toml`:
```toml
curve25519-dalek = "4"
```

If opting into [SemVer-exempted features](#public-api-semver-exemptions) a range
can be used to scope the tested compatible version range e.g.:
```toml
curve25519-dalek = ">= 4.0, < 4.2"
```

## Feature Flags

| Feature            | Default? | Description |
| :---               |  :---:   | :---        |
| `alloc`            |    ✓     | Enables Edwards and Ristretto multiscalar multiplication, batch scalar inversion, and batch Ristretto double-and-compress. Also enables `zeroize`. |
| `zeroize`          |    ✓     | Enables [`Zeroize`][zeroize-trait] for all scalar and curve point types. |
| `precomputed-tables` |    ✓     | Includes precomputed basepoint multiplication tables. This speeds up `EdwardsPoint::mul_base` and `RistrettoPoint::mul_base` by ~4x, at the cost of ~30KB added to the code size. |
| `rand_core`        |          | Enables `Scalar::random` and `RistrettoPoint::random`. This is an optional dependency whose version is not subject to SemVer. See [below](#public-api-semver-exemptions) for more details. |
| `digest`           |          | Enables `RistrettoPoint::{from_hash, hash_from_bytes}` and `Scalar::{from_hash, hash_from_bytes}`. This is an optional dependency whose version is not subject to SemVer. See [below](#public-api-semver-exemptions) for more details. |
| `serde`            |          | Enables `serde` serialization/deserialization for all the point and scalar types. |
| `legacy_compatibility`|       | Enables `Scalar::from_bits`, which allows the user to build unreduced scalars whose arithmetic is broken. Do not use this unless you know what you're doing. |
| `group`            |          | Enables external `group` and `ff` crate traits |

To disable the default features when using `curve25519-dalek` as a dependency,
add `default-features = false` to the dependency in your `Cargo.toml`. To
disable it when running `cargo`, add the `--no-default-features` CLI flag.

## Major Version API Changes

Breaking changes for each major version release can be found in
[`CHANGELOG.md`](CHANGELOG.md), under the "Breaking changes" subheader. The
latest breaking changes in high level are below:

### Breaking changes in 4.0.0

* Update the MSRV from 1.41 to 1.60
* Provide SemVer policy
* Make `digest` and `rand_core` optional features
* Remove `std` and `nightly` features
* Replace backend selection - See [CHANGELOG.md](CHANGELOG.md) and [backends](#backends)
* Replace methods `Scalar::{zero, one}` with constants `Scalar::{ZERO, ONE}`
* `Scalar::from_canonical_bytes` now returns `CtOption`
* `Scalar::is_canonical` now returns `Choice`
* Remove `Scalar::from_bytes_clamped` and `Scalar::reduce`
* Deprecate and feature-gate `Scalar::from_bits` behind `legacy_compatibility`
* Deprecate `EdwardsPoint::hash_from_bytes` and rename it
  `EdwardsPoint::nonspec_map_to_curve`
* Require including a new trait, `use curve25519_dalek::traits::BasepointTable`
  whenever using `EdwardsBasepointTable` or `RistrettoBasepointTable`

This release also does a lot of dependency updates and relaxations to unblock upstream build issues.

# Backends

Curve arithmetic is implemented and used by one of the following backends:

| Backend  | Selection | Implementation                                                | Bits / Word sizes |
| :---     | :---      | :---                                                          | :---              |
| `serial` | Automatic | An optimized, non-parllel implementation                      | `32` and `64`     |
| `fiat`   | Manual    | Formally verified field arithmetic from [fiat-crypto]         | `32` and `64`     |
| `simd`   | Automatic | Intel AVX2 / AVX512 IFMA accelerated backend                  | `64` only         |

At runtime, `curve25519-dalek` selects an arithmetic backend from the set of backends it was compiled to support. For Intel x86-64 targets, unless otherwise specified, it will build itself with `simd` support, and default to `serial` at runtime if the appropriate CPU features aren't detected. See [SIMD backend] for more details.

In the future, `simd` backend may be extended to cover more instruction sets. This change will be non-breaking as this is considered as implementation detail.

## Manual Backend Override

You can force the crate to compile with specific backend support, e.g., `serial` for x86-64 targets to save code size, or `fiat` to force the runtime to use verified code. To do this, set the environment variable:
```sh
RUSTFLAGS='--cfg curve25519_dalek_backend="BACKEND"'
```
Equivalently, you can write to
`~/.cargo/config`:
```toml
[build]
rustflags = ['--cfg=curve25519_dalek_backend="BACKEND"']
```
More info [here](https://doc.rust-lang.org/cargo/reference/config.html#buildrustflags).

Note for contributors: The target backends are not entirely independent of each
other. The [SIMD backend] directly depends on parts of the serial backend to
function.

## Bits / Word size

`curve25519-dalek` will automatically choose the word size for the `fiat` and
`serial` backends, based on the build target.
For example, building for a 64-bit machine, the default 64 bit word size is
automatically chosen when either the `serial` or `fiat` backend is selected.

In some targets it might be required to override the word size for better
performance.
Backend word size can be overridden for `serial` and `fiat` by setting the
environment variable:
```sh
RUSTFLAGS='--cfg curve25519_dalek_bits="SIZE"'
```
`SIZE` is `32` or `64`. As in the above section, this can also be placed
in `~/.cargo/config`.

Note: The [SIMD backend] requires a word size of 64 bits. Attempting to set bits=32 and backend=`simd` will yield a compile error.

### Cross-compilation

Because backend selection is done by target, cross-compiling will select the correct word size automatically. For example, if a x86-64 Linux machine runs the following commands, `curve25519-dalek` will be compiled with the 32-bit `serial` backend.
```console
$ sudo apt install gcc-multilib # (or whatever package manager you use)
$ rustup target add i686-unknown-linux-gnu
$ cargo build --target i686-unknown-linux-gnu
```

## SIMD backend

The specific SIMD backend (AVX512 / AVX2 / `serial` default) is selected automatically at runtime, depending on the currently available CPU features, and whether Rust nightly is being used for compilation. The precise conditions are specified below.

For a given CPU feature, you can also specify an appropriate `-C target_feature` to build a binary which assumes the required SIMD instructions are always available. Don't do this if you don't have a good reason.

| Backend | `RUSTFLAGS`                               | Requires nightly? |
| :---    | :---                                      | :---              |
| avx2    | `-C target_feature=+avx2`                 | no                |
| avx512  | `-C target_feature=+avx512ifma,+avx512vl` | yes               |

If compiled on a non-nightly compiler, `curve25519-dalek` will not include AVX512 code, and therefore will never select it at runtime.

# Documentation

The semver-stable, public-facing `curve25519-dalek` API is documented [here][docs].

## Building Docs Locally

The `curve25519-dalek` documentation requires a custom HTML header to include
KaTeX for math support. Unfortunately `cargo doc` does not currently support
this, but docs can be built using
```sh
make doc
```
for regular docs, and
```sh
make doc-internal
```
for docs that include private items.

# Maintenance Policies

All on-by-default features of this library are covered by
[semantic versioning][semver] (SemVer). SemVer exemptions are outlined below
for MSRV and public API.

## Minimum Supported Rust Version

| Releases | MSRV   |
| :---     |:-------|
| 4.x      | 1.60.0 |
| 3.x      | 1.41.0 |

From 4.x and on, MSRV changes will be accompanied by a minor version bump.

## Public API SemVer Exemptions

Breaking changes to SemVer-exempted components affecting the public API will be accompanied by
_some_ version bump. Below are the specific policies:

| Releases | Public API Component(s)                         | Policy              |
| :---     | :---                                            | :---                |
| 4.x      | Dependencies `group`, `digest` and `rand_core`  | Minor SemVer bump   |

# Safety

The `curve25519-dalek` types are designed to make illegal states
unrepresentable.  For example, any instance of an `EdwardsPoint` is
guaranteed to hold a point on the Edwards curve, and any instance of a
`RistrettoPoint` is guaranteed to hold a valid point in the Ristretto
group.

All operations are implemented using constant-time logic (no
secret-dependent branches, no secret-dependent memory accesses),
unless specifically marked as being variable-time code.
We believe that our constant-time logic is lowered to constant-time
assembly, at least on `x86_64` targets.

As an additional guard against possible future compiler optimizations,
the `subtle` crate places an optimization barrier before every
conditional move or assignment.  More details can be found in [the
documentation for the `subtle` crate][subtle_doc].

Some functionality (e.g., multiscalar multiplication or batch
inversion) requires heap allocation for temporary buffers.  All
heap-allocated buffers of potentially secret data are explicitly
zeroed before release.

However, we do not attempt to zero stack data, for two reasons.
First, it's not possible to do so correctly: we don't have control
over stack allocations, so there's no way to know how much data to
wipe.  Second, because `curve25519-dalek` provides a mid-level API,
the correct place to start zeroing stack data is likely not at the
entrypoints of `curve25519-dalek` functions, but at the entrypoints of
functions in other crates.

The implementation is memory-safe, and contains no significant
`unsafe` code.  The SIMD backend uses `unsafe` internally to call SIMD
intrinsics.  These are marked `unsafe` only because invoking them on an
inappropriate CPU would cause `SIGILL`, but the entire backend is only
invoked when the appropriate CPU features are detected at runtime, or
when the whole program is compiled with the appropriate `target_feature`s.

# Performance

Benchmarks are run using [`criterion.rs`][criterion]:

```sh
cargo bench --features "rand_core"
export RUSTFLAGS='-C target_cpu=native'
cargo +nightly bench --features "rand_core"
```

Performance is a secondary goal behind correctness, safety, and
clarity, but we aim to be competitive with other implementations.

# FFI

Unfortunately, we have no plans to add FFI to `curve25519-dalek` directly.  The
reason is that we use Rust features to provide an API that maintains safety
invariants, which are not possible to maintain across an FFI boundary.  For
instance, as described in the _Safety_ section above, invalid points are
impossible to construct, and this would not be the case if we exposed point
operations over FFI.

However, `curve25519-dalek` is designed as a *mid-level* API, aimed at
implementing other, higher-level primitives.  Instead of providing FFI at the
mid-level, our suggestion is to implement the higher-level primitive (a
signature, PAKE, ZKP, etc) in Rust, using `curve25519-dalek` as a dependency,
and have that crate provide a minimal, byte-buffer-oriented FFI specific to
that primitive.

# Contributing

Please see [CONTRIBUTING.md][contributing].

# About

**SPOILER ALERT:** *The Twelfth Doctor's first encounter with the Daleks is in
his second full episode, "Into the Dalek". A beleaguered ship of the "Combined
Galactic Resistance" has discovered a broken Dalek that has turned "good",
desiring to kill all other Daleks. The Doctor, Clara and a team of soldiers
are miniaturized and enter the Dalek, which the Doctor names Rusty. They
repair the damage, but accidentally restore it to its original nature, causing
it to go on the rampage and alert the Dalek fleet to the whereabouts of the
rebel ship. However, the Doctor manages to return Rusty to its previous state
by linking his mind with the Dalek's: Rusty shares the Doctor's view of the
universe's beauty, but also his deep hatred of the Daleks. Rusty destroys the
other Daleks and departs the ship, determined to track down and bring an end
to the Dalek race.*

`curve25519-dalek` is authored by Isis Agora Lovecruft and Henry de Valence.

Portions of this library were originally a port of [Adam Langley's
Golang ed25519 library](https://github.com/agl/ed25519), which was in
turn a port of the reference `ref10` implementation.  Most of this code,
including the 32-bit field arithmetic, has since been rewritten.

The fast `u32` and `u64` scalar arithmetic was implemented by Andrew Moon, and
the addition chain for scalar inversion was provided by Brian Smith.  The
optimised batch inversion was contributed by Sean Bowe and Daira Hopwood.

The `no_std` and `zeroize` support was contributed by Tony Arcieri.

The formally verified `fiat_backend` integrates Rust code generated by the
[Fiat Crypto project](https://github.com/mit-plv/fiat-crypto) and was
contributed by François Garillot.

Thanks also to Ashley Hauck, Lucas Salibian, Manish Goregaokar, Jack Grigg,
Pratyush Mishra, Michael Rosenberg, @pinkforest, and countless others for their
contributions.

[ed25519-dalek]: https://github.com/dalek-cryptography/curve25519-dalek/tree/main/ed25519-dalek
[x25519-dalek]: https://github.com/dalek-cryptography/curve25519-dalek/tree/main/x25519-dalek
[docs]: https://docs.rs/curve25519-dalek/
[contributing]: https://github.com/dalek-cryptography/curve25519-dalek/blob/master/CONTRIBUTING.md
[criterion]: https://github.com/japaric/criterion.rs
[parallel_doc]: https://docs.rs/curve25519-dalek/latest/curve25519_dalek/backend/vector/index.html
[subtle_doc]: https://docs.rs/subtle
[fiat-crypto]: https://github.com/mit-plv/fiat-crypto
[semver]: https://semver.org/spec/v2.0.0.html
[rngcorestd]: https://github.com/rust-random/rand/tree/7aa25d577e2df84a5156f824077bb7f6bdf28d97/rand_core#crate-features
[zeroize-trait]: https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html
[SIMD backend]: #simd-backend
