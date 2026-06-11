// SPDX-License-Identifier: MIT

//! Segregated Witness API - enables typical usage for encoding and decoding segwit addresses.
//!
//! [BIP-173] and [BIP-350] contain some complexity. This module aims to allow you to create modern
//! Bitcoin addresses correctly and easily without intimate knowledge of the BIPs. However, if you
//! do posses such knowledge and are doing unusual things you may prefer to use the `primitives`
//! submodules directly.
//!
//! # Examples
//!
//! ```
//! # #[cfg(feature = "alloc")] {
//! use bech32::{hrp, segwit, Fe32, Hrp};
//!
//! let witness_prog = [
//!     0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4,
//!     0x54, 0x94, 0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23,
//!     0xf1, 0x43, 0x3b, 0xd6,
//! ];
//!
//! // Encode a taproot address suitable for use on mainnet.
//! let _ = segwit::encode_v1(hrp::BC, &witness_prog);
//!
//! // Encode a segwit v0 address suitable for use on testnet.
//! let _ = segwit::encode_v0(hrp::TB, &witness_prog);
//!
//! // If you have the witness version already you can use:
//! # let witness_version = segwit::VERSION_0;
//! let _ = segwit::encode(hrp::BC, witness_version, &witness_prog);
//!
//! // Decode a Bitcoin bech32 segwit address.
//! let address = "bc1q2s3rjwvam9dt2ftt4sqxqjf3twav0gdx0k0q2etxflx38c3x8tnssdmnjq";
//! let (hrp, witness_version, witness_program) = segwit::decode(address).expect("failed to decode address");
//! # }
//! ```
//!
//! [BIP-173]: <https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki>
//! [BIP-350]: <https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki>
//! [`bip_173_test_vectors.rs`]: <https://github.com/rust-bitcoin/rust-bech32/blob/master/tests/bip_173_test_vectors.rs>
//! [`bip_350_test_vectors.rs`]: <https://github.com/rust-bitcoin/rust-bech32/blob/master/tests/bip_350_test_vectors.rs>

#[cfg(all(feature = "alloc", not(feature = "std"), not(test)))]
use alloc::{string::String, vec::Vec};
use core::fmt;

use crate::error::write_err;
use crate::primitives::decode::SegwitCodeLengthError;
#[cfg(feature = "alloc")]
use crate::primitives::decode::{SegwitHrpstring, SegwitHrpstringError};
use crate::primitives::gf32::Fe32;
use crate::primitives::hrp::Hrp;
use crate::primitives::iter::{ByteIterExt, Fe32IterExt};
#[cfg(feature = "alloc")]
use crate::primitives::segwit;
use crate::primitives::segwit::{
    InvalidWitnessVersionError, WitnessLengthError, MAX_STRING_LENGTH,
};
use crate::primitives::{Bech32, Bech32m};

#[rustfmt::skip]                // Keep public re-exports separate.
#[doc(inline)]
pub use {
    crate::primitives::segwit::{VERSION_0, VERSION_1},
};

/// Decodes a segwit address.
///
/// # Returns
///
/// The HRP, the witness version, and a guaranteed valid length witness program.
///
/// # Examples
///
/// ```
/// use bech32::segwit;
/// let address = "bc1py3m7vwnghyne9gnvcjw82j7gqt2rafgdmlmwmqnn3hvcmdm09rjqcgrtxs";
/// let (_hrp, _witness_version, _witness_program) = segwit::decode(address).expect("failed to decode address");
/// ```
#[cfg(feature = "alloc")]
#[inline]
pub fn decode(s: &str) -> Result<(Hrp, Fe32, Vec<u8>), DecodeError> {
    let segwit = SegwitHrpstring::new(s)?;
    Ok((segwit.hrp(), segwit.witness_version(), segwit.byte_iter().collect::<Vec<u8>>()))
}

