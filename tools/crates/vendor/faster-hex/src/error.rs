/// An error from checked hexadecimal encoding or decoding.
///
/// Slice conversions leave the entire destination unchanged on error. Each
/// operation documents its validation order: for example, [`crate::hex_decode`]
/// checks odd input length, destination capacity, then characters. A character
/// check such as [`crate::hex_check`] returns a boolean instead of this type.
///
/// This enum and its data-bearing variants are non-exhaustive. Match fields
/// using `..` and keep a wildcard arm for future variants. Downstream crates
/// cannot construct the non-exhaustive data-bearing variants directly.
///
/// [`Display`](core::fmt::Display) and [`Debug`](core::fmt::Debug) provide
/// diagnostics; their output is not a stable serialization format. Match variants
/// and fields when handling errors programmatically.
/// [`core::error::Error`] is implemented with every feature set.
///
/// # Examples
///
/// ```
/// use faster_hex::{hex_decode, Error};
///
/// let mut destination = [0; 1];
/// let error = hex_decode(b"0011", &mut destination).unwrap_err();
/// let required = match error {
///     Error::OutputTooSmall { required, .. } => Some(required),
///     _ => None,
/// };
/// assert_eq!(required, Some(2));
/// ```
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[non_exhaustive]
#[cfg_attr(feature = "defmt-03", derive(defmt::Format))]
pub enum Error {
    /// The input contains a non-hex byte or a letter in a disallowed case.
    ///
    /// Identifies the first invalid byte in source order, including for SIMD
    /// conversions. Non-ASCII and non-UTF-8 input is represented without loss.
    #[non_exhaustive]
    InvalidChar {
        /// The zero-based position of the first invalid byte in the supplied input.
        ///
        /// This counts bytes, not Unicode characters or decoded output bytes.
        index: usize,
        /// The invalid byte, including a letter rejected by the case policy.
        byte: u8,
    },
    /// The destination cannot hold the complete output.
    ///
    /// Used by slice conversion and fixed-capacity string encoding. Extra
    /// capacity is allowed; this error only indicates insufficient capacity.
    #[non_exhaustive]
    OutputTooSmall {
        /// The full required destination capacity in bytes, not the shortfall.
        ///
        /// This is twice the input length for encoding and half the even input
        /// length for decoding.
        required: usize,
    },
    /// An array decoder's input has the wrong decoded length.
    ///
    /// Both shorter and longer inputs fail. This is distinct from
    /// [`OutputTooSmall`](Self::OutputTooSmall), which allows spare capacity.
    #[non_exhaustive]
    LengthMismatch {
        /// The required array length, in decoded bytes.
        expected: usize,
        /// The input length after dividing the even hex length by two.
        actual: usize,
    },
    /// The hex input contains an odd number of bytes and cannot form byte pairs.
    ///
    /// Decoders check this before capacity, exact length or character validity.
    /// Character-only checkers may still accept an odd-length input.
    OddLength,
    /// The encoded length cannot be represented as a `usize`.
    ///
    /// This is a length-arithmetic error, not an allocation-failure result.
    Overflow,
}

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match *self {
            Self::InvalidChar { index, byte } => {
                write!(f, "invalid hex byte 0x{byte:02x} at index {index}")
            }
            Self::OutputTooSmall { required } => {
                write!(f, "output requires at least {required} bytes")
            }
            Self::LengthMismatch { expected, actual } => {
                write!(f, "expected {expected} decoded bytes, got {actual}")
            }
            Self::OddLength => f.write_str("hex input length must be even"),
            Self::Overflow => f.write_str("encoded length overflow"),
        }
    }
}

impl core::error::Error for Error {}
