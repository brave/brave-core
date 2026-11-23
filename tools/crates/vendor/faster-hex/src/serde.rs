#![warn(missing_docs)]

use core::iter::FromIterator;

mod internal {
    use crate::{
        decode::{hex_decode_array_with_case, hex_decode_with_case, CheckCase},
        encode::encode,
    };
    use alloc::{borrow::Cow, string::String, vec, vec::Vec};
    use core::{fmt, iter::FromIterator, mem::MaybeUninit};
    use serde::{
        de::{Error, Unexpected, Visitor},
        Deserialize, Deserializer, Serialize, Serializer,
    };

    // Serde's Cow<str> deserializer always allocates. This adapter preserves
    // String's deserialize_string hint and accepted inputs, but borrows text
    // that the format can lend (for example, unescaped JSON from a slice).
    struct Text<'a>(Cow<'a, str>);

    impl<'de> Deserialize<'de> for Text<'de> {
        fn deserialize<D: Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
            struct TextVisitor;

            impl<'de> Visitor<'de> for TextVisitor {
                type Value = Text<'de>;

                fn expecting(&self, f: &mut fmt::Formatter) -> fmt::Result {
                    f.write_str("a string")
                }

                fn visit_str<E: Error>(self, text: &str) -> Result<Self::Value, E> {
                    Ok(Text(Cow::Owned(text.into())))
                }

                fn visit_borrowed_str<E: Error>(self, text: &'de str) -> Result<Self::Value, E> {
                    Ok(Text(Cow::Borrowed(text)))
                }

                fn visit_string<E: Error>(self, text: String) -> Result<Self::Value, E> {
                    Ok(Text(Cow::Owned(text)))
                }

                fn visit_bytes<E: Error>(self, bytes: &[u8]) -> Result<Self::Value, E> {
                    match core::str::from_utf8(bytes) {
                        Ok(text) => self.visit_str(text),
                        Err(_) => Err(E::invalid_value(Unexpected::Bytes(bytes), &self)),
                    }
                }

                fn visit_borrowed_bytes<E: Error>(
                    self,
                    bytes: &'de [u8],
                ) -> Result<Self::Value, E> {
                    match core::str::from_utf8(bytes) {
                        Ok(text) => self.visit_borrowed_str(text),
                        Err(_) => Err(E::invalid_value(Unexpected::Bytes(bytes), &self)),
                    }
                }

                fn visit_byte_buf<E: Error>(self, bytes: Vec<u8>) -> Result<Self::Value, E> {
                    match String::from_utf8(bytes) {
                        Ok(text) => self.visit_string(text),
                        Err(error) => {
                            Err(E::invalid_value(Unexpected::Bytes(error.as_bytes()), &self))
                        }
                    }
                }
            }
            deserializer.deserialize_string(TextVisitor)
        }
    }

    pub(crate) fn serialize<S, T>(
        data: T,
        serializer: S,
        with_prefix: bool,
        case: CheckCase,
    ) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
        T: AsRef<[u8]>,
    {
        let src = data.as_ref();
        let prefix: &[u8] = if with_prefix { b"0x" } else { b"" };
        let len = src
            .len()
            .checked_mul(2)
            .and_then(|len| len.checked_add(prefix.len()))
            .ok_or_else(|| serde::ser::Error::custom(crate::Error::Overflow))?;
        // Fixed values through 65 bytes (including H520) fit with the prefix.
        // Initialize only the storage selected for this call.
        let mut stack;
        let mut heap;
        let dst = if len <= 132 {
            stack = [MaybeUninit::uninit(); 132];
            &mut stack[..len]
        } else {
            heap = Vec::<u8>::with_capacity(len);
            &mut heap.spare_capacity_mut()[..len]
        };
        for (slot, &byte) in dst.iter_mut().zip(prefix) {
            slot.write(byte);
        }
        encode(src, &mut dst[prefix.len()..], case == CheckCase::Upper)
            .map_err(serde::ser::Error::custom)?;
        // SAFETY: The prefix and encoder initialized all len bytes as ASCII.
        // MaybeUninit<u8> has u8's layout; unused capacity is excluded and the
        // backing stack buffer or allocation stays live throughout serialization.
        serializer.serialize_str(unsafe {
            core::str::from_utf8_unchecked(core::slice::from_raw_parts(dst.as_ptr().cast(), len))
        })
    }

    fn payload<E: Error>(text: &str, with_prefix: bool) -> Result<&str, E> {
        let text = if with_prefix {
            text.strip_prefix("0x")
                .ok_or_else(|| E::custom("invalid prefix"))?
        } else {
            text
        };
        if !text.len().is_multiple_of(2) {
            return Err(E::custom("invalid length"));
        }
        Ok(text)
    }

    fn decode<E: Error>(
        text: &str,
        with_prefix: bool,
        case: CheckCase,
        max_bytes: usize,
    ) -> Result<Vec<u8>, E> {
        let text = payload::<E>(text, with_prefix)?;
        let len = text.len() / 2;
        if len > max_bytes {
            return Err(E::custom(format_args!(
                "expected at most {max_bytes} decoded bytes, got {len}"
            )));
        }
        let mut bytes = vec![0; len];
        hex_decode_with_case(text.as_bytes(), &mut bytes, case).map_err(E::custom)?;
        Ok(bytes)
    }

    fn decode_array<E: Error, const N: usize>(
        text: &str,
        with_prefix: bool,
        case: CheckCase,
    ) -> Result<[u8; N], E> {
        let text = payload::<E>(text, with_prefix)?;
        hex_decode_array_with_case(text.as_bytes(), case).map_err(E::custom)
    }

    pub(crate) fn deserialize_array<'de, D, const N: usize>(
        deserializer: D,
        with_prefix: bool,
        case: CheckCase,
    ) -> Result<[u8; N], D::Error>
    where
        D: Deserializer<'de>,
    {
        let text = Text::deserialize(deserializer)?;
        decode_array(&text.0, with_prefix, case)
    }

    pub(crate) fn deserialize_option_array<'de, D, const N: usize>(
        deserializer: D,
        with_prefix: bool,
        case: CheckCase,
    ) -> Result<Option<[u8; N]>, D::Error>
    where
        D: Deserializer<'de>,
    {
        Option::<Text>::deserialize(deserializer)?
            .map(|text| decode_array(&text.0, with_prefix, case))
            .transpose()
    }

    pub(crate) fn deserialize<'de, D, T>(
        deserializer: D,
        with_prefix: bool,
        check_case: CheckCase,
        max_bytes: usize,
    ) -> Result<T, D::Error>
    where
        D: Deserializer<'de>,
        T: FromIterator<u8>,
    {
        let text = Text::deserialize(deserializer)?;
        decode(&text.0, with_prefix, check_case, max_bytes).map(|bytes| bytes.into_iter().collect())
    }

    pub(crate) fn serialize_option<S, T>(
        data: &Option<T>,
        serializer: S,
        with_prefix: bool,
        case: CheckCase,
    ) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
        T: AsRef<[u8]>,
    {
        // Some serializers attach an Option tag before serializing its value.
        // Give them a hex representation that implements Serde's value protocol.
        struct HexValue<'a> {
            data: &'a [u8],
            with_prefix: bool,
            case: CheckCase,
        }

        impl Serialize for HexValue<'_> {
            fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
                serialize(self.data, serializer, self.with_prefix, self.case)
            }
        }

        match data {
            Some(data) => serializer.serialize_some(&HexValue {
                data: data.as_ref(),
                with_prefix,
                case,
            }),
            None => serializer.serialize_none(),
        }
    }

    pub(crate) fn deserialize_option<'de, D, T>(
        deserializer: D,
        with_prefix: bool,
        check_case: CheckCase,
        max_bytes: usize,
    ) -> Result<Option<T>, D::Error>
    where
        D: Deserializer<'de>,
        T: FromIterator<u8>,
    {
        // Let Serde retain its complete Option protocol, including untagged
        // handling; the same decoder serves both optional and required text.
        let bytes = Option::<Text>::deserialize(deserializer)?
            .map(|text| decode(&text.0, with_prefix, check_case, max_bytes))
            .transpose()?;
        Ok(bytes.map(|bytes| bytes.into_iter().collect()))
    }
}

