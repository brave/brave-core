#![doc = include_str!("README.md")]
#![cfg_attr(not(feature = "std"), no_std)]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(feature = "nightly", feature(portable_simd))]
#![allow(stable_features)]

extern crate alloc;

mod error;
#[doc(hidden)]
pub mod implementation;
mod internal;
pub mod mutf8;

use alloc::borrow::Cow;
use alloc::vec::Vec;

use simdutf8::basic::from_utf8;

pub use self::error::DecodingError;
use self::implementation::active::contains_utf8_4_byte_char_header;
use self::internal::{DecodeOptions, Flavor};

/// Converts a slice of bytes to a string, including invalid characters.
///
/// The algorithm is as follows:
///
/// - If the input is valid UTF-8, but also valid CESU-8, the function will
///   return <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid CESU-8, but not valid UTF-8, the function will
///   return <code>[Cow::Owned]\([String]\)</code>. This case has the potential
///   to panic.
/// - If the input is not valid CESU-8, the function will return
///   <code>[Cow::Owned]\([String]\)</code>, where the best attempt at decoding
///   the input strictly as CESU-8 will be made, with any invalid bytes being
///   replaced with the [U+FFFD REPLACEMENT CHARACTER] (�). This case has the
///   potential to panic.
///
/// **NOTE:** This function is significantly slower than [`decode_lossy`], but
/// is more "correct" in the sense that valid UTF-8 that is not valid CESU-8
/// will be treated as invalid input and properly replaced with the [U+FFFD
/// REPLACEMENT CHARACTER] (�). If you don't need this guarantee, and can
/// tolerate valid UTF-8 that is not valid CESU-8, use [`decode_lossy`] instead.
///
/// [U+FFFD REPLACEMENT CHARACTER]: char::REPLACEMENT_CHARACTER
///
/// # Panics
///
/// This function will panic if the buffer required to decode the input exceeds
/// [`isize::MAX`] bytes.
///
/// # Examples
///
/// If the input is valid UTF-8, but also valid CESU-8, the function will return
/// a borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// let bytes = b"Hello, world!";
/// let decoded = simd_cesu8::decode_lossy_strict(bytes);
/// assert_eq!(decoded, Cow::Borrowed("Hello, world!"));
/// ```
///
/// If the input is valid CESU-8, but not valid UTF-8, the function will return
/// an owned Rust string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// let bytes = [0xed, 0xa0, 0xbd, 0xed, 0xb2, 0x96];
/// let decoded = simd_cesu8::decode_lossy_strict(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("💖")));
/// ```
///
/// If the input is not valid CESU-8, the function will return an owned Rust
/// string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// // NOTE: This is an unpaired surrogate followed by a valid CESU-8 surrogate
/// // pair.
/// let bytes = [0xed, 0xa0, 0xbd, 0xed, 0xa0, 0xbd, 0xed, 0xb2, 0x96];
/// let decoded = simd_cesu8::decode_lossy_strict(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("���💖")));
/// ```
///
/// Unlike [`decode_lossy`], this function will treat valid UTF-8 that is not
/// valid CESU-8 as invalid input:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// // NOTE: This is a valid UTF-8 string, but not valid CESU-8.
/// let bytes = [0xf0, 0x9f, 0x92, 0x96];
/// assert_eq!(core::str::from_utf8(&bytes), Ok("💖"));
/// let decoded = simd_cesu8::decode_lossy_strict(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("����")));
/// ```
#[must_use]
#[inline]
pub fn decode_lossy_strict(bytes: &[u8]) -> Cow<'_, str> {
    if contains_utf8_4_byte_char_header(bytes) || from_utf8(bytes).is_err() {
        let result = internal::decode(bytes, DecodeOptions {
            flavor: Flavor::Cesu8,
            lossy: true,
        });

        // SAFETY: If `lossy` is `true`, the function will always return a valid
        // string.
        let string = unsafe { result.unwrap_unchecked() };

        Cow::Owned(string)
    } else {
        // SAFETY: We know that `bytes` is a valid UTF-8 string.
        Cow::Borrowed(unsafe { core::str::from_utf8_unchecked(bytes) })
    }
}