/// Encodes a segwit address.
///
/// Does validity checks on the `witness_version`, length checks on the `witness_program`, and
/// checks the total encoded string length.
///
/// As specified by [BIP-350] we use the [`Bech32m`] checksum algorithm for witness versions 1 and
/// above, and for witness version 0 we use the original ([BIP-173]) [`Bech32`] checksum
/// algorithm.
///
/// See also [`encode_v0`] or [`encode_v1`].
///
/// [`Bech32`]: crate::primitives::Bech32
/// [`Bech32m`]: crate::primitives::Bech32m
/// [BIP-173]: <https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki>
/// [BIP-350]: <https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki>
#[cfg(feature = "alloc")]
#[inline]
pub fn encode(
    hrp: Hrp,
    witness_version: Fe32,
    witness_program: &[u8],
) -> Result<String, EncodeError> {
    segwit::validate_witness_version(witness_version)?;
    segwit::validate_witness_program_length(witness_program.len(), witness_version)?;

    let _ = encoded_length(hrp, witness_version, witness_program)?;

    let mut buf = String::new();
    encode_to_fmt_unchecked(&mut buf, hrp, witness_version, witness_program)?;
    Ok(buf)
}

/// Encodes a segwit version 0 address.
///
/// Does validity checks on the `witness_version`, length checks on the `witness_program`, and
/// checks the total encoded string length.
#[cfg(feature = "alloc")]
#[inline]
pub fn encode_v0(hrp: Hrp, witness_program: &[u8]) -> Result<String, EncodeError> {
    encode(hrp, VERSION_0, witness_program)
}

/// Encodes a segwit version 1 address.
///
/// Does validity checks on the `witness_version`, length checks on the `witness_program`, and
/// checks the total encoded string length.
#[cfg(feature = "alloc")]
#[inline]
pub fn encode_v1(hrp: Hrp, witness_program: &[u8]) -> Result<String, EncodeError> {
    encode(hrp, VERSION_1, witness_program)
}

/// Encodes a segwit address to a writer ([`fmt::Write`]) using lowercase characters.
///
/// There are no guarantees that the written string is a valid segwit address unless all the
/// parameters are valid. See the body of `encode()` to see the validity checks required.
#[inline]
pub fn encode_to_fmt_unchecked<W: fmt::Write>(
    fmt: &mut W,
    hrp: Hrp,
    witness_version: Fe32,
    witness_program: &[u8],
) -> fmt::Result {
    encode_lower_to_fmt_unchecked(fmt, hrp, witness_version, witness_program)
}

/// Encodes a segwit address to a writer ([`fmt::Write`]) using lowercase characters.
///
/// There are no guarantees that the written string is a valid segwit address unless all the
/// parameters are valid. See the body of `encode()` to see the validity checks required.
pub fn encode_lower_to_fmt_unchecked<W: fmt::Write>(
    fmt: &mut W,
    hrp: Hrp,
    witness_version: Fe32,
    witness_program: &[u8],
) -> fmt::Result {
    let mut buf = [0u8; MAX_STRING_LENGTH];
    let mut pos = 0;

    let iter = witness_program.iter().copied().bytes_to_fes();
    match witness_version {
        VERSION_0 => {
            let bytes = iter.with_checksum::<Bech32>(&hrp).with_witness_version(VERSION_0).bytes();
            buf.iter_mut().zip(bytes).for_each(|(dst, src)| {
                *dst = src;
                pos += 1;
            });
        }
        version => {
            let bytes = iter.with_checksum::<Bech32m>(&hrp).with_witness_version(version).bytes();
            buf.iter_mut().zip(bytes).for_each(|(dst, src)| {
                *dst = src;
                pos += 1;
            });
        }
    }

    let s = core::str::from_utf8(&buf[..pos]).expect("we only write ASCII");
    fmt.write_str(s)?;

    Ok(())
}

/// Encodes a segwit address to a writer ([`fmt::Write`]) using uppercase characters.
///
/// This is provided for use when creating QR codes.
///
/// There are no guarantees that the written string is a valid segwit address unless all the
/// parameters are valid. See the body of `encode()` to see the validity checks required.
#[inline]
pub fn encode_upper_to_fmt_unchecked<W: fmt::Write>(
    fmt: &mut W,
    hrp: Hrp,
    witness_version: Fe32,
    witness_program: &[u8],
) -> fmt::Result {
    let mut buf = [0u8; MAX_STRING_LENGTH];
    let mut pos = 0;

    let iter = witness_program.iter().copied().bytes_to_fes();
    match witness_version {
        VERSION_0 => {
            let bytes = iter.with_checksum::<Bech32>(&hrp).with_witness_version(VERSION_0).bytes();
            buf.iter_mut().zip(bytes).for_each(|(dst, src)| {
                *dst = src.to_ascii_uppercase();
                pos += 1;
            });
        }
        version => {
            let bytes = iter.with_checksum::<Bech32m>(&hrp).with_witness_version(version).bytes();
            buf.iter_mut().zip(bytes).for_each(|(dst, src)| {
                *dst = src.to_ascii_uppercase();
                pos += 1;
            });
        }
    }

    let s = core::str::from_utf8(&buf[..pos]).expect("we only write ASCII");
    fmt.write_str(s)?;

    Ok(())
}