/// Serializes a byte view as lowercase hex with a `0x` prefix.
///
/// Available with `serde`. This is the default serializer used by
/// `#[serde(with = "faster_hex")]`. It accepts any [`AsRef<[u8]>`], reads that view
/// once, and writes a Serde string in every format, including binary formats.
/// Empty bytes serialize as `"0x"`. Use a named policy module to change the
/// prefix or letter case. Temporary storage may allocate; allocation failure
/// follows the allocator's error handling.
///
/// # Errors
///
/// Returns the serializer's error if writing the string fails, or if the encoded
/// length including the prefix cannot be represented as a `usize`.
///
/// # Panics
///
/// Panics if temporary output storage would exceed [`isize::MAX`] bytes.
///
/// # Examples
///
/// ```
/// #[derive(serde::Serialize)]
/// struct Record {
///     #[serde(with = "faster_hex")]
///     bytes: Vec<u8>,
/// }
///
/// let value = Record { bytes: vec![0xab, 1] };
/// assert_eq!(serde_json::to_string(&value)?, r#"{"bytes":"0xab01"}"#);
/// # Ok::<(), serde_json::Error>(())
/// ```
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
pub fn serialize<S, T>(data: T, serializer: S) -> Result<S::Ok, S::Error>
where
    S: serde::Serializer,
    T: AsRef<[u8]>,
{
    withpfx_ignorecase::serialize(data, serializer)
}