/// Converts a slice of bytes to a string, including invalid characters.
///
/// The algorithm is as follows:
///
/// - If the input is valid UTF-8, but invalid CESU-8, the function will return
///   <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid CESU-8, but also valid UTF-8, the function will
///   return <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid CESU-8, but not valid UTF-8, the function will
///   return <code>[Cow::Owned]\([String]\)</code>. This case has the potential
///   to panic.
/// - If the input is not valid CESU-8 or UTF-8, the function will return
///   <code>[Cow::Owned]\([String]\)</code>, where the best attempt at decoding
///   the input strictly as CESU-8 will be made, with any invalid bytes being
///   replaced with the [U+FFFD REPLACEMENT CHARACTER] (�). This case has the
///   potential to panic.
///
/// **NOTE:** The first case may be interpreted as an error, but it's actually
/// a significant performance optimization. If you need to strictly enforce
/// CESU-8 decoding, use [`decode_lossy_strict`] instead.
///
/// [U+FFFD REPLACEMENT CHARACTER]: char::REPLACEMENT_CHARACTER
///
/// # Panics
///
/// This function will panic if the buffer required to decode the input exceeds
/// [`isize::MAX`] bytes.
///
/// # Examples
///
/// If the input is valid UTF-8, but invalid CESU-8, the function will return a
/// borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// // NOTE: This is a valid UTF-8 string, but not valid CESU-8.
/// let bytes = [0xf0, 0x9f, 0x92, 0x96];
/// let decoded = simd_cesu8::decode_lossy(&bytes);
/// assert_eq!(decoded, Cow::Borrowed("💖"));
/// ```
///
/// If the input is valid CESU-8, but also valid UTF-8, the function will return
/// a borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// let bytes = b"Hello, world!";
/// let decoded = simd_cesu8::decode_lossy(bytes);
/// assert_eq!(decoded, Cow::Borrowed("Hello, world!"));
/// ```
///
/// If the input is valid CESU-8, but not valid UTF-8, the function will return
/// an owned Rust string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// let bytes = [0xed, 0xa0, 0xbd, 0xed, 0xb2, 0x96];
/// let decoded = simd_cesu8::decode_lossy(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("💖")));
/// ```
///
/// If the input is not valid CESU-8 or UTF-8, the function will return an owned
/// Rust string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// // NOTE: This is an unpaired surrogate followed by a valid CESU-8 surrogate
/// // pair.
/// let bytes = [0xed, 0xa0, 0xbd, 0xed, 0xa0, 0xbd, 0xed, 0xb2, 0x96];
/// let decoded = simd_cesu8::decode_lossy(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("���💖")));
/// ```
#[must_use]
#[inline]
pub fn decode_lossy(bytes: &[u8]) -> Cow<'_, str> {
    if let Ok(string) = from_utf8(bytes) {
        Cow::Borrowed(string)
    } else {
        let result = internal::decode(bytes, DecodeOptions {
            flavor: Flavor::Cesu8,
            lossy: true,
        });

        // SAFETY: If `lossy` is `true`, the function will always return a valid
        // string.
        let string = unsafe { result.unwrap_unchecked() };

        Cow::Owned(string)
    }
}

/// Converts a slice of bytes to a string.
///
/// The algorithm is as follows:
///
/// - If the input is valid CESU-8, but also valid UTF-8, the function will
///   return <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid CESU-8, but not valid UTF-8, the function will
///   return <code>[Cow::Owned]\([String]\)</code>. This case has the potential
///   to panic.
///
/// # Errors
///
/// If the input is not valid CESU-8, this function will return a
/// [`DecodingError`].
///
/// # Panics
///
/// This function will panic if the buffer required to decode the input exceeds
/// [`isize::MAX`] bytes.
///
/// # Examples
///
/// If the input is valid CESU-8, but also valid UTF-8, the function will return
/// a borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// let bytes = b"Hello, world!";
/// let decoded = simd_cesu8::decode_strict(bytes)?;
/// assert_eq!(decoded, Cow::Borrowed("Hello, world!"));
/// # Ok(())
/// # }
/// ```
///
/// If the input is valid CESU-8, but not valid UTF-8, the function will return
/// an owned Rust string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// let bytes = [0xed, 0xa0, 0xbd, 0xed, 0xb2, 0x96];
/// let decoded = simd_cesu8::decode_strict(&bytes)?;
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("💖")));
/// # Ok(())
/// # }
/// ```
///
/// Unlike [`decode`], this function will treat valid UTF-8 that is not valid
/// CESU-8 as invalid input:
///
/// ```
/// // NOTE: This is a valid UTF-8 string, but not valid CESU-8.
/// let bytes = [0xf0, 0x9f, 0x92, 0x96];
/// assert_eq!(core::str::from_utf8(&bytes), Ok("💖"));
///
/// let result = simd_cesu8::decode_strict(&bytes);
/// assert!(result.is_err());
/// ```
#[inline]
pub fn decode_strict(bytes: &[u8]) -> Result<Cow<'_, str>, DecodingError> {
    if contains_utf8_4_byte_char_header(bytes) || from_utf8(bytes).is_err() {
        let string = internal::decode(bytes, DecodeOptions {
            flavor: Flavor::Cesu8,
            lossy: false,
        })?;

        Ok(Cow::Owned(string))
    } else {
        // SAFETY: We know that `bytes` is a valid UTF-8 string.
        let string = unsafe { core::str::from_utf8_unchecked(bytes) };
        Ok(Cow::Borrowed(string))
    }
}