/// Encodes a segwit address to a writer ([`io::Write`]) using lowercase characters.
///
/// There are no guarantees that the written string is a valid segwit address unless all the
/// parameters are valid. See the body of `encode()` to see the validity checks required.
///
/// [`io::Write`]: std::io::Write
#[cfg(feature = "std")]
#[inline]
pub fn encode_to_writer_unchecked<W: std::io::Write>(
    w: &mut W,
    hrp: Hrp,
    witness_version: Fe32,
    witness_program: &[u8],
) -> std::io::Result<()> {
    encode_lower_to_writer_unchecked(w, hrp, witness_version, witness_program)
}

/// Encodes a segwit address to a writer ([`io::Write`]) using lowercase characters.
///
/// There are no guarantees that the written string is a valid segwit address unless all the
/// parameters are valid. See the body of `encode()` to see the validity checks required.
///
/// [`io::Write`]: std::io::Write
#[cfg(feature = "std")]
#[inline]
pub fn encode_lower_to_writer_unchecked<W: std::io::Write>(
    w: &mut W,
    hrp: Hrp,
    witness_version: Fe32,
    witness_program: &[u8],
) -> std::io::Result<()> {
    let mut buf = [0u8; MAX_STRING_LENGTH];
    let mut pos = 0;

    let iter = witness_program.iter().copied().bytes_to_fes();
    match witness_version {
        VERSION_0 => {
            let bytes = iter.with_checksum::<Bech32>(&hrp).with_witness_version(VERSION_0).bytes();
            buf.iter_mut().zip(bytes).for_each(|(dst, src)| {
                *dst = src;
                pos += 1;
            });
        }
        version => {
            let bytes = iter.with_checksum::<Bech32m>(&hrp).with_witness_version(version).bytes();
            buf.iter_mut().zip(bytes).for_each(|(dst, src)| {
                *dst = src;
                pos += 1;
            });
        }
    }

    w.write_all(&buf[..pos])?;

    Ok(())
}

/// Encodes a segwit address to a [`io::Write`] writer using uppercase characters.
///
/// This is provided for use when creating QR codes.
///
/// There are no guarantees that the written string is a valid segwit address unless all the
/// parameters are valid. See the body of `encode()` to see the validity checks required.
///
/// [`io::Write`]: std::io::Write
#[cfg(feature = "std")]
#[inline]
pub fn encode_upper_to_writer_unchecked<W: std::io::Write>(
    w: &mut W,
    hrp: Hrp,
    witness_version: Fe32,
    witness_program: &[u8],
) -> std::io::Result<()> {
    let mut buf = [0u8; MAX_STRING_LENGTH];
    let mut pos = 0;

    let iter = witness_program.iter().copied().bytes_to_fes();
    match witness_version {
        VERSION_0 => {
            let bytes = iter.with_checksum::<Bech32>(&hrp).with_witness_version(VERSION_0).bytes();
            buf.iter_mut().zip(bytes).for_each(|(dst, src)| {
                *dst = src.to_ascii_uppercase();
                pos += 1;
            });
        }
        version => {
            let bytes = iter.with_checksum::<Bech32m>(&hrp).with_witness_version(version).bytes();
            buf.iter_mut().zip(bytes).for_each(|(dst, src)| {
                *dst = src.to_ascii_uppercase();
                pos += 1;
            });
        }
    }

    w.write_all(&buf[..pos])?;

    Ok(())
}

/// Returns the length of the address after encoding HRP, witness version and program.
///
/// # Returns
///
/// `Ok(address_length)` if the encoded address length is less than or equal to 90. Otherwise
/// returns a [`SegwitCodeLengthError`] containing the encoded address length.
pub fn encoded_length(
    hrp: Hrp,
    _witness_version: Fe32, // Emphasize that this is only for segwit.
    witness_program: &[u8],
) -> Result<usize, SegwitCodeLengthError> {
    // Ck is only for length and since they are both the same we can use either here.
    let len = crate::encoded_length::<Bech32>(hrp, witness_program).map(|len| len + 1)?; // +1 for witness version.

    if len > MAX_STRING_LENGTH {
        Err(SegwitCodeLengthError(len))
    } else {
        Ok(len)
    }
}