/// Deserializes a `0x`-prefixed hex string into a byte collection.
///
/// Available with `serde`. This is the default deserializer for
/// `#[serde(with = "faster_hex")]`. The prefix must be exactly `0x`; payload
/// letters may use either case, including mixed case. Whitespace and separators
/// are rejected. `"0x"` produces an empty collection.
///
/// Decoded bytes are collected into `T`, which implements [`FromIterator<u8>`],
/// for example [`Vec<u8>`](alloc::vec::Vec) or
/// [`VecDeque<u8>`](alloc::collections::VecDeque).
/// Input text is borrowed when the format can lend it; transient,
/// escaped or owned text may require storage. Use [`array`](mod@crate::array)
/// for `[u8; N]` fields, or [`deserialize_bounded`] to limit decoded output.
/// Allocation failure follows the allocator's error handling.
///
/// # Errors
///
/// Propagates deserializer errors, including invalid input types or UTF-8. Once
/// text is available, checks the prefix, even payload length, then characters,
/// in that order. Character diagnostics report the first invalid byte and its
/// byte position within the payload, excluding `0x`. Collection starts only
/// after successful decoding.
///
/// # Panics
///
/// A custom collector may panic, for example when its fixed capacity is exceeded.
/// This adapter does not provide fallible collection.
///
/// # Examples
///
/// ```
/// #[derive(serde::Deserialize)]
/// struct Record {
///     #[serde(with = "faster_hex")]
///     bytes: Vec<u8>,
/// }
///
/// let value: Record = serde_json::from_str(r#"{"bytes":"0x00aB"}"#)?;
/// assert_eq!(value.bytes, [0, 0xab]);
/// # Ok::<(), serde_json::Error>(())
/// ```
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
pub fn deserialize<'de, D, T>(deserializer: D) -> Result<T, D::Error>
where
    D: serde::Deserializer<'de>,
    T: FromIterator<u8>,
{
    withpfx_ignorecase::deserialize(deserializer)
}

/// Deserializes at most `MAX` decoded bytes from a `0x`-prefixed hex string.
///
/// Available with `serde`. The prefix, case and collection behavior is the same
/// as [`deserialize`]. `MAX` counts decoded bytes, so at most twice that many
/// hex digits are accepted. A zero limit accepts `"0x"`.
///
/// The limit is checked before allocating decoded output or invoking the collector.
/// It does not limit input text storage used by the format, error-message storage,
/// or allocations performed by a custom collector. It also does not constrain
/// serialization; pair this function with the ordinary [`serialize`] function.
/// Allocation failure follows the allocator's error handling.
///
/// # Errors
///
/// Propagates deserializer errors. After obtaining text, checks the prefix, even
/// payload length, the decoded-byte limit, then characters/case, in that order.
/// An over-limit value is rejected even if it also contains invalid hex digits.
///
/// # Panics
///
/// A custom collector can still panic when full; the acceptance limit does not
/// change its capacity or collection implementation.
///
/// # Examples
///
/// ```
/// #[derive(serde::Serialize, serde::Deserialize)]
/// struct Packet {
///     #[serde(
///         serialize_with = "faster_hex::serialize",
///         deserialize_with = "faster_hex::deserialize_bounded::<2, _, _>"
///     )]
///     bytes: Vec<u8>,
/// }
///
/// let packet: Packet = serde_json::from_str(r#"{"bytes":"0xab01"}"#)?;
/// assert_eq!(packet.bytes, [0xab, 1]);
/// assert!(serde_json::from_str::<Packet>(r#"{"bytes":"0x000102"}"#).is_err());
/// # Ok::<(), serde_json::Error>(())
/// ```
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
pub fn deserialize_bounded<'de, const MAX: usize, D, T>(deserializer: D) -> Result<T, D::Error>
where
    D: serde::Deserializer<'de>,
    T: FromIterator<u8>,
{
    withpfx_ignorecase::deserialize_bounded::<MAX, D, T>(deserializer)
}

