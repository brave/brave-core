# `float8`: 8-bit floating point types for Rust

This crate provides 2 types:
- `F8E4M3`: Sign + 4-bit exponent + 3-bit mantissa. More precise but less dynamic range.
- `F8E5M2`: Sign + 5-bit exponent + 2-bit mantissa. Less precise but more dynamic range (same exponent as `f16`).

Generally, this crate is modelled after the `half` crate, so it can be
used alongside and with minimal code changes.

- This crate provides `no_std` support
- Requires Rust 1.70 or greater

## Optional features
- `std` - Enable features that depend on the Rust standard library.
- `serde` - Add support for the `serde` crate with `Serialize` and `Deserialize` traits.
- `num-traits` - Implement traits from `num-traits` such as `ToPrimitive`, `FromPrimitive`, `AsPrimitive`, `Num`, `Float`, `FloatCore`, and `Bounded`.
- `bytemuck` - Implement traits from `bytemuck` including `Zeroable` and `Pod`
- `zerocopy` - Implement traits from `zerocopy` including `AsBytes` and `FromBytes`
- `rand_distr` - Implement traits from `rand_distr` including `Distribution` and others
- `rkyv` - Enable zero-copy deserialization with `rkyv`.

## Resources
- Good introduction: https://en.wikipedia.org/wiki/Minifloat
- Paper: https://arxiv.org/pdf/2209.05433