/// An error while decoding a segwit address.
#[cfg(feature = "alloc")]
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub struct DecodeError(pub SegwitHrpstringError);

#[cfg(feature = "alloc")]
impl fmt::Display for DecodeError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write_err!(f, "decoding segwit address failed"; self.0)
    }
}

#[cfg(feature = "std")]
impl std::error::Error for DecodeError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> { Some(&self.0) }
}

#[cfg(feature = "alloc")]
impl From<SegwitHrpstringError> for DecodeError {
    #[inline]
    fn from(e: SegwitHrpstringError) -> Self { Self(e) }
}

/// An error while constructing a [`SegwitHrpstring`] type.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
#[cfg(feature = "alloc")]
pub enum EncodeError {
    /// Invalid witness version (must be 0-16 inclusive).
    WitnessVersion(InvalidWitnessVersionError),
    /// Invalid witness length.
    WitnessLength(WitnessLengthError),
    /// Encoding HRP, witver, and program into a bech32 string exceeds maximum allowed.
    TooLong(SegwitCodeLengthError),
    /// Writing to formatter failed.
    Fmt(fmt::Error),
}

#[cfg(feature = "alloc")]
impl fmt::Display for EncodeError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use EncodeError::*;

        match *self {
            WitnessVersion(ref e) => write_err!(f, "witness version"; e),
            WitnessLength(ref e) => write_err!(f, "witness length"; e),
            TooLong(ref e) => write_err!(f, "encode error"; e),
            Fmt(ref e) => write_err!(f, "writing to formatter failed"; e),
        }
    }
}

#[cfg(feature = "std")]
#[cfg(feature = "alloc")]
impl std::error::Error for EncodeError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        use EncodeError::*;

        match *self {
            WitnessVersion(ref e) => Some(e),
            WitnessLength(ref e) => Some(e),
            TooLong(ref e) => Some(e),
            Fmt(ref e) => Some(e),
        }
    }
}

#[cfg(feature = "alloc")]
impl From<InvalidWitnessVersionError> for EncodeError {
    #[inline]
    fn from(e: InvalidWitnessVersionError) -> Self { Self::WitnessVersion(e) }
}

#[cfg(feature = "alloc")]
impl From<WitnessLengthError> for EncodeError {
    #[inline]
    fn from(e: WitnessLengthError) -> Self { Self::WitnessLength(e) }
}

#[cfg(feature = "alloc")]
impl From<SegwitCodeLengthError> for EncodeError {
    #[inline]
    fn from(e: SegwitCodeLengthError) -> Self { Self::TooLong(e) }
}

#[cfg(feature = "alloc")]
impl From<fmt::Error> for EncodeError {
    #[inline]
    fn from(e: fmt::Error) -> Self { Self::Fmt(e) }
}

#[cfg(all(test, feature = "alloc"))]
mod tests {
    use super::*;
    use crate::primitives::decode::{SegwitCodeLengthError, SegwitHrpstringError};
    use crate::primitives::hrp;

    #[test]
    // Just shows we handle both v0 and v1 addresses, for complete test
    // coverage see primitives submodules and test vectors.
    fn roundtrip_valid_mainnet_addresses() {
        // A few recent addresses from mainnet (Block 801266).
        let addresses = vec![
            "bc1q2s3rjwvam9dt2ftt4sqxqjf3twav0gdx0k0q2etxflx38c3x8tnssdmnjq", // Segwit v0
            "bc1py3m7vwnghyne9gnvcjw82j7gqt2rafgdmlmwmqnn3hvcmdm09rjqcgrtxs", // Segwit v1
        ];

        for address in addresses {
            let (hrp, version, program) = decode(address).expect("failed to decode valid address");
            let encoded = encode(hrp, version, &program).expect("failed to encode address");
            assert_eq!(encoded, address);
        }
    }

    fn witness_program() -> [u8; 20] {
        [
            0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4, 0x54, 0x94, 0x1c, 0x45, 0xd1, 0xb3,
            0xa3, 0x23, 0xf1, 0x43, 0x3b, 0xd6,
        ]
    }