/// Converts a slice of bytes to a string.
///
/// The algorithm is as follows:
///
/// - If the input is valid UTF-8, but invalid CESU-8, the function will return
///   <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid CESU-8, but also valid UTF-8, the function will
///   return <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid CESU-8, but not valid UTF-8, the function will
///   return <code>[Cow::Owned]\([String]\)</code>. This case has the potential
///   to panic.
///
/// **NOTE:** The first case may be interpreted as an error, but it's actually
/// a significant performance optimization. If you need to strictly enforce
/// CESU-8 decoding, use [`decode_strict`] instead.
///
/// # Errors
///
/// If the input is not valid CESU-8 or UTF-8, this function will return a
/// [`DecodingError`].
///
/// # Panics
///
/// This function will panic if the buffer required to decode the input exceeds
/// [`isize::MAX`] bytes.
///
/// # Examples
///
/// If the input is valid UTF-8, but invalid CESU-8, the function will return a
/// borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// // NOTE: This is a valid UTF-8 string, but not valid CESU-8.
/// let bytes = [0xf0, 0x9f, 0x92, 0x96];
/// let decoded = simd_cesu8::decode(&bytes)?;
/// assert_eq!(decoded, Cow::Borrowed("💖"));
/// # Ok(())
/// # }
/// ```
///
/// If the input is valid CESU-8, but also valid UTF-8, the function will return
/// a borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// let bytes = b"Hello, world!";
/// let decoded = simd_cesu8::decode(bytes)?;
/// assert_eq!(decoded, Cow::Borrowed("Hello, world!"));
/// # Ok(())
/// # }
/// ```
///
/// If the input is valid CESU-8, but not valid UTF-8, the function will return
/// an owned Rust string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// let bytes = [0xed, 0xa0, 0xbd, 0xed, 0xb2, 0x96];
/// let decoded = simd_cesu8::decode(&bytes)?;
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("💖")));
/// # Ok(())
/// # }
/// ```
#[inline]
pub fn decode(bytes: &[u8]) -> Result<Cow<'_, str>, DecodingError> {
    if let Ok(string) = from_utf8(bytes) {
        Ok(Cow::Borrowed(string))
    } else {
        let string = internal::decode(bytes, DecodeOptions {
            flavor: Flavor::Cesu8,
            lossy: false,
        })?;

        Ok(Cow::Owned(string))
    }
}

/// Encodes a string to CESU-8.
///
/// The algorithm is as follows:
///
/// - If the input, as UTF-8, is also valid CESU-8, the function will return
///   <code>[Cow::Borrowed]\([&\[u8\]][slice]\)</code>.
/// - If the input, as UTF-8, is not valid CESU-8, the function will return
///   <code>[Cow::Owned]\([Vec]<[u8]>\)</code>. This case has the potential to
///   panic.
///
/// # Panics
///
/// This function will panic if the buffer required to encode the input exceeds
/// [`isize::MAX`] bytes.
///
/// # Examples
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// let single_byte = "\u{0045}";
/// assert_eq!(single_byte, "E");
/// assert_eq!(single_byte.len(), 1);
/// assert_eq!(single_byte.as_bytes(), &[0x45]);
/// assert_eq!(simd_cesu8::encode(single_byte), Cow::Borrowed(&[0x45]));
///
/// let two_bytes = "\u{0205}";
/// assert_eq!(two_bytes, "ȅ");
/// assert_eq!(two_bytes.len(), 2);
/// assert_eq!(two_bytes.as_bytes(), &[0xc8, 0x85]);
/// assert_eq!(simd_cesu8::encode(two_bytes), Cow::Borrowed(&[0xc8, 0x85]));
///
/// let three_bytes = "\u{20ac}";
/// assert_eq!(three_bytes, "€");
/// assert_eq!(three_bytes.len(), 3);
/// assert_eq!(three_bytes.as_bytes(), &[0xe2, 0x82, 0xac]);
/// assert_eq!(
///     simd_cesu8::encode(three_bytes),
///     Cow::Borrowed(&[0xe2, 0x82, 0xac])
/// );
///
/// let four_bytes = "\u{10400}";
/// assert_eq!(four_bytes, "𐐀");
/// assert_eq!(four_bytes.len(), 4);
/// assert_eq!(four_bytes.as_bytes(), &[0xf0, 0x90, 0x90, 0x80]);
/// assert_eq!(
///     simd_cesu8::encode(four_bytes),
///     Cow::<[u8]>::Owned(vec![0xed, 0xa0, 0x81, 0xed, 0xb0, 0x80])
/// );
/// ```
#[must_use]
#[inline]
pub fn encode(value: &str) -> Cow<'_, [u8]> {
    if needs_encoded(value) {
        Cow::Owned(internal::encode(value, Flavor::Cesu8))
    } else {
        Cow::Borrowed(value.as_bytes())
    }
}

