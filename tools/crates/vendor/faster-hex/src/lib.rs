//! Hexadecimal encoding, decoding and formatting for byte sequences.
//!
//! The core operations write into caller-provided buffers without allocating.
//! SIMD implementations are selected internally where available; a portable scalar
//! implementation provides the same behavior on other targets.
//!
//! # Getting started
//!
//! ```
//! use faster_hex::{hex_decode, hex_encode};
//!
//! let mut encoded = [0; 10];
//! let text = hex_encode(b"hello", &mut encoded)?;
//! assert_eq!(text, "68656c6c6f");
//!
//! let mut decoded = [0; 5];
//! assert_eq!(hex_decode(text.as_bytes(), &mut decoded)?, b"hello");
//! # Ok::<(), faster_hex::Error>(())
//! ```
//!
//! # Choosing an operation
//!
//! | Destination or task | API |
//! | --- | --- |
//! | Encode into a byte buffer | [`hex_encode`], [`hex_encode_upper`] |
//! | Decode into a byte buffer | [`hex_decode`], [`hex_decode_with_case`] |
//! | Decode an exact-length array | [`hex_decode_array`], [`hex_decode_array_with_case`] |
//! | Format borrowed bytes into text | [`Hex`] with [`Display`](core::fmt::Display), [`LowerHex`](core::fmt::LowerHex) or [`UpperHex`](core::fmt::UpperHex) |
//! | Check characters without decoding | [`hex_check`], [`hex_check_with_case`] |
//!
//! With `alloc`, `hex_string` and `hex_string_upper` create owned strings;
//! `hex_append` and `hex_append_upper` reuse a string's capacity;
//! `hex_decode_vec` and `hex_decode_vec_with_case` create owned byte vectors.
//! The `heapless-09` feature provides fixed-capacity strings in `heapless_09`.
//!
//! # Conversion contracts
//!
//! Encoding writes two ASCII digits per input byte. Decoding consumes the complete
//! input and requires an even number of ASCII hex digits. Both preserve leading
//! zeroes and byte order; neither treats the input as an integer. Slice and owned
//! decoders reject `0x` prefixes, whitespace, separators and non-ASCII characters.
//! Serde adapters have their own explicit prefix policies.
//!
//! Successful slice conversions return exactly the written prefix, borrowing only
//! the destination. Extra destination capacity remains unchanged. Empty inputs
//! succeed. Every slice conversion error preserves the entire destination, even
//! when invalid input occurs after a long valid prefix.
//!
//! [`Error`] exposes byte positions and required or exact lengths. Slice decoding
//! checks odd input, destination capacity, then characters. Array decoding checks
//! odd input, exact decoded length, then characters. Each function documents its
//! full error contract. [`hex_check`] checks characters only and can accept odd
//! lengths; checked decoders already perform validation, so a preceding check is
//! unnecessary.
//!
//! # Crate features
//!
//! Features are additive. The defaults are `std` and `serde`. With defaults disabled
//! and no optional features, the crate has no dependencies and needs neither an
//! allocator nor the standard library.
//!
//! | Feature | Provides |
//! | --- | --- |
//! | None | Slice and fixed-array conversion, borrowed formatting, and `core::error::Error` |
//! | `alloc` | Owned strings and byte vectors; appending to strings |
//! | `std` | `alloc` and standard-library support in enabled dependencies |
//! | `serde` | Serde adapters and `alloc`; also works without `std` |
//! | `heapless-09` | Fixed-capacity strings using `heapless` 0.9, without requiring `alloc` |
//! | `defmt` (also `defmt-03`) | `defmt::Format` for [`Error`] and [`CheckCase`] |
//!
//! For example, enable Serde without the standard library:
//!
//! ```toml
//! [dependencies]
//! faster-hex = { version = "2", default-features = false, features = ["serde"] }
//! ```
//!
//! # Platforms
//!
//! On x86 and x86-64, implementations use SSE4.1, AVX2 and, for checking and
//! decoding, AVX-512BW. Runtime detection checks CPU and operating-system support
//! for features not enabled at compile time. AArch64 targets that guarantee NEON
//! use it directly. Other configurations use the portable fallback. Backend
//! selection, SIMD thresholds and instruction sequences are implementation details;
//! no public backend selection or architecture-specific call is required.
//!
//! The minimum supported Rust version is 1.95.0 throughout the 2.0.x line.
#![cfg_attr(not(any(test, feature = "std")), no_std)]
#![warn(missing_docs)]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(
    feature = "alloc",
    doc = r#"
# Owned output and capacity reuse

[`hex_decode_vec`] returns owned decoded bytes; [`hex_string`] returns owned
text. [`hex_append`] preserves existing text and returns only its new suffix.
These functions follow the allocator's normal error handling. Allocation failures
are not codec errors.

