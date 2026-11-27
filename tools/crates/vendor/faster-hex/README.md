# faster-hex

[![Crates.io](https://img.shields.io/crates/v/faster-hex.svg)](https://crates.io/crates/faster-hex)
[![docs.rs](https://img.shields.io/docsrs/faster-hex?label=docs.rs)](https://docs.rs/faster-hex)

Fast hexadecimal encoding and decoding with SIMD acceleration and a portable
fallback. Uses SSE4.1 or AVX2 on x86 and NEON on AArch64 when available, with
AVX-512BW acceleration for validation and decoding. Supports `no_std`.
Requires Rust 1.95.0 or later.

## Usage

```toml
[dependencies]
faster-hex = "2"
```

```rust
use faster_hex::{hex_decode, hex_encode};

fn main() -> Result<(), faster_hex::Error> {
    let mut encoded = [0; 10];
    let text = hex_encode(b"hello", &mut encoded)?;
    assert_eq!(text, "68656c6c6f");

    let mut decoded = [0; 5];
    assert_eq!(hex_decode(text.as_bytes(), &mut decoded)?, b"hello");
    Ok(())
}
```

Both conversions process the complete input and leave the destination unchanged
on errors. Extra output capacity is allowed and remains untouched. Decoding
accepts mixed-case ASCII hex, reports the first invalid byte and its position,
and offers explicit lowercase/uppercase policies through `hex_decode_with_case`.

For a fixed hash, `hex_decode_array::<32>` returns an owned array without heap
allocation. For logging or an existing text writer, `Hex::new(&bytes)` formats
directly, including a prefix with `{:#x}`, without a temporary hex string. Use
`hex_append` to reuse a `String`'s capacity across calls.

The default features are `std` and `serde`. Set `default-features = false` for a
dependency-free core. Enable `alloc` for owned output or `heapless-09` for
fixed-capacity strings.

For embedded logging with `defmt` 1.x, use `features = ["defmt"]` to format
`Error` and `CheckCase`. Existing projects can keep `features = ["defmt-03"]`.

```toml
[dependencies]
defmt = "1"
faster-hex = { version = "2", default-features = false, features = ["defmt"] }
```

[API documentation](https://docs.rs/faster-hex) follows the released crate. For a
local checkout, run `cargo doc --all-features --no-deps --open`.
For upgrades from 1.0 or 0.10, see the [migration guide](MIGRATION.md).
Release history: [changelog](CHANGELOG.md).

## Testing changes

CI checks the API against the pinned 2.0 baseline across all supported
feature combinations, including external trait and function-signature contracts.
It runs native 64-bit x86/ARM and 32-bit x86 tests, including guard-page checks
with independently sized input and output. Miri checks portable paths; native
tests and fuzzing exercise SIMD. CI runs libFuzzer on x86 and ARM and AFL on x86.
Weekly and manual fuzz runs use longer time limits, restore cached corpora, and
save minimized inputs and crash diagnostics as workflow artifacts.

Both fuzz engines share public-contract and core tests. Every input constructs
valid hex independently, exercises every available backend, and injects faults
after valid prefixes. Short inputs also drive multi-block payloads. Backend
adapters are shared with unit tests and exposed only under `cfg(fuzzing)`; normal
and `--all-features` builds have no backend API.
After minimization, LLVM coverage replay must still reach the scalar and available
SIMD kernels. JSON and HTML coverage reports are retained with the fuzz artifacts.

For native AVX-512 acceptance, run the **Fuzz** workflow with `require-avx512`
enabled. Both x86 engines must run on CPUs with AVX2, AVX-512F and AVX-512BW;
their minimized corpora must execute every AVX-512 checking and decoding kernel,
including the owned-output loop.
The replay verifies Rust's CPU/OS detection and records each kernel's execution
and region counts. Unsupported runners fail before the fuzz build. Standard
hosted runner labels do not guarantee AVX-512; retry the affected job if necessary.
Regular runs exercise AVX-512 whenever available, without requiring it on every
x86 runner. Use this explicit acceptance mode before freezing AVX-512 changes.

## Benchmarks

For development, run `cargo bench-dev` from this checkout. Its 17 cases cover
encoding and decoding 4, 8, 32, 65, 256 and 4096 bytes, short and Hash256 arrays,
an allocated vector, and errors at the start and end of a large input.
Slice buffers are allocated before timing; validation and CPU dispatch are included.
Each case uses 0.2 seconds of warm-up and 1 second of sampling, so expect roughly
20–30 seconds after compilation.

Save a baseline before editing, then compare the same cases after your change:

```sh
cargo bench-dev --save-baseline before
# Edit the implementation.
cargo bench-dev --baseline before
```

Criterion's `change: time` reports the change in latency: negative is faster,
positive is slower. It also reports whether the change is statistically
significant. `--baseline before` keeps the saved baseline unchanged, so you can
compare multiple edits against it. Results live in `target/criterion`; keep that
directory when switching between commits. Use the same machine, Rust toolchain,
features and `Cargo.lock`, and run one benchmark process at a time.
Use the same benchmark code on both sides; recreate saved baselines after
changing the harness.

To compare with `hex`, `const-hex`, `hex-simd`, `fashex`, `better-hex` and
`data-encoding`, run:

```sh
cargo bench-compare
```

This runs 56 cases at 8, 32, 256 and 4096 bytes, taking roughly 70 seconds after
compilation. All libraries receive the same input and reuse their output buffer.
Encoding is lowercase; decoding accepts mixed case. Sizes and throughput count
binary payload bytes: `decode/faster_hex_mixed/32` reads 64 hex characters and
produces 32 bytes. Append `--list` to a command to inspect the selected cases.

The aliases do not set CPU affinity. For repeatable Linux comparisons, pin all
runs to the same logical CPU; on hybrid processors, select a performance core
and keep its SMT sibling idle. Replace `1` below with a CPU from your topology:

```sh
taskset -c 1 cargo bench-compare
```

Keep background load low and repeat the runs. Pinning prevents migration between
cores, but does not eliminate frequency changes or interference.

For a small performance difference, confirm the affected cases using Criterion's
longer default sampling, saving and comparing a new baseline as above:

```sh
cargo bench --bench hex -- '^encode/faster_hex/32$' --save-baseline confirm
# Edit the implementation, then rerun with --baseline confirm.
```

For Hash256 work, `cargo bench-hash` selects 28 cases at 32 binary bytes / 64
hex characters, taking roughly 35 seconds after compilation. It compares hot
encoding/decoding, rotation through 4096 different hashes, owned-array decoding,
and validation-only checks (fashex has no public checker). Rotation and buffer
reuse happen inside the timer; input generation and correctness checks stay
outside it.
Use `--save-baseline before` / `--baseline before` here too. Always rerun
`bench-dev` to check other lengths after a Hash256 optimization.

The `decode_array` cases return a new fixed array without heap allocation at
4, 8, 32, 65, 256 and 4096 bytes. The
`faster_hex_slice` and `fashex_slice` controls decode into a local array and
return it, so every case includes the same output ownership. Compare them with
the existing `decode` cases to see how output ownership and call-site optimization
affect performance.

The `decode_vec` cases include allocation, validation, decoding and destruction
at 32, 65, 256, 4096 and 65536 bytes. `fashex_slice` uses an initialized vector
through its slice interface. Run `cargo bench --bench hex -- '^decode_vec/'`
to compare these owning operations separately from reusable-buffer decoding.

To compare the default bench profile with cross-crate ThinLTO, keep separate
build directories and share only the Criterion reports:

```sh
CRITERION_HOME="$PWD/target/criterion-builds" CARGO_TARGET_DIR=target/bench-default \
  CARGO_PROFILE_BENCH_LTO=false cargo bench-hash --save-baseline default
CRITERION_HOME="$PWD/target/criterion-builds" CARGO_TARGET_DIR=target/bench-thin \
  CARGO_PROFILE_BENCH_LTO=thin cargo bench-hash --baseline default
```

Use the same CPU affinity and other build settings for both commands. A change
in these results measures the build configuration, not a library optimization.

`cargo bench --bench format` compares borrowed `Hex`, an allocated hex string,
per-byte formatting and padded uppercase output. `cargo bench --bench serde`
measures JSON serialization/deserialization and Postcard serialization, including
reused output buffers. `serialize_text_reuse` writes the same already encoded
payload to isolate JSON work from hex conversion. These test library operations
directly; application and network overhead are outside their scope.

When comparing separate checkouts, give them separate `CARGO_TARGET_DIR` values
and a shared `CRITERION_HOME` for reports. Reusing a build directory across copies
of the same package can select stale artifacts. Keep compiler flags, features
and the dependency lockfile identical on both sides.

The four benchmark targets cover conversion, arrays and strings (`hex`), validation
and invalid-byte positions (`check`), JSON/Postcard adapters (`serde`), and borrowed
formatting (`format`). Run a focused alias during development; the full matrix
uses Criterion's longer default sampling. CI runs `cargo bench --all-features
--benches -- --test` to execute cases without collecting timings. SIMD alignment
and boundary correctness belong to the forced-backend and guard-page tests.

Deferred work: [further optimization](OPTIMIZATION.md).

## License

[MIT](LICENSE). Third-party notices are in [LICENSE-THIRD-PARTY](LICENSE-THIRD-PARTY).
