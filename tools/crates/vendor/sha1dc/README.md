# sha1dc

[![CI](https://github.com/srijs/sha1dc/actions/workflows/ci.yml/badge.svg)](https://github.com/srijs/sha1dc/actions/workflows/ci.yml)
[![crates.io](https://img.shields.io/crates/v/sha1dc.svg)](https://crates.io/crates/sha1dc)
[![docs.rs](https://docs.rs/sha1dc/badge.svg)](https://docs.rs/sha1dc)

SHA-1 is cryptographically broken, because chosen-prefix collisions against it
are practical. However, there are still cases where it is needed for
compatibility, such as in `git`'s object identifiers.

This security issue can be mitigated by detecting those manufactured collisions.
This crate follows the method of Marc Stevens and Dan Shumow, which finds the
message blocks that a collision attack produces and reports them
([usenix17-paper], [crypto13-paper]).

To implement filtering for known disturbance vectors, it follows a code
generation approach. Each condition for an attack is a linear equation over two
bits of the expanded message. A solver searches the space they span for a set of
equations that suits the target instruction set, and emits code for it. Current
targets are `neon`, `sse2` and `avx2`, as well as a scalar baseline.

Where available, the implementation also uses SHA-1 hardware instructions on
`x86_64` and `aarch64`. Detection still does more work per block than plain
SHA-1, and costs 16% to 22% of throughput, depending on the machine.

## Usage

```rust
let digest = sha1dc::digest(b"hello world")?;
assert_eq!(digest.to_string(), "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed");
```

Two modes are provided, as two separate `Hasher` structs. `Hasher` keeps the
standard digest, with output equivalent to a non-detecting SHA-1
implementation. `mitigate::Hasher` computes an alternative digest instead. Both
report a detected attack as an error. The [documentation] covers both.

## Features

- `std` *(default)*: Enables run-time CPU feature detection for hardware
  acceleration and `std::io::Write` support for the hasher.

## Performance

These figures compare the crate against two others: [`sha1`], which does no
detection at all, and [`sha1-checked`], which detects the same collisions and
is a direct translation of the original C code to Rust. Each is at the best it
can do on the machine. Throughput is in MiB/s, and as a fraction of the
[`sha1`] column.

| machine                   |    [`sha1`] |     `sha1dc` | [`sha1-checked`] |
|---------------------------|------------:|-------------:|-----------------:|
|                           |    *0.11.0* | *this crate* |    *0.11.0-rc.0* |
| **AArch64**               |             |              |                  |
| Apple M4                  | 2969 (100%) |   2337 (79%) |        721 (24%) |
| Graviton4                 | 1618 (100%) |   1335 (82%) |        412 (25%) |
| **x86-64 with SHA-NI**    |             |              |                  |
| Sapphire Rapids           | 1926 (100%) |   1511 (78%) |        466 (24%) |
| Ice Lake                  | 1583 (100%) |   1275 (81%) |        322 (20%) |
| Zen 4                     | 1845 (100%) |   1543 (84%) |        448 (24%) |
| Zen 3                     | 1864 (100%) |   1559 (84%) |        418 (22%) |
| **x86-64 without SHA-NI** |             |              |                  |
| Cascade Lake              |  623 (100%) |    555 (89%) |        330 (53%) |

[`sha1`] and `sha1dc` both take the SHA-1 instructions of the machine,
SHA-NI on x86 and the ARMv8 ones on the M4 and the Graviton4, and differ in
whether they detect. The `sha1dc` shortfall from 100% is therefore what
detection costs: 16% to 22%, depending on the machine. Cascade Lake predates
SHA-NI, so both compute SHA-1 in software there, and detection costs 11%.

*The Apple M4 is a laptop; the rest are EC2 metal instances: c8g.metal-24xl
(Graviton4), c7i.metal-24xl (Sapphire Rapids), c6i.metal (Ice Lake),
c7a.metal-48xl (Zen 4), c6a.metal (Zen 3) and c5.metal (Cascade Lake).*

## Testing

The filter is generated code, so the tests check what the generator produces
against the original implementation rather than against itself.

- **Against the original.** The tests check every generated form against the
  original C implementation, [sha1collisiondetection]: it must give exactly
  the same answer on a million random message blocks, and next to a block
  that each disturbance vector survives, with every bit and every group of
  tied bits flipped in turn. The original tested its own generated code the
  same way.
- **Against known collisions.** SHAttered, SHA-mbles and a reduced-round
  collision must be detected, with and without mitigation.
- **Against plain SHA-1.** Digests must match the NIST test vectors and the
  [`sha1`] crate on random input, and the hardware and software backends must
  agree on any block.

CI runs the tests on x86-64 and AArch64, on Linux, macOS and Windows, and under
QEMU on big-endian s390x, 32-bit x86 and older x86 CPUs, so that every form of
the filter and every backend runs somewhere.

## License

Licensed under either of [Apache License, Version 2.0](LICENSE-APACHE) or
[MIT license](LICENSE-MIT) at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in this project by you, as defined in the Apache-2.0 license,
shall be dual licensed as above, without any additional terms or conditions.

[usenix17-paper]: https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-stevens.pdf
[crypto13-paper]: https://marc-stevens.nl/research/papers/C13-S.pdf
[sha1collisiondetection]: https://github.com/cr-marcstevens/sha1collisiondetection
[`sha1`]: https://crates.io/crates/sha1
[`sha1-checked`]: https://crates.io/crates/sha1-checked
[documentation]: https://docs.rs/sha1dc
