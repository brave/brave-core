//! A module for encoding and decoding Modified UTF-8 (MUTF-8), a variant of
//! CESU-8 that encodes the null character (`0x00`) as two bytes: `0xc0` and
//! `0x80`. In general, this module is nearly identical to the functions found
//! in the root of this crate.

use alloc::borrow::Cow;

use simdutf8::basic::from_utf8;

use crate::error::DecodingError;
use crate::implementation::active::contains_null_or_utf8_4_byte_char_header;
use crate::internal;
use crate::internal::{DecodeOptions, Flavor};

/// Converts a slice of bytes to a string, including invalid characters.
///
/// The algorithm is as follows:
///
/// - If the input is valid MUTF-8, but also valid UTF-8, the function will
///   return <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid MUTF-8, but invalid UTF-8, the function will return
///   <code>[Cow::Owned]\([String]\)</code>. This case has the potential to
///   panic.
/// - If the input is not valid MUTF-8, the function will return
///   <code>[Cow::Owned]\([String]\)</code>, where the best attempt at decoding
///   the input strictly as MUTF-8 will be made, with any invalid bytes being
///   replaced with the [U+FFFD REPLACEMENT CHARACTER] (�). This case has the
///   potential to panic.
///
/// **NOTE:** This function is significantly slower than [`decode_lossy`], but
/// is more "correct" in the sense that valid UTF-8 that is not valid MUTF-8
/// will be treated as invalid input and properly replaced with the [U+FFFD
/// REPLACEMENT CHARACTER] (�). If you don't need this guarantee, and can
/// tolerate valid UTF-8 that is not valid MUTF-8, use [`decode_lossy`] instead.
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
/// If the input is valid MUTF-8, but also valid UTF-8, the function will return
/// a borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// use simd_cesu8::mutf8;
///
/// let bytes = b"Hello, world!";
/// let decoded = mutf8::decode_lossy_strict(bytes);
/// assert_eq!(decoded, Cow::Borrowed("Hello, world!"));
/// ```
///
/// If the input is valid MUTF-8, but invalid UTF-8, the function will return an
/// owned string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// use simd_cesu8::mutf8;
///
/// let bytes = [0xc0, 0x80];
/// let decoded = mutf8::decode_lossy_strict(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("\0")));
/// ```
///
/// If the input is not valid MUTF-8, the function will return an owned string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// use simd_cesu8::mutf8;
///
/// // NOTE: This is the first half of MUTF-8's byte pair followed by a null
/// // byte in MUTF-8.
/// let bytes = [0xc0, 0xc0, 0x80];
/// let decoded = mutf8::decode_lossy_strict(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("�\0")));
/// ```
///
/// Unlike [`decode_lossy`], this function will treat valid UTF-8 that is not
/// valid MUTF-8 as an invalid input:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// use simd_cesu8::mutf8;
///
/// let bytes = [0x00, 0x00, 0x00];
/// assert_eq!(core::str::from_utf8(&bytes), Ok("\0\0\0"));
///
/// let decoded = mutf8::decode_lossy_strict(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("���")));
/// ```
#[must_use]
#[inline]
pub fn decode_lossy_strict(bytes: &[u8]) -> Cow<'_, str> {
    if contains_null_or_utf8_4_byte_char_header(bytes) || from_utf8(bytes).is_err() {
        let result = internal::decode(bytes, DecodeOptions {
            flavor: Flavor::Mutf8,
            lossy: true,
        });

        // SAFETY: When `lossy` is `true`, the function will always return a
        // valid string.
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
/// - If the input is valid UTF-8, but invalid MUTF-8, the function will return
///   <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid MUTF-8, but also valid UTF-8, the function will
///   return <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid MUTF-8, but invalid UTF-8, the function will return
///   <code>[Cow::Owned]\([String]\)</code>. This case has the potential to
///   panic.
/// - If the input is not valid MUTF-8 or UTF-8, the function will return
///   <code>[Cow::Owned]\([String]\)</code>, where the best attempt at decoding
///   the input strictly as MUTF-8 will be made, with any invalid bytes being
///   replaced with the [U+FFFD REPLACEMENT CHARACTER] (�). This case has the
///   potential to panic.
///
/// **NOTE:** The first case may be interpreted as an error, but it's actually
/// a significant performance optimization. If you need to strictly enforce
/// MUTF-8 decoding, use [`decode_lossy_strict`] instead.
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
/// If the input is valid UTF-8, but invalid MUTF-8, the function will return a
/// borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// use simd_cesu8::mutf8;
///
/// // NOTE: This is a valid UTF-8 string, but invalid MUTF-8.
/// let bytes = [0x00];
/// let decoded = mutf8::decode_lossy(&bytes);
/// assert_eq!(decoded, Cow::Borrowed("\0"));
/// ```
///
/// If the input is valid MUTF-8, but also valid UTF-8, the function will return
/// a borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// use simd_cesu8::mutf8;
///
/// let bytes = b"Hello, world!";
/// let decoded = mutf8::decode_lossy(bytes);
/// assert_eq!(decoded, Cow::Borrowed("Hello, world!"));
/// ```
///
/// If the input is valid MUTF-8, but invalid UTF-8, the function will return an
/// owned string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// use simd_cesu8::mutf8;
///
/// let bytes = [0xc0, 0x80];
/// let decoded = mutf8::decode_lossy(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("\0")));
/// ```
///
/// If the input is not valid MUTF-8 or UTF-8, the function will return an owned
/// string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// use simd_cesu8::mutf8;
///
/// // NOTE: This is the first half of MUTF-8's byte pair followed by a null
/// // byte in MUTF-8.
/// let bytes = [0xc0, 0xc0, 0x80];
/// let decoded = mutf8::decode_lossy(&bytes);
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("�\0")));
/// ```
#[must_use]
#[inline]
pub fn decode_lossy(bytes: &[u8]) -> Cow<'_, str> {
    if let Ok(string) = from_utf8(bytes) {
        Cow::Borrowed(string)
    } else {
        let result = internal::decode(bytes, DecodeOptions {
            flavor: Flavor::Mutf8,
            lossy: true,
        });

        // SAFETY: When `lossy` is `true`, the function will always return a
        // valid string.
        let string = unsafe { result.unwrap_unchecked() };

        Cow::Owned(string)
    }
}