// Required and optional adapters share one prefix/case policy declaration.
macro_rules! serde_adapters {
    ($mod_name:ident, $option_name:ident, $with_pfx:expr, $check_case:expr, $prefix:literal, $description:literal) => {
        #[doc = $description]
        #[doc = concat!(
                                    r###"
Use `#[serde(with = "...")]` for byte collections. Serialization reads one
[`AsRef<[u8]>`] view; deserialization collects into [`FromIterator<u8>`] after
validating the complete input. All formats use strings, including binary formats.

A required prefix is exactly `0x`. Payloads contain only ASCII hex digits;
empty payloads are accepted. For arrays use this module's [`array`] adapter;
for a decoded-byte limit use [`deserialize_bounded`].

# Examples

```
#[derive(Debug, PartialEq, serde::Serialize, serde::Deserialize)]
struct Record {
    #[serde(with = "faster_hex::"###, stringify!($mod_name), r###"")]
    bytes: Vec<u8>,
}
let record = Record { bytes: vec![0x12, 0x34] };
let json = serde_json::to_string(&record)?;
assert_eq!(json, r#"{"bytes":""###, $prefix, r###"1234"}"#);
assert_eq!(serde_json::from_str::<Record>(&json)?, record);
# Ok::<(), serde_json::Error>(())
```
"###
                                )]
        #[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
        pub mod $mod_name {
            use crate::decode::CheckCase;
            use crate::serde::internal;
            use core::iter::FromIterator;

            /// Serializes a byte view using this module's prefix and case policy.
            ///
            /// Reads [`AsRef::as_ref`] once and sends a string to every serializer.
            /// Allocation behavior matches [`crate::serialize`].
            ///
            /// # Errors
            ///
            /// Returns an error if serialization fails or the encoded length overflows.
            ///
            /// # Panics
            ///
            /// Panics if temporary output storage would exceed [`isize::MAX`] bytes.
            ///
            /// # Examples
            ///
            /// See the [module example](self#examples).
            pub fn serialize<S, T>(data: T, serializer: S) -> Result<S::Ok, S::Error>
            where
                S: serde::Serializer,
                T: AsRef<[u8]>,
            {
                internal::serialize(data, serializer, $with_pfx, $check_case)
            }

            /// Deserializes a hex string using this module's prefix and case policy.
            ///
            /// Borrows input text when the format can lend it.
            /// Allocation behavior matches [`crate::deserialize`].
            ///
            /// # Errors
            ///
            /// Propagates deserializer errors. Checks any required prefix, even payload
            /// length, then characters/case before collection. Positions exclude the prefix.
            ///
            /// # Panics
            ///
            /// A custom collector may panic, for example when its fixed capacity is exceeded.
            ///
            /// # Examples
            ///
            /// See the [module example](self#examples).
            pub fn deserialize<'de, D, T>(deserializer: D) -> Result<T, D::Error>
            where
                D: serde::Deserializer<'de>,
                T: FromIterator<u8>,
            {
                internal::deserialize(deserializer, $with_pfx, $check_case, usize::MAX)
            }

            /// Deserializes at most `MAX` decoded bytes using this module's policy.
            ///
            /// The limit precedes decoded-output allocation and collection; it does not
            /// bound input, error or collector storage. `MAX == 0` accepts an empty payload.
            /// Allocation behavior matches [`crate::deserialize_bounded`].
            ///
            /// # Errors
            ///
            /// Propagates deserializer errors. Checks any required prefix, even length,
            /// decoded-byte limit, then characters/case. Byte positions exclude the prefix.
            ///
            /// # Panics
            ///
            /// A custom collector may still panic; the limit does not change its capacity.
            ///
            /// # Examples
            ///
            /// See [`crate::deserialize_bounded`] for the Serde field attributes.
            pub fn deserialize_bounded<'de, const MAX: usize, D, T>(
                deserializer: D,
            ) -> Result<T, D::Error>
            where
                D: serde::Deserializer<'de>,
                T: FromIterator<u8>,
            {
                internal::deserialize(deserializer, $with_pfx, $check_case, MAX)
            }

            /// Exact-length arrays using the parent module's prefix and case policy.
            ///
            /// Deserialization writes into `[u8; N]` without an intermediate byte vector.
            /// Input text is borrowed where possible; formats may need storage for
            /// escaped or transient text. Serialization uses the parent adapter's
            /// string representation, including any framing added by the format.
            #[doc = concat!(
                                        r###"