```
use faster_hex::{hex_append, hex_decode_vec};

let bytes = hex_decode_vec(b"00aB")?;
let mut text = String::with_capacity(64);
text.push_str("id: ");
assert_eq!(hex_append(&bytes, &mut text), "00ab");
assert_eq!(text, "id: 00ab");
# Ok::<(), faster_hex::Error>(())
```
"#
)]
#![cfg_attr(
    feature = "serde",
    doc = r##"
# Serde adapters

The default `#[serde(with = "faster_hex")]` adapter writes lowercase hex with a
`0x` prefix and accepts either letter case when reading. A required prefix is
exactly `0x`, never `0X`. Named modules select the wire policy:

| Module | Prefix | Serialization | Accepted letters |
| --- | --- | --- | --- |
| [`withpfx_ignorecase`] (default) | `0x` | Lowercase | Either case |
| [`nopfx_ignorecase`] | None | Lowercase | Either case |
| [`withpfx_lowercase`] | `0x` | Lowercase | Lowercase |
| [`nopfx_lowercase`] | None | Lowercase | Lowercase |
| [`withpfx_uppercase`] | `0x` | Uppercase | Uppercase |
| [`nopfx_uppercase`] | None | Uppercase | Uppercase |

Each policy also has an `option_` counterpart, an `array` submodule, and a
`deserialize_bounded` function. Present byte values use strings, including in
binary formats. Option adapters preserve the format's `Some`/`None` tags; an empty
present value stays distinct from `None`. For missing struct fields, add
`#[serde(default)]` alongside the `with` attribute.

Use [`array`](mod@crate::array) for exact-length arrays. Generic adapters instead
collect into [`FromIterator<u8>`](core::iter::FromIterator) containers; bounded
collectors can panic when full. [`deserialize_bounded`] limits decoded bytes before output allocation,
but does not bound the format's input storage or a custom collector's allocations.

```
#[derive(Debug, PartialEq, serde::Serialize, serde::Deserialize)]
struct Record {
    #[serde(with = "faster_hex::array")]
    id: [u8; 2],
    #[serde(default, with = "faster_hex::option_nopfx_lowercase::array")]
    extra: Option<[u8; 2]>,
}

let record = Record { id: [0xab, 1], extra: Some([0xcd, 2]) };
let json = serde_json::to_string(&record)?;
assert_eq!(json, r#"{"id":"0xab01","extra":"cd02"}"#);
assert_eq!(serde_json::from_str::<Record>(&json)?, record);
assert_eq!(serde_json::from_str::<Record>(r#"{"id":"0xab01"}"#)?.extra, None);
# Ok::<(), serde_json::Error>(())
```
"##
)]

#[cfg(feature = "alloc")]
extern crate alloc;

mod decode;
mod encode;
mod error;
mod format;

// Both cargo-fuzz and cargo-afl set this cfg. It is deliberately not a Cargo
// feature: regular builds (including --all-features) have no backend API.
// Unit tests share the same safe adapters instead of duplicating unsafe calls.
#[cfg(any(test, fuzzing))]
#[doc(hidden)]
#[allow(missing_docs)]
pub mod fuzzing;

#[cfg(feature = "heapless-09")]
#[cfg_attr(docsrs, doc(cfg(feature = "heapless-09")))]
pub mod heapless_09;

#[cfg(feature = "serde")]
mod serde;

pub use crate::decode::{
    hex_check, hex_check_with_case, hex_decode, hex_decode_array, hex_decode_array_with_case,
    hex_decode_with_case, CheckCase,
};
pub use crate::encode::{hex_encode, hex_encode_upper};

#[cfg(feature = "alloc")]
pub use crate::encode::{hex_append, hex_append_upper, hex_string, hex_string_upper};

#[cfg(feature = "alloc")]
pub use crate::decode::{hex_decode_vec, hex_decode_vec_with_case};

pub use crate::error::Error;
pub use crate::format::Hex;

#[cfg(feature = "serde")]
pub use crate::serde::withpfx_ignorecase::array;

#[cfg(feature = "serde")]
pub use crate::serde::{
    deserialize, deserialize_bounded, nopfx_ignorecase, nopfx_lowercase, nopfx_uppercase,
    option_nopfx_ignorecase, option_nopfx_lowercase, option_nopfx_uppercase,
    option_withpfx_ignorecase, option_withpfx_lowercase, option_withpfx_uppercase, serialize,
    withpfx_ignorecase, withpfx_lowercase, withpfx_uppercase,
};

#[cfg(any(target_arch = "x86", target_arch = "x86_64", target_arch = "aarch64"))]
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[cfg_attr(feature = "defmt-03", derive(defmt::Format))]
#[allow(dead_code, reason = "Static ISA baselines bypass runtime detection")]
pub(crate) enum Vectorization {
    None = 0,
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    SSE41 = 1,
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    AVX2 = 2,
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    // Keep dispatch sparse so adding this backend does not create a jump table.
    AVX512 = 128,
    #[cfg(target_arch = "aarch64")]
    Neon = 3,
}

