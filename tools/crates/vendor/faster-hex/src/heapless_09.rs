//! Fixed-capacity strings using `heapless` 0.9.
//!
//! Available with the `heapless-09` feature, independently of `alloc` or `std`.
//! These functions return [`heapless::String`] values with inline storage and
//! report insufficient capacity as an error. Capacity counts encoded ASCII
//! characters: storing `N` input bytes requires at least `2 * N` characters.
//!
//! # Examples
//!
//! ```
//! use faster_hex::heapless_09;
//!
//! let id = heapless_09::hex_string::<8>(&[0x12, 0xab, 0, 0xff])?;
//! assert_eq!(id.as_str(), "12ab00ff");
//! # Ok::<(), faster_hex::Error>(())
//! ```

use crate::{encode::hex_encode_custom, Error};
use heapless::{String, Vec};

/// Encodes `src` as a lowercase hexadecimal string with inline capacity `N`.
///
/// Every byte produces two ASCII digits, with no prefix or separators. The
/// result owns its storage without allocating. Empty input succeeds even when
/// `N` is zero. Any unused capacity remains available on the returned string.
///
/// # Errors
///
/// Returns [`Error::Overflow`] if twice the input length cannot be represented,
/// or [`Error::OutputTooSmall`] if `N` is less than the encoded byte length.
/// Insufficient capacity is an error rather than a panic.
///
/// # Examples
///
/// ```
/// use faster_hex::{heapless_09::hex_string, Error};
///
/// assert_eq!(hex_string::<4>(&[0xab, 1])?.as_str(), "ab01");
/// assert!(matches!(hex_string::<3>(&[0xab, 1]),
///     Err(Error::OutputTooSmall { required: 4, .. })));
/// assert_eq!(hex_string::<0>(&[])?.as_str(), "");
/// # Ok::<(), Error>(())
/// ```
pub fn hex_string<const N: usize>(src: &[u8]) -> Result<String<N>, Error> {
    hex_string_custom_case(src, false)
}

/// Encodes `src` as an uppercase hexadecimal string with inline capacity `N`.
///
/// This has the ownership and allocation-free behavior of [`hex_string`], using
/// `A` through `F` for letter digits. Capacity counts encoded characters.
///
/// # Errors
///
/// Returns [`Error::Overflow`] or [`Error::OutputTooSmall`] under the same
/// conditions as [`hex_string`].
///
/// # Examples
///
/// ```
/// assert_eq!(faster_hex::heapless_09::hex_string_upper::<4>(&[0xab, 1])?.as_str(),
///            "AB01");
/// # Ok::<(), faster_hex::Error>(())
/// ```
pub fn hex_string_upper<const N: usize>(src: &[u8]) -> Result<String<N>, Error> {
    hex_string_custom_case(src, true)
}

fn hex_string_custom_case<const N: usize>(src: &[u8], upper: bool) -> Result<String<N>, Error> {
    let len = src.len().checked_mul(2).ok_or(Error::Overflow)?;
    let mut buffer = Vec::<u8, N>::new();
    buffer
        .resize(len, 0)
        .map_err(|_| Error::OutputTooSmall { required: len })?;
    hex_encode_custom(src, &mut buffer, upper)?;
    // SAFETY: Every byte in the vector was encoded as ASCII hex.
    Ok(unsafe { String::from_utf8_unchecked(buffer) })
}