# Examples

```
#[derive(Debug, PartialEq, serde::Serialize, serde::Deserialize)]
struct Record {
    #[serde(with = "faster_hex::"###, stringify!($mod_name), r###"::array")]
    id: [u8; 2],
}
let record = Record { id: [0x12, 0x34] };
let json = serde_json::to_string(&record)?;
assert_eq!(serde_json::from_str::<Record>(&json)?, record);
# Ok::<(), serde_json::Error>(())
```
"###
                                    )]
            pub mod array {
                use super::{internal, CheckCase};

                pub use super::serialize;

                /// Deserializes exactly `N` bytes into an owned array.
                ///
                /// Empty payloads succeed only for `N == 0`.
                ///
                /// # Errors
                ///
                /// Checks any required prefix, even payload length, exact decoded length,
                /// then characters/case. Length errors count decoded bytes; invalid-byte
                /// positions exclude the prefix. Format errors propagate.
                ///
                /// # Examples
                ///
                /// See the [module example](self#examples).
                pub fn deserialize<'de, D, const N: usize>(
                    deserializer: D,
                ) -> Result<[u8; N], D::Error>
                where
                    D: serde::Deserializer<'de>,
                {
                    internal::deserialize_array(deserializer, $with_pfx, $check_case)
                }
            }
        }

        #[doc = concat!($description, " Optional values.")]
        #[doc = concat!(
                                    r###"