/// Converts a slice of bytes to a string.
///
/// The algorithm is as follows:
///
/// - If the input is valid MUTF-8, but also valid UTF-8, the function will
///   return <code>[Cow::Borrowed]\(&[str]\)</code>.
/// - If the input is valid MUTF-8, but invalid UTF-8, the function will return
///   <code>[Cow::Owned]\([String]\)</code>. This case has the potential to
///   panic.
///
/// # Errors
///
/// If the input is not valid MUTF-8, the function will return a
/// [`DecodingError`].
///
/// # Panics
///
/// This function will panic if the buffer required to decode the input exceeds
/// [`isize::MAX`] bytes.
///
/// # Examples
///
/// If the input is valid MUTF-8, but also valid UTF-8, the function will return
/// a borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// use simd_cesu8::mutf8;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// let bytes = b"Hello, world!";
/// let decoded = mutf8::decode_strict(bytes)?;
/// assert_eq!(decoded, Cow::Borrowed("Hello, world!"));
/// # Ok(())
/// # }
/// ```
///
/// If the input is valid MUTF-8, but invalid UTF-8, the function will return an
/// owned string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// use simd_cesu8::mutf8;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// let bytes = [0xc0, 0x80];
/// let decoded = mutf8::decode_strict(&bytes)?;
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("\0")));
/// # Ok(())
/// # }
/// ```
///
/// Unlike [`decode`], this function will treat valid UTF-8 that is not valid
/// MUTF-8 as an invalid input:
///
/// ```
/// use simd_cesu8::mutf8;
///
/// let bytes = [0x00];
/// assert_eq!(core::str::from_utf8(&bytes), Ok("\0"));
///
/// let result = mutf8::decode_strict(&bytes);
/// assert!(result.is_err());
/// ```
#[inline]
pub fn decode_strict(bytes: &[u8]) -> Result<Cow<'_, str>, DecodingError> {
    if contains_null_or_utf8_4_byte_char_header(bytes) || from_utf8(bytes).is_err() {
        let string = internal::decode(bytes, DecodeOptions {
            flavor: Flavor::Mutf8,
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
/// - If the input is valid UTF-8, but invalid MUTF-8, the function will return
///   <code>[Cow::Borrowed]\([&str\])</code>.
/// - If the input is valid MUTF-8, but also valid UTF-8, the function will
///   return <code>[Cow::Borrowed]\([&str\])</code>.
/// - If the input is valid MUTF-8, but invalid UTF-8, the function will return
///   <code>[Cow::Owned]\([String]\)</code>. This case has the potential to
///   panic.
///
/// **NOTE:** The first case might be interpreted as an error, but it's actually
/// a significant performance optimization. If you need to strictly enforce
/// MUTF-8 decoding, use [`decode_strict`] instead.
///
/// # Errors
///
/// If the input is not valid MUTF-8 or UTF-8, the function will return a
/// [`DecodingError`].
///
/// # Panics
///
/// This function will panic if the buffer required to decode the input exceeds
/// [`isize::MAX`] bytes.
///
/// # Examples
///
/// If the input is valid UTF-8, but invalid MUTF-8, the function will return
/// a borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// use simd_cesu8::mutf8;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// // NOTE: This is valid UTF-8, but invalid MUTF-8.
/// let bytes = [0x00];
/// let decoded = mutf8::decode(&bytes)?;
/// assert_eq!(decoded, Cow::Borrowed("\0"));
/// # Ok(())
/// # }
/// ```
///
/// If the input is valid MUTF-8, but also valid UTF-8, the function will return
/// a borrowed string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
///
/// use simd_cesu8::mutf8;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// let bytes = b"Hello, world!";
/// let decoded = mutf8::decode(bytes)?;
/// assert_eq!(decoded, Cow::Borrowed("Hello, world!"));
/// # Ok(())
/// # }
/// ```
///
/// If the input is valid MUTF-8, but invalid UTF-8, the function will return an
/// owned string:
///
/// ```
/// # extern crate alloc;
/// use alloc::borrow::Cow;
/// use alloc::string::String;
///
/// use simd_cesu8::mutf8;
///
/// # fn main() -> Result<(), simd_cesu8::DecodingError> {
/// let bytes = [0xc0, 0x80];
/// let decoded = mutf8::decode(&bytes)?;
/// assert_eq!(decoded, Cow::<str>::Owned(String::from("\0")));
/// # Ok(())
/// # }
/// ```
#[inline]
pub fn decode(bytes: &[u8]) -> Result<Cow<'_, str>, DecodingError> {
    if let Ok(value) = from_utf8(bytes) {
        Ok(Cow::Borrowed(value))
    } else {
        let string = internal::decode(bytes, DecodeOptions {
            flavor: Flavor::Mutf8,
            lossy: false,
        })?;

        Ok(Cow::Owned(string))
    }
}

/// Encodes a string to MUTF-8.
///
/// The algorithm is as follows:
///
/// - If the input, as UTF-8, is also valid MUTF-8, the function will return
///   <code>[Cow::Borrowed]\([&\[u8\]][slice]\)</code>.
/// - If the input, as UTF-8, is not valid MUTF-8, the function will return
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
/// use simd_cesu8::mutf8;
///
/// let null = "\0";
/// assert_eq!(null.len(), 1);
/// assert_eq!(null.as_bytes(), &[0x00]);
/// assert_eq!(mutf8::encode(null), Cow::<[u8]>::Owned(vec![0xc0, 0x80]));
///
/// let single_byte  = "\u{0045}";
/// assert_eq!(single_byte, "E");
/// assert_eq!(single_byte.len(), 1);
/// assert_eq!(single_byte.as_bytes(), &[0x45]);
/// assert_eq!(mutf8::encode(single_byte), Cow::Borrowed(&[0x45]));
///
/// let two_bytes = "\u{0205}";
/// assert_eq!(two_bytes, "ȅ");
/// assert_eq!(two_bytes.len(), 2);
/// assert_eq!(two_bytes.as_bytes(), &[0xc8, 0x85]);
/// assert_eq!(mutf8::encode(two_bytes), Cow::Borrowed(&[0xc8, 0x85]));
///
/// let three_bytes = "\u{20ac}";
/// assert_eq!(three_bytes, "€");
/// assert_eq!(three_bytes.len(), 3);
/// assert_eq!(three_bytes.as_bytes(), &[0xe2, 0x82, 0xac]);
/// assert_eq!(
///     mutf8::encode(three_bytes),
///     Cow::Borrowed(&[0xe2, 0x82, 0xac])
/// );
///
/// let four_bytes = "\u{10400}";
/// assert_eq!(four_bytes, "𐐀");
/// assert_eq!(four_bytes.len(), 4);
/// assert_eq!(four_bytes.as_bytes(), &[0xf0, 0x90, 0x90, 0x80]);
/// assert_eq!(
///     mutf8::encode(four_bytes),
///     Cow::<[u8]>::Owned(vec![0xed, 0xa0, 0x81, 0xed, 0xb0, 0x80])
/// );
#[must_use]
#[inline]
pub fn encode(value: &str) -> Cow<'_, [u8]> {
    if contains_null_or_utf8_4_byte_char_header(value.as_bytes()) {
        Cow::Owned(internal::encode(value, Flavor::Mutf8))
    } else {
        Cow::Borrowed(value.as_bytes())
    }
}

/// Returns `true` if the input string needs to be encoded to MUTF-8.
///
/// # Examples
///
/// ```
/// use simd_cesu8::mutf8;
///
/// let null = "\0";
/// assert_eq!(null.len(), 1);
/// assert!(mutf8::needs_encoded(null));
///
/// let single_byte = "E";
/// assert_eq!(single_byte.len(), 1);
/// assert!(!mutf8::needs_encoded(single_byte));
///
/// let two_bytes = "ȅ";
/// assert_eq!(two_bytes.len(), 2);
/// assert!(!mutf8::needs_encoded(two_bytes));
///
/// let three_bytes = "€";
/// assert_eq!(three_bytes.len(), 3);
/// assert!(!mutf8::needs_encoded(three_bytes));
///
/// let four_bytes = "𐐀";
/// assert_eq!(four_bytes.len(), 4);
/// assert!(mutf8::needs_encoded(four_bytes));
/// ```
#[must_use]
#[inline]
pub fn needs_encoded(value: &str) -> bool {
    contains_null_or_utf8_4_byte_char_header(value.as_bytes())
}
