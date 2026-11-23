# Migration guide

## Migrating from 1.0 to 2.0

Set the `faster-hex` dependency to `"2"`. If you use the fixed-capacity
encoders, change the Cargo feature from `heapless-08` to `heapless-09` and
imports from `faster_hex::heapless_08` to `faster_hex::heapless_09`.
Both encoders now return a `heapless` 0.9 string. If your code names that type,
update your direct `heapless` dependency to `"0.9.4"` as well.

```toml
[dependencies]
faster-hex = { version = "2", default-features = false, features = ["heapless-09"] }
heapless = { version = "0.9.4", default-features = false }
```

```rust
use faster_hex::heapless_09::hex_string;

let text: heapless::String<4> = hex_string(&[0xab, 1]).unwrap();
assert_eq!(text.as_str(), "ab01");
```

If `std` is the only `faster-hex` feature you enable, you can keep an existing
pin such as `defmt = "=0.3.10"`.

For logging with `defmt` 1.x, use `features = ["defmt"]` on `faster-hex`.
Existing manifests can keep `features = ["defmt-03"]`. With either name, Cargo
considers the `defmt` 1.x requirement from `heapless` 0.9 when resolving the
lockfile, even if `heapless-09` is disabled.
In that case, loosen an old pin such as `defmt = "=0.3.10"` to `defmt = "0.3"`.
Cargo can then select 0.3.100 or a later 0.3 release that shares the 1.x
`Format` trait. If another crate imposes the pin, update it to a version that
allows a compatible release.

## Migrating from 0.10 to 1.0

Version 1.0 requires Rust 1.95.0. The default features remain `std, serde`.
Existing valid, exactly sized slice conversions keep the same bytes and letter-case
behavior. The changes below make lengths, allocation and backend selection explicit.

### Slice contracts

Since 0.10.1, `hex_encode` and `hex_encode_upper` return only the initialized hex
prefix. Spare destination bytes remain unchanged and are never exposed through a
UTF-8 string. Code that used the returned string's length as the destination
capacity should use the destination slice's length instead.

`hex_decode` and `hex_decode_with_case` now return `Result<&mut [u8], Error>`.
The successful slice contains exactly the decoded bytes and borrows only `dst`.
Calls followed by `?;` or `.unwrap();` usually need no changes. Wrappers returning
`Result<(), Error>` should use `.map(|_| ())`.

Decode consumes the entire input. It accepts extra output capacity, leaves that
capacity untouched, and rejects a destination too small for the complete input.
To decode a deliberate prefix, slice the input explicitly:

```rust
use faster_hex::hex_decode;

let source = b"0011";
let mut first_byte = [0xa5; 1];
hex_decode(&source[..2], &mut first_byte).unwrap();
assert_eq!(first_byte, [0]);
```

Passing the complete `source` above with a one-byte destination now returns
`Error::OutputTooSmall { required: 2, .. }` without changing the destination. Odd input is rejected
with `Error::OddLength`, including when the destination is empty. Empty input is valid.

All checked slice APIs preserve the entire destination on every error. Decode error
precedence is odd input, insufficient output capacity, then invalid character/case.
`hex_check` remains a character-only check: it can accept an odd number of digits.

### Public API replacements

| Removed export | Replacement |
| --- | --- |
| `hex_decode_unchecked`, `hex_decode_fallback` | `hex_decode` or `hex_decode_with_case` |
| `hex_encode_fallback`, `hex_encode_upper_fallback` | `hex_encode`, `hex_encode_upper` |
| `hex_check_fallback`, `hex_check_sse`, `hex_check_neon` | `hex_check` |
| `hex_check_sse_with_case`, `hex_check_neon_with_case` | `hex_check_with_case` |
| Deprecated `hex_to` | `hex_encode(src, dst).map(|_| ())` |

`CheckCase` and `hex_decode_with_case` are directly exported. Backend functions are
implementation details; callers no longer need architecture-specific imports or
unsafe CPU-feature assumptions.

In 0.10.0, the safe `hex_decode_unchecked` entry could read beyond a short source
on AVX2 when the destination held at least 32 bytes. Version 0.10.1 fixes the
bounds check; 1.0 removes the entry. Migrate to a checked decoder. Installed
0.10.0 copies remain affected until upgraded.

`Error` is now non-exhaustive and adds `OddLength`. `InvalidLength(n)` becomes
`OutputTooSmall { required, .. }`, with the required destination capacity in bytes.
`InvalidChar` becomes `InvalidChar { index, byte, .. }`, identifying the first invalid
input byte, including a letter rejected by the case policy. The index counts bytes,
not Unicode characters. Include `..` in the data-bearing variant patterns and a
wildcard arm for future variants:

```rust
use faster_hex::{hex_decode, Error};

fn describe(error: Error) -> String {
    match error {
        Error::InvalidChar { index, byte, .. } => format!("byte {byte:02x} at {index}"),
        Error::OutputTooSmall { required, .. } => format!("need {required} bytes"),
        other => other.to_string(),
    }
}

let error = hex_decode(b"0g", &mut [0; 2]).unwrap_err();
assert_eq!(describe(error), "byte 67 at 1");
```

Downstream code cannot construct the non-exhaustive data-bearing variants; applications
should define their own errors rather than manufacture a failed codec operation.
`Debug` now shows the enum and fields. Both `Debug` and `Display` text have changed
and are not stable serialization formats. Match variants instead of parsing text.
`core::error::Error` is implemented with every feature set.

### Features and strings

New optional conveniences require no changes to existing slice calls.
`hex_decode_array::<N>` and `hex_decode_array_with_case::<N>` return `[u8; N]`
without allocation and require exactly `N` decoded bytes. Their error order is
odd length, `LengthMismatch { expected, actual, .. }`, then character/case.
Both lengths in the new non-exhaustive variant count decoded bytes.
With `alloc`, `hex_decode_vec` and `hex_decode_vec_with_case` return owned bytes;
odd input is rejected before allocation, while even invalid input can allocate.
All four consume strict, unprefixed ASCII hex and preserve existing slice contracts.

Disabling defaults now provides the dependency-free slice API. Heapless support is optional;
enabling `alloc` or `std` no longer changes another function's return type.

| Requirement | Features and API |
| --- | --- |
| New allocated string | `alloc`: `hex_string`, `hex_string_upper` |
| New allocated bytes | `alloc`: `hex_decode_vec`, `hex_decode_vec_with_case` |
| Exact decoded array | No feature required: `hex_decode_array::<N>`, `hex_decode_array_with_case::<N>` |
| Reuse an existing allocation | `alloc`: `hex_append`, `hex_append_upper` |
| Format directly into a writer | No feature required: `Hex::new(bytes)` supports `Display`, `LowerHex`, `UpperHex` |
| Fixed capacity without allocation | `heapless-08`: `heapless_08::hex_string::<N>`, `heapless_08::hex_string_upper::<N>` |
| Serde without `std` | `default-features = false, features = ["serde"]` |

The heapless functions return `Result`; insufficient capacity is
`Error::OutputTooSmall { required, .. }`. Capacity `N` counts encoded characters, so binary
input needs twice as much capacity. `heapless-08`, `alloc`, `serde` and `defmt-03` can be
combined without removing or renaming another enabled API.

The `heapless_08` module always uses `heapless` 0.8. Support for an incompatible
dependency version will use a new module and feature, retaining the 0.8 API in 1.x.

Append functions preserve existing content and return the appended suffix. To replace
the previous value while retaining capacity, call `String::clear()` first.

For logs or a `core::fmt::Write` destination, `Hex` avoids an intermediate String.
It preserves leading zeroes and uses integer-style formatting flags. Precision
never truncates bytes; a writer error can leave already accepted text in place.

### Serde compatibility

Existing adapter module names, JSON representation, prefix/case rules and
generic `FromIterator<u8>` support remain compatible. The required prefix is exactly
`0x`. The implementation borrows input text when the deserializer can lend it and uses
stack storage for short serialized values; callers should not depend on allocation counts.

Serialization now calls `AsRef::as_ref` once per present value, using that same slice
for sizing and encoding. A stateful `AsRef` implementation therefore no longer lets
different input views affect these two steps. Invalid-byte errors now include the
position within the hex payload, after removing a required `0x` prefix.

The collector's behavior is still its responsibility; bounded collectors can panic
on capacity exhaustion. Use the new `faster_hex::array` adapter for `[u8; N]` fields,
or the `array` submodule of a named policy for other prefix/case rules and optional
arrays. These enforce exactly `N` bytes before conversion and avoid an intermediate
byte vector. The generic adapters retain their existing `FromIterator<u8>` bound.

Option serialization now calls `serialize_some` for present values. The old adapter
bypassed this Serde tag: with postcard and no prefix, `Some([])` could decode as
`None`, while nonempty values failed to round-trip. JSON is unchanged, but binary
records produced using the affected unreleased Option adapters have a different
representation. The indistinguishable empty-`Some`/`None` case cannot be recovered
from those old bytes alone.

## Compatibility policy

- Within each major release, faster-hex preserves public signatures, feature names
  and defaults, exposed dependency types, implemented traits, and documented
  input/error/mutation rules.
  Features remain additive under Cargo feature unification.
- Rust 1.95.0 is the minimum for 1.0.x and 2.0.x. Later increases require a minor
  release and release notes; this does not freeze consumers' dependency versions.
- New input grammars, storage ownership or partial-progress behavior use separate
  APIs. Adding error variants or fields preserves the meanings of existing errors.
- Backend selection, timing, allocation counts, error text and hash algorithms are
  implementation details. Documented allocation-free operations remain so; no
  memory layout or Rust ABI is promised.