Present values follow [`"###, stringify!($mod_name), r###"`](crate::"###,
                                    stringify!($mod_name), r###"). Absent values use Serde's `None`
representation (`null` in JSON). Binary formats retain their normal [`Option`]
tags, so `Some(empty)` remains distinct from `None`.

Use `#[serde(default)]` to accept a missing struct field. For fixed arrays use
[`array`]; for a limit on present values use [`deserialize_bounded`].

# Examples

```
#[derive(Debug, PartialEq, serde::Serialize, serde::Deserialize)]
struct Record {
    #[serde(default, with = "faster_hex::"###, stringify!($option_name), r###"")]
    bytes: Option<Vec<u8>>,
}
let record = Record { bytes: Some(vec![0x12, 0x34]) };
let json = serde_json::to_string(&record)?;
assert_eq!(serde_json::from_str::<Record>(&json)?, record);
assert_eq!(serde_json::from_str::<Record>("{}")?.bytes, None);
# Ok::<(), serde_json::Error>(())
```
"###
                                )]
        #[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
        pub mod $option_name {
            use crate::decode::CheckCase;
            use crate::serde::internal;
            use core::iter::FromIterator;

            /// Serializes an optional byte view, preserving Serde's [`Option`] tags.
            ///
            /// Present values use this module's hex string policy and read
            /// [`AsRef::as_ref`] once. `None` uses the format's absent-value representation.
            /// Allocation behavior matches [`crate::serialize`].
            ///
            /// # Errors
            ///
            /// Propagates serializer errors, including errors writing an Option tag.
            /// Present values also fail if their encoded length overflows.
            ///
            /// # Panics
            ///
            /// Panics if temporary output storage would exceed [`isize::MAX`] bytes.
            ///
            /// # Examples
            ///
            /// See the [module example](self#examples).
            pub fn serialize<S, T>(data: &Option<T>, serializer: S) -> Result<S::Ok, S::Error>
            where
                S: serde::Serializer,
                T: AsRef<[u8]>,
            {
                internal::serialize_option(data, serializer, $with_pfx, $check_case)
            }

            /// Deserializes an optional hex string into a byte collection.
            ///
            /// Present strings use this module's policy; an empty payload is `Some(empty)`.
            /// Allocation behavior matches [`crate::deserialize`].
            ///
            /// # Errors
            ///
            /// Propagates deserializer errors. Checks present text for any required prefix,
            /// even payload length, then characters/case before collection. Invalid input
            /// is an error, not `None`. Byte positions exclude the prefix.
            ///
            /// # Panics
            ///
            /// A custom collector may panic, for example when its fixed capacity is exceeded.
            ///
            /// # Examples
            ///
            /// See the [module example](self#examples).
            pub fn deserialize<'de, D, T>(deserializer: D) -> Result<Option<T>, D::Error>
            where
                D: serde::Deserializer<'de>,
                T: FromIterator<u8>,
            {
                internal::deserialize_option(deserializer, $with_pfx, $check_case, usize::MAX)
            }

            /// Deserializes an optional collection of at most `MAX` decoded bytes.
            ///
            /// Accepts `None` and empty payloads even when `MAX == 0`. Storage limits
            /// and allocation behavior match [`crate::deserialize_bounded`].
            ///
            /// # Errors
            ///
            /// Propagates deserializer errors. Checks present text for any required prefix,
            /// even payload length, decoded-byte limit, then characters/case. Invalid or
            /// over-limit values are errors, not `None`. Byte positions exclude the prefix.
            ///
            /// # Panics
            ///
            /// A custom collector may still panic; the limit does not change its capacity.
            ///
            /// # Examples
            ///
            /// See [`crate::deserialize_bounded`] for the Serde field attributes.
            pub fn deserialize_bounded<'de, const MAX: usize, D, T>(
                deserializer: D,
            ) -> Result<Option<T>, D::Error>
            where
                D: serde::Deserializer<'de>,
                T: FromIterator<u8>,
            {
                internal::deserialize_option(deserializer, $with_pfx, $check_case, MAX)
            }

            /// Optional fixed-length arrays using this module's prefix and case policy.
            ///
            /// Present strings decode directly into `[u8; N]` without an intermediate
            /// byte vector; input text may still need storage. `None` and `Some([])`
            /// remain distinct in every format. Add `#[serde(default)]` for missing fields.
            ///
            /// # Examples
            ///
            /// See the [crate example](crate#serde-adapters) for optional array fields.
            pub mod array {
                use super::{internal, CheckCase};

                pub use super::serialize;

                /// Deserializes an optional array of exactly `N` bytes.
                ///
                /// An empty present payload requires `N == 0`.
                ///
                /// # Errors
                ///
                /// Checks present text for any required prefix, even payload length, exact
                /// decoded length, then characters/case. Format errors propagate; invalid input
                /// is an error, not `None`.
                /// Lengths count decoded bytes; invalid-byte positions exclude the prefix.
                ///
                /// # Examples
                ///
                /// See the [crate example](crate#serde-adapters) for optional array fields.
                pub fn deserialize<'de, D, const N: usize>(
                    deserializer: D,
                ) -> Result<Option<[u8; N]>, D::Error>
                where
                    D: serde::Deserializer<'de>,
                {
                    internal::deserialize_option_array(deserializer, $with_pfx, $check_case)
                }
            }
        }
    };
}

serde_adapters!(
    withpfx_ignorecase,
    option_withpfx_ignorecase,
    true,
    CheckCase::None,
    "0x",
    "Lowercase serialization with a 0x prefix; accepts either letter case."
);
serde_adapters!(
    nopfx_ignorecase,
    option_nopfx_ignorecase,
    false,
    CheckCase::None,
    "",
    "Lowercase serialization without a prefix; accepts either letter case."
);
serde_adapters!(
    withpfx_lowercase,
    option_withpfx_lowercase,
    true,
    CheckCase::Lower,
    "0x",
    "Lowercase hex with a required 0x prefix."
);
serde_adapters!(
    nopfx_lowercase,
    option_nopfx_lowercase,
    false,
    CheckCase::Lower,
    "",
    "Lowercase hex without a prefix."
);
serde_adapters!(
    withpfx_uppercase,
    option_withpfx_uppercase,
    true,
    CheckCase::Upper,
    "0x",
    "Uppercase hex with a required 0x prefix."
);
serde_adapters!(
    nopfx_uppercase,
    option_nopfx_uppercase,
    false,
    CheckCase::Upper,
    "",
    "Uppercase hex without a prefix."
);