#[inline(always)]
#[cfg(any(target_arch = "x86", target_arch = "x86_64", target_arch = "aarch64"))]
pub(crate) fn vectorization_support() -> Vectorization {
    #[cfg(all(
        any(target_arch = "x86", target_arch = "x86_64"),
        target_feature = "avx512bw",
        not(miri)
    ))]
    {
        return Vectorization::AVX512;
    }

    #[cfg(all(
        any(target_arch = "x86", target_arch = "x86_64"),
        target_feature = "avx2",
        not(target_feature = "avx512bw"),
        not(miri)
    ))]
    {
        return Vectorization::AVX2;
    }

    #[cfg(all(
        any(target_arch = "x86", target_arch = "x86_64"),
        target_feature = "sse",
        not(target_feature = "avx2"),
        not(miri)
    ))]
    {
        use core::sync::atomic::{AtomicU8, Ordering};
        static FLAGS: AtomicU8 = AtomicU8::new(u8::MAX);

        // Relaxed is enough: racing initializers detect the same CPU features.
        return match FLAGS.load(Ordering::Relaxed) {
            0 => Vectorization::None,
            1 => Vectorization::SSE41,
            2 => Vectorization::AVX2,
            128 => Vectorization::AVX512,
            _ => {
                let backend = vectorization_support_no_cache_x86();
                FLAGS.store(backend as u8, Ordering::Relaxed);
                backend
            }
        };
    }

    #[cfg(all(target_arch = "aarch64", target_feature = "neon", not(miri)))]
    {
        return Vectorization::Neon;
    }

    #[allow(unreachable_code)]
    Vectorization::None
}

#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    target_feature = "sse",
    not(target_feature = "avx2"),
    not(miri)
))]
#[cold]
fn vectorization_support_no_cache_x86() -> Vectorization {
    #[cfg(target_arch = "x86")]
    use core::arch::x86::__cpuid_count;
    #[cfg(target_arch = "x86_64")]
    use core::arch::x86_64::__cpuid_count;

    // SGX doesn't support CPUID,
    // If there's no SSE there might not be CPUID and there's no SSE4.1/AVX2
    if cfg!(target_env = "sgx") || !cfg!(target_feature = "sse") {
        return Vectorization::None;
    }

    // Query only supported basic leaves: out-of-range CPUID results may describe
    // a different leaf, whose bits must not be interpreted as AVX2 support.
    let max_leaf = __cpuid_count(0, 0).eax;
    if max_leaf < 1 {
        return Vectorization::None;
    }
    let proc_info_ecx = __cpuid_count(1, 0).ecx;
    let have_sse4 = (proc_info_ecx >> 19) & 1 == 1;
    // If there's no SSE4 there can't be AVX2.
    if !have_sse4 {
        return Vectorization::None;
    }

    let have_xsave = (proc_info_ecx >> 26) & 1 == 1;
    let have_osxsave = (proc_info_ecx >> 27) & 1 == 1;
    let have_avx = (proc_info_ecx >> 28) & 1 == 1;
    if max_leaf >= 7 && have_xsave && have_osxsave && have_avx {
        // SAFETY: XSAVE is available and enabled by the OS; leaf 7 exists.
        return unsafe { avx_support_no_cache_x86() };
    }
    Vectorization::SSE41
}

// We enable xsave so it can inline the _xgetbv call.
// # Safety: Requires XSAVE, OSXSAVE and AVX, and CPUID basic leaf 7 must exist.
#[target_feature(enable = "xsave")]
#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    target_feature = "sse",
    not(target_feature = "avx2"),
    not(miri)
))]
#[cold]
unsafe fn avx_support_no_cache_x86() -> Vectorization {
    #[cfg(target_arch = "x86")]
    use core::arch::x86::{__cpuid_count, _xgetbv};
    #[cfg(target_arch = "x86_64")]
    use core::arch::x86_64::{__cpuid_count, _xgetbv};

    let xcr0 = _xgetbv(0);
    let os_avx_support = xcr0 & 6 == 6;
    if os_avx_support {
        let extended_features_ebx = __cpuid_count(7, 0).ebx;
        let have_avx2 = (extended_features_ebx >> 5) & 1 == 1;
        if have_avx2 {
            // AVX-512 needs opmask and both ZMM state components in addition
            // to SSE/AVX state. CPUID alone is insufficient for safe dispatch.
            let avx512 = (1 << 16) | (1 << 30); // AVX-512F and AVX-512BW.
            if xcr0 & 0xe6 == 0xe6 && extended_features_ebx & avx512 == avx512 {
                return Vectorization::AVX512;
            }
            return Vectorization::AVX2;
        }
    }
    Vectorization::SSE41
}

#[cfg(test)]
mod tests;