    #[test]
    fn encode_lower_to_fmt() {
        let program = witness_program();
        let mut address = String::new();
        encode_to_fmt_unchecked(&mut address, hrp::BC, VERSION_0, &program)
            .expect("failed to encode address to QR code");

        let want = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
        assert_eq!(address, want);
    }

    #[test]
    fn encode_upper_to_fmt() {
        let program = witness_program();
        let mut address = String::new();
        encode_upper_to_fmt_unchecked(&mut address, hrp::BC, VERSION_0, &program)
            .expect("failed to encode address to QR code");

        let want = "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4";
        assert_eq!(address, want);
    }

    #[test]
    #[cfg(feature = "std")]
    fn encode_lower_to_writer() {
        let program = witness_program();
        let mut buf = Vec::new();
        encode_lower_to_writer_unchecked(&mut buf, hrp::BC, VERSION_0, &program)
            .expect("failed to encode");

        let address = std::str::from_utf8(&buf).expect("ascii is valid utf8");
        let want = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
        assert_eq!(address, want);
    }

    #[test]
    #[cfg(feature = "std")]
    fn encode_upper_to_writer() {
        let program = witness_program();
        let mut buf = Vec::new();
        encode_upper_to_writer_unchecked(&mut buf, hrp::BC, VERSION_0, &program)
            .expect("failed to encode");

        let address = std::str::from_utf8(&buf).expect("ascii is valid utf8");
        let want = "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4";
        assert_eq!(address, want);
    }

    #[test]
    #[cfg(feature = "std")]
    fn encode_lower_to_writer_including_lowecaseing_hrp() {
        let program = witness_program();
        let mut buf = Vec::new();
        let hrp = Hrp::parse_unchecked("BC");
        encode_lower_to_writer_unchecked(&mut buf, hrp, VERSION_0, &program)
            .expect("failed to encode");

        let address = std::str::from_utf8(&buf).expect("ascii is valid utf8");
        let want = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
        assert_eq!(address, want);
    }

    #[test]
    fn encoded_length_works() {
        let addresses = vec![
            "bc1q2s3rjwvam9dt2ftt4sqxqjf3twav0gdx0k0q2etxflx38c3x8tnssdmnjq",
            "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4",
        ];

        for address in addresses {
            let (hrp, version, program) = decode(address).expect("valid address");

            let encoded = encode(hrp, version, &program).expect("valid data");
            let want = encoded.len();
            let got = encoded_length(hrp, version, &program).expect("encoded length");

            assert_eq!(got, want);
        }
    }

    #[test]
    fn can_encode_maximum_length_address() {
        let program = [0_u8; 40]; // Maximum witness program length.
        let hrp = Hrp::parse_unchecked("anhrpthatis18chars");
        let addr = encode(hrp, VERSION_1, &program).expect("valid data");
        assert_eq!(addr.len(), MAX_STRING_LENGTH);
    }

    #[test]
    fn can_not_encode_address_too_long() {
        let tcs = vec![
            ("anhrpthatis19charsx", 91),
            ("anhrpthatisthemaximumallowedlengthofeightythreebytesxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 155)
        ];

        for (hrp, len) in tcs {
            let program = [0_u8; 40]; // Maximum witness program length.
            let hrp = Hrp::parse_unchecked(hrp);
            let err = encode(hrp, VERSION_1, &program).unwrap_err();
            assert_eq!(err, EncodeError::TooLong(SegwitCodeLengthError(len)));
        }
    }

    #[test]
    fn can_decode_maximum_length_address() {
        let address = "anhrpthatisnineteen1pqyqszqgpqyqszqgpqyqszqgpqyqszqgpqyqszqgpqyqszqgpqyqszqgpqyqszqghfyyfz";
        assert_eq!(address.len(), MAX_STRING_LENGTH);

        assert!(decode(address).is_ok());
    }

    #[test]
    fn can_not_decode_address_too_long() {
        let address = "anhrpthatistwentycha1pqyqszqgpqyqszqgpqyqszqgpqyqszqgpqyqszqgpqyqszqgpqyqszqgpqyqszqgqfrwjz";
        assert_eq!(address.len(), MAX_STRING_LENGTH + 1);

        assert_eq!(decode(address).unwrap_err(), DecodeError(SegwitHrpstringError::TooLong(91)));
    }
}