/// Encodes a string to CESU-8 into a provided vector of bytes.
///
/// The algorithm is as follows:
///
/// - If the input, as UTF-8, is also valid CESU-8, the function will copy the
///   input into the buffer.
/// - If the input, as UTF-8, is not valid CESU-8, the function will encode the
///   input into the buffer. This case has the potential to panic.
///
/// # Panics
///
/// This function will panic if the total buffer, including any existing bytes,
/// required to encode the input exceeds [`isize::MAX`] bytes.
///
/// # Examples
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// let mut buf = Vec::new();
///
/// let single_byte = "\u{0045}";
/// assert_eq!(single_byte, "E");
/// assert_eq!(single_byte.len(), 1);
/// assert_eq!(single_byte.as_bytes(), &[0x45]);
/// simd_cesu8::encode_into(single_byte, &mut buf);
/// assert_eq!(&buf, &[0x45]);
/// buf.clear();
///
/// let two_bytes = "\u{0205}";
/// assert_eq!(two_bytes, "ȅ");
/// assert_eq!(two_bytes.len(), 2);
/// assert_eq!(two_bytes.as_bytes(), &[0xc8, 0x85]);
/// simd_cesu8::encode_into(two_bytes, &mut buf);
/// assert_eq!(&buf, &[0xc8, 0x85]);
/// buf.clear();
///
/// let three_bytes = "\u{20ac}";
/// assert_eq!(three_bytes, "€");
/// assert_eq!(three_bytes.len(), 3);
/// assert_eq!(three_bytes.as_bytes(), &[0xe2, 0x82, 0xac]);
/// simd_cesu8::encode_into(three_bytes, &mut buf);
/// assert_eq!(&buf, &[0xe2, 0x82, 0xac]);
/// buf.clear();
///
/// let four_bytes = "\u{10400}";
/// assert_eq!(four_bytes, "𐐀");
/// assert_eq!(four_bytes.len(), 4);
/// assert_eq!(four_bytes.as_bytes(), &[0xf0, 0x90, 0x90, 0x80]);
/// simd_cesu8::encode_into(four_bytes, &mut buf);
/// assert_eq!(&buf, &[0xed, 0xa0, 0x81, 0xed, 0xb0, 0x80]);
/// buf.clear();
/// ```
#[inline]
pub fn encode_into(value: &str, buf: &mut Vec<u8>) {
    if needs_encoded(value) {
        internal::encode_into(value, Flavor::Cesu8, buf);
    } else {
        buf.extend_from_slice(value.as_bytes());
    }
}

/// Returns `true` if the input string needs to be encoded to CESU-8.
///
/// # Examples
///
/// ```
/// let one = "E";
/// assert_eq!(one.len(), 1);
/// assert!(!simd_cesu8::needs_encoded(one));
///
/// let two = "ȅ";
/// assert_eq!(two.len(), 2);
/// assert!(!simd_cesu8::needs_encoded(two));
///
/// let three = "€";
/// assert_eq!(three.len(), 3);
/// assert!(!simd_cesu8::needs_encoded(three));
///
/// let four = "𐐀";
/// assert_eq!(four.len(), 4);
/// assert!(simd_cesu8::needs_encoded(four));
/// ```
#[must_use]
#[inline]
pub fn needs_encoded(value: &str) -> bool {
    implementation::active::contains_utf8_4_byte_char_header(value.as_bytes())
}
