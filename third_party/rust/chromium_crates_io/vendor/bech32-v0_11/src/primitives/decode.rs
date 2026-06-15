// SPDX-License-Identifier: MIT

//! Decoding of bech32 encoded strings as specified by [BIP-173] and [BIP-350].
//!
//! You should only need to use this module directly if you want control over exactly what is
//! checked and when it is checked (correct bech32 characters, valid checksum, valid checksum for
//! specific checksum algorithm, etc). If you are parsing/validating modern (post BIP-350) bitcoin
//! segwit addresses consider using the [`crate::segwit`] API.
//!
//! If you do find yourself using this module directly then consider using the most general type
//! that serves your purposes, each type can be created by parsing an address string to `new`. You
//! likely do not want to arbitrarily transition from one type to the next even though possible. And
//! be prepared to spend some time with the bips - you have been warned :)
//!
//! # Details
//!
//! A Bech32 string is at most 90 characters long and consists of:
//!
//! - The human-readable part, which is intended to convey the type of data, or anything else that
//!   is relevant to the reader. This part MUST contain 1 to 83 US-ASCII characters.
//! - The separator, which is always "1".
//! - The data part, which is at least 6 characters long and only consists of alphanumeric
//!   characters excluding "1", "b", "i", and "o".
//!
//! The types in this module heavily lean on the wording in BIP-173: *We first
//! describe the general checksummed base32 format called Bech32 and then define Segregated Witness
//! addresses using it.*
//!
//! - `UncheckedHrpstring`: Parses the general checksummed base32 format and provides checksum validation.
//! - `CheckedHrpstring`: Provides access to the data encoded by a general checksummed base32 string and segwit checks.
//! - `SegwitHrpstring`: Provides access to the data encoded by a segwit address.
//!
//! # Examples
//!
//! ```
//! use bech32::{Bech32, Bech32m, Fe32, Hrp};
//! use bech32::primitives::decode::{CheckedHrpstring, SegwitHrpstring, UncheckedHrpstring};
//! use bech32::segwit::VERSION_1;
//!
//! // An arbitrary HRP and a string of valid bech32 characters.
//! let s = "abcd143hj65vxw49rts6kcw35u6r6tgzguyr03vvveeewjqpn05efzq444444";
//! assert!(UncheckedHrpstring::new(s).is_ok());
//! // But it has an invalid checksum.
//! assert!(CheckedHrpstring::new::<Bech32>(s).is_err());
//! assert!(CheckedHrpstring::new::<Bech32m>(s).is_err());
//! assert!(SegwitHrpstring::new(s).is_err());
//!
//! // An arbitrary HRP, a string of valid bech32 characters, and a valid bech32 checksum.
//! let s = "abcd14g08d6qejxtdg4y5r3zarvary0c5xw7kxugcx9";
//! assert!(UncheckedHrpstring::new(s).is_ok());
//! assert!(CheckedHrpstring::new::<Bech32>(s).is_ok());
//! // But not a valid segwit address.
//! assert!(SegwitHrpstring::new(s).is_err());
//! // And not a valid bech32m checksum.
//! assert!(CheckedHrpstring::new::<Bech32m>(s).is_err());
//!
//! // A valid Bitcoin taproot address.
//! let s = "bc1pdp43hj65vxw49rts6kcw35u6r6tgzguyr03vvveeewjqpn05efzq7un9w0";
//! assert!(UncheckedHrpstring::new(s).is_ok());
//! assert!(CheckedHrpstring::new::<Bech32m>(s).is_ok());
//! assert!(SegwitHrpstring::new(s).is_ok());
//! // But not a valid segwit v0 checksum.
//! assert!(CheckedHrpstring::new::<Bech32>(s).is_err());
//!
//! // Get the HRP, witness version, and encoded data.
//! let address = "bc1pdp43hj65vxw49rts6kcw35u6r6tgzguyr03vvveeewjqpn05efzq7un9w0";
//! let segwit = SegwitHrpstring::new(address).expect("valid segwit address");
//! let _encoded_data = segwit.byte_iter();
//! assert_eq!(segwit.hrp(), Hrp::parse("bc").unwrap());
//! assert_eq!(segwit.witness_version(), VERSION_1);
//! ```
//!
//! [BIP-173]: <https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki>
//! [BIP-350]: <https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki>

use core::{fmt, iter, slice, str};

use crate::error::write_err;
use crate::primitives::checksum::{self, Checksum};
use crate::primitives::gf32::Fe32;
use crate::primitives::hrp::{self, Hrp};
use crate::primitives::iter::{Fe32IterExt, FesToBytes};
use crate::primitives::segwit::{self, WitnessLengthError, VERSION_0};
use crate::{Bech32, Bech32m};

/// Separator between the hrp and payload (as defined by BIP-173).
const SEP: char = '1';

/// An HRP string that has been parsed but not yet had the checksum checked.
///
/// Parsing an HRP string only checks validity of the characters, it does not validate the
/// checksum in any way.
///
/// Unless you are attempting to validate a string with multiple checksums then you likely do not
/// want to use this type directly, instead use [`CheckedHrpstring::new`]`(s)`.
///
/// # Examples
///
/// ```
/// use bech32::{Bech32, Bech32m, primitives::decode::UncheckedHrpstring};
///
/// let addr = "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4";
/// let unchecked = UncheckedHrpstring::new(addr).expect("valid bech32 character encoded string");
/// if unchecked.has_valid_checksum::<Bech32>() {
///     // Remove the checksum and do something with the data.
///     let checked = unchecked.remove_checksum::<Bech32>();
///     let _ = checked.byte_iter();
/// } else if unchecked.has_valid_checksum::<Bech32m>() {
///     // Remove the checksum and do something with the data as above.
/// } else {
///     // Checksum is not valid for either the bech32 or bech32 checksum algorithms.
/// }
/// ```
#[derive(Debug)]
pub struct UncheckedHrpstring<'s> {
    /// The human-readable part, guaranteed to be lowercase ASCII characters.
    hrp: Hrp,
    /// This is ASCII byte values of the parsed string, guaranteed to be valid bech32 characters.
    ///
    /// The characters after the separator i.e., the "data part" defined by BIP-173.
    data_part_ascii: &'s [u8],
    /// The length of the parsed hrpstring.
    hrpstring_length: usize,
}

impl<'s> UncheckedHrpstring<'s> {
    /// Parses an bech32 encode string and constructs a [`UncheckedHrpstring`] object.
    ///
    /// Checks for valid ASCII values, does not validate the checksum.
    #[inline]
    pub fn new(s: &'s str) -> Result<Self, UncheckedHrpstringError> {
        let sep_pos = check_characters(s)?;
        let (hrp, rest) = s.split_at(sep_pos);

        let ret = UncheckedHrpstring {
            hrp: Hrp::parse(hrp)?,
            data_part_ascii: &rest.as_bytes()[1..], // Skip the separator.
            hrpstring_length: s.len(),
        };

        Ok(ret)
    }

    /// Returns the human-readable part.
    #[inline]
    pub fn hrp(&self) -> Hrp { self.hrp }

    /// Returns the data part as ASCII bytes i.e., everything after the separator '1'.
    ///
    /// The byte values are guaranteed to be valid bech32 characters. Includes the checksum
    /// if one was present in the parsed string.
    ///
    /// # Examples
    ///
    /// ```
    /// use bech32::primitives::decode::UncheckedHrpstring;
    ///
    /// let addr = "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
    /// let ascii = "qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
    ///
    /// let unchecked = UncheckedHrpstring::new(&addr).unwrap();
    /// assert!(unchecked.data_part_ascii().iter().eq(ascii.as_bytes().iter()))
    /// ```
    #[inline]
    pub fn data_part_ascii(&self) -> &'s [u8] { self.data_part_ascii }

    /// Attempts to remove the first byte of the data part, treating it as a witness version.
    ///
    /// If [`Self::witness_version`] succeeds this function removes the first character (witness
    /// version byte) from the internal ASCII data part buffer. Future calls to
    /// [`Self::data_part_ascii`] will no longer include it.
    ///
    /// # Examples
    ///
    /// ```
    /// use bech32::{primitives::decode::UncheckedHrpstring, Fe32};
    ///
    /// let addr = "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
    /// let ascii = "ar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
    ///
    /// let mut unchecked = UncheckedHrpstring::new(&addr).unwrap();
    /// let witness_version = unchecked.remove_witness_version().unwrap();
    /// assert_eq!(witness_version, Fe32::Q);
    /// assert!(unchecked.data_part_ascii().iter().eq(ascii.as_bytes().iter()))
    /// ```
    #[inline]
    pub fn remove_witness_version(&mut self) -> Option<Fe32> {
        self.witness_version().map(|witver| {
            self.data_part_ascii = &self.data_part_ascii[1..]; // Remove the witness version byte.
            witver
        })
    }

    /// Returns the segwit witness version if there is one.
    ///
    /// Attempts to convert the first character of the data part to a witness version. If this
    /// succeeds, and it is a valid version (0..16 inclusive) we return it, otherwise `None`.
    ///
    /// Future calls to [`Self::data_part_ascii`] will still include the witness version, use
    /// [`Self::remove_witness_version`] to remove it.
    ///
    /// This function makes no guarantees on the validity of the checksum.
    ///
    /// # Examples
    ///
    /// ```
    /// use bech32::{primitives::decode::UncheckedHrpstring, Fe32};
    ///
    /// // Note the invalid checksum!
    /// let addr = "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzffffff";
    ///
    /// let unchecked = UncheckedHrpstring::new(&addr).unwrap();
    /// assert_eq!(unchecked.witness_version(), Some(Fe32::Q));
    /// ```
    #[inline]
    pub fn witness_version(&self) -> Option<Fe32> {
        let data_part = self.data_part_ascii();
        if data_part.is_empty() {
            return None;
        }

        // unwrap ok because we know we gave valid bech32 characters.
        let witness_version = Fe32::from_char(data_part[0].into()).unwrap();
        if witness_version.to_u8() > 16 {
            return None;
        }
        Some(witness_version)
    }

    /// Validates that data has a valid checksum for the `Ck` algorithm and returns a [`CheckedHrpstring`].
    #[inline]
    pub fn validate_and_remove_checksum<Ck: Checksum>(
        self,
    ) -> Result<CheckedHrpstring<'s>, ChecksumError> {
        self.validate_checksum::<Ck>()?;
        Ok(self.remove_checksum::<Ck>())
    }

    /// Validates that data has a valid checksum for the `Ck` algorithm (this may mean an empty
    /// checksum if `NoChecksum` is used).
    ///
    /// This is useful if you do not know which checksum algorithm was used and wish to validate
    /// against multiple algorithms consecutively. If this function returns `true` then call
    /// `remove_checksum` to get a [`CheckedHrpstring`].
    #[inline]
    pub fn has_valid_checksum<Ck: Checksum>(&self) -> bool {
        self.validate_checksum::<Ck>().is_ok()
    }

    /// Validates that data has a valid checksum for the `Ck` algorithm (this may mean an empty
    /// checksum if `NoChecksum` is used).
    #[inline]
    pub fn validate_checksum<Ck: Checksum>(&self) -> Result<(), ChecksumError> {
        use ChecksumError::*;

        if self.hrpstring_length > Ck::CODE_LENGTH {
            return Err(ChecksumError::CodeLength(CodeLengthError {
                encoded_length: self.hrpstring_length,
                code_length: Ck::CODE_LENGTH,
            }));
        }

        if Ck::CHECKSUM_LENGTH == 0 {
            // Called with NoChecksum
            return Ok(());
        }

        if self.data_part_ascii.len() < Ck::CHECKSUM_LENGTH {
            return Err(InvalidLength);
        }

        let mut checksum_eng = checksum::Engine::<Ck>::new();
        checksum_eng.input_hrp(self.hrp());

        // Unwrap ok since we checked all characters in our constructor.
        for fe in self.data_part_ascii.iter().map(|&b| Fe32::from_char_unchecked(b)) {
            checksum_eng.input_fe(fe);
        }

        if checksum_eng.residue() != &Ck::TARGET_RESIDUE {
            return Err(InvalidResidue);
        }

        Ok(())
    }

    /// Removes the checksum for the `Ck` algorithm and returns an [`CheckedHrpstring`].
    ///
    /// Data must be valid (ie, first call `has_valid_checksum` or `validate_checksum()`). This
    /// function is typically paired with `has_valid_checksum` when validating against multiple
    /// checksum algorithms consecutively.
    ///
    /// # Panics
    ///
    /// May panic if data is not valid.
    #[inline]
    pub fn remove_checksum<Ck: Checksum>(self) -> CheckedHrpstring<'s> {
        let end = self.data_part_ascii.len() - Ck::CHECKSUM_LENGTH;

        CheckedHrpstring {
            hrp: self.hrp(),
            ascii: &self.data_part_ascii[..end],
            hrpstring_length: self.hrpstring_length,
        }
    }
}

/// An HRP string that has been parsed and had the checksum validated.
///
/// This type does not treat the first byte of the data part in any special way i.e., as the witness
/// version byte. If you are parsing Bitcoin segwit addresses consider using [`SegwitHrpstring`].
///
/// > We first describe the general checksummed base32 format called Bech32 and then
/// > define Segregated Witness addresses using it.
///
/// This type abstracts over the general checksummed base32 format called Bech32.
///
/// # Examples
///
/// ```
/// use bech32::{Bech32m, primitives::decode::CheckedHrpstring};
///
/// // Parse a general checksummed bech32 encoded string.
/// let s = "abcd14g08d6qejxtdg4y5r3zarvary0c5xw7knqc5r8";
/// let checked = CheckedHrpstring::new::<Bech32m>(s)
///       .expect("valid bech32 string with a valid checksum according to the bech32m algorithm");
///
/// // Do something with the encoded data.
/// let _ = checked.byte_iter();
/// ```
#[derive(Debug)]
pub struct CheckedHrpstring<'s> {
    /// The human-readable part, guaranteed to be lowercase ASCII characters.
    hrp: Hrp,
    /// This is ASCII byte values of the parsed string, guaranteed to be valid bech32 characters.
    ///
    /// The characters after the '1' separator and the before the checksum.
    ascii: &'s [u8],
    /// The length of the parsed hrpstring.
    hrpstring_length: usize, // Guaranteed to be <= CK::CODE_LENGTH
}

impl<'s> CheckedHrpstring<'s> {
    /// Parses and validates an HRP string, without treating the first data character specially.
    ///
    /// If you are validating the checksum multiple times consider using [`UncheckedHrpstring`].
    ///
    /// This is equivalent to `UncheckedHrpstring::new().validate_and_remove_checksum::<CK>()`.
    #[inline]
    pub fn new<Ck: Checksum>(s: &'s str) -> Result<Self, CheckedHrpstringError> {
        let unchecked = UncheckedHrpstring::new(s)?;
        let checked = unchecked.validate_and_remove_checksum::<Ck>()?;
        Ok(checked)
    }

    /// Returns the human-readable part.
    #[inline]
    pub fn hrp(&self) -> Hrp { self.hrp }

    /// Returns a partial slice of the data part, as ASCII bytes, everything after the separator '1'
    /// before the checksum.
    ///
    /// The byte values are guaranteed to be valid bech32 characters.
    ///
    /// # Examples
    ///
    /// ```
    /// use bech32::{Bech32, primitives::decode::CheckedHrpstring};
    ///
    /// let addr = "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
    /// let ascii = "qar0srrr7xfkvy5l643lydnw9re59gtzz";
    ///
    /// let checked = CheckedHrpstring::new::<Bech32>(&addr).unwrap();
    /// assert!(checked.data_part_ascii_no_checksum().iter().eq(ascii.as_bytes().iter()))
    /// ```
    #[inline]
    pub fn data_part_ascii_no_checksum(&self) -> &'s [u8] { self.ascii }

    /// Attempts to remove the first byte of the data part, treating it as a witness version.
    ///
    /// If [`Self::witness_version`] succeeds this function removes the first character (witness
    /// version byte) from the internal ASCII data part buffer. Future calls to
    /// [`Self::data_part_ascii_no_checksum`] will no longer include it.
    ///
    /// # Examples
    ///
    /// ```
    /// use bech32::{primitives::decode::CheckedHrpstring, Bech32, Fe32};
    ///
    /// let addr = "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
    /// let ascii = "ar0srrr7xfkvy5l643lydnw9re59gtzz";
    ///
    /// let mut checked = CheckedHrpstring::new::<Bech32>(&addr).unwrap();
    /// let witness_version = checked.remove_witness_version().unwrap();
    /// assert_eq!(witness_version, Fe32::Q);
    /// assert!(checked.data_part_ascii_no_checksum().iter().eq(ascii.as_bytes().iter()))
    /// ```
    #[inline]
    pub fn remove_witness_version(&mut self) -> Option<Fe32> {
        self.witness_version().map(|witver| {
            self.ascii = &self.ascii[1..]; // Remove the witness version byte.
            witver
        })
    }

    /// Returns the segwit witness version if there is one.
    ///
    /// Attempts to convert the first character of the data part to a witness version. If this
    /// succeeds, and it is a valid version (0..16 inclusive) we return it, otherwise `None`.
    ///
    /// Future calls to [`Self::data_part_ascii_no_checksum`] will still include the witness
    /// version, use [`Self::remove_witness_version`] to remove it.
    ///
    /// This function makes no guarantees on the validity of the checksum.
    ///
    /// # Examples
    ///
    /// ```
    /// use bech32::{primitives::decode::CheckedHrpstring, Bech32, Fe32};
    ///
    /// let addr = "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
    ///
    /// let checked = CheckedHrpstring::new::<Bech32>(&addr).unwrap();
    /// assert_eq!(checked.witness_version(), Some(Fe32::Q));
    /// ```
    #[inline]
    pub fn witness_version(&self) -> Option<Fe32> {
        let data_part = self.data_part_ascii_no_checksum();
        if data_part.is_empty() {
            return None;
        }

        // unwrap ok because we know we gave valid bech32 characters.
        let witness_version = Fe32::from_char(data_part[0].into()).unwrap();
        if witness_version.to_u8() > 16 {
            return None;
        }
        Some(witness_version)
    }

    /// Returns an iterator that yields the data part of the parsed bech32 encoded string as [`Fe32`]s.
    ///
    /// Converts the ASCII bytes representing field elements to the respective field elements.
    #[inline]
    pub fn fe32_iter<I: Iterator<Item = u8>>(&self) -> AsciiToFe32Iter<'_> {
        AsciiToFe32Iter { iter: self.ascii.iter().copied() }
    }

    /// Returns an iterator that yields the data part of the parsed bech32 encoded string.
    ///
    /// Converts the ASCII bytes representing field elements to the respective field elements, then
    /// converts the stream of field elements to a stream of bytes.
    #[inline]
    pub fn byte_iter(&self) -> ByteIter<'_> {
        ByteIter { iter: AsciiToFe32Iter { iter: self.ascii.iter().copied() }.fes_to_bytes() }
    }

    /// Converts this type to a [`SegwitHrpstring`] after validating the witness and HRP.
    #[inline]
    pub fn validate_segwit(mut self) -> Result<SegwitHrpstring<'s>, SegwitHrpstringError> {
        if self.ascii.is_empty() {
            return Err(SegwitHrpstringError::NoData);
        }

        if self.hrpstring_length > segwit::MAX_STRING_LENGTH {
            return Err(SegwitHrpstringError::TooLong(self.hrpstring_length));
        }

        // Unwrap ok since check_characters checked the bech32-ness of this char.
        let witness_version = Fe32::from_char(self.ascii[0].into()).unwrap();
        self.ascii = &self.ascii[1..]; // Remove the witness version byte.

        self.validate_segwit_padding()?;
        self.validate_witness_program_length(witness_version)?;

        Ok(SegwitHrpstring { hrp: self.hrp(), witness_version, ascii: self.ascii })
    }

    /// Validates the segwit padding rules.
    ///
    /// Must be called after the witness version byte is removed from the data part.
    ///
    /// From BIP-173:
    /// > Re-arrange those bits into groups of 8 bits. Any incomplete group at the
    /// > end MUST be 4 bits or less, MUST be all zeroes, and is discarded.
    #[inline]
    pub fn validate_segwit_padding(&self) -> Result<(), PaddingError> {
        if self.ascii.is_empty() {
            return Ok(()); // Empty data implies correct padding.
        }

        let fe_iter = AsciiToFe32Iter { iter: self.ascii.iter().copied() };
        let padding_len = fe_iter.len() * 5 % 8;

        if padding_len > 4 {
            return Err(PaddingError::TooMuch)?;
        }

        let last_fe = fe_iter.last().expect("checked above");
        let last_byte = last_fe.0;

        let padding_contains_non_zero_bits = match padding_len {
            0 => false,
            1 => last_byte & 0b0001 > 0,
            2 => last_byte & 0b0011 > 0,
            3 => last_byte & 0b0111 > 0,
            4 => last_byte & 0b1111 > 0,
            _ => unreachable!("checked above"),
        };
        if padding_contains_non_zero_bits {
            Err(PaddingError::NonZero)
        } else {
            Ok(())
        }
    }

    /// Validates the segwit witness length rules.
    ///
    /// Must be called after the witness version byte is removed from the data part.
    #[inline]
    pub fn validate_witness_program_length(
        &self,
        witness_version: Fe32,
    ) -> Result<(), WitnessLengthError> {
        segwit::validate_witness_program_length(self.byte_iter().len(), witness_version)
    }
}

/// An valid length HRP string that has been parsed, had the checksum validated, had the witness
/// version validated, had the witness data length checked, and the had witness version and checksum
/// removed.
///
/// # Examples
///
/// ```
/// use bech32::primitives::decode::SegwitHrpstring;
///
/// // Parse a segwit V0 address.
/// let address = "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
/// let segwit = SegwitHrpstring::new(address).expect("valid segwit address");
///
/// // Do something with the encoded data.
/// let _ = segwit.byte_iter();
/// ```
#[derive(Debug)]
pub struct SegwitHrpstring<'s> {
    /// The human-readable part, valid for segwit addresses.
    hrp: Hrp,
    /// The first byte of the parsed data part.
    witness_version: Fe32,
    /// This is ASCII byte values of the parsed string, guaranteed to be valid bech32 characters.
    ///
    /// The characters after the witness version and before the checksum.
    ascii: &'s [u8],
}

impl<'s> SegwitHrpstring<'s> {
    /// Parses an HRP string, treating the first data character as a witness version.
    ///
    /// The version byte does not appear in the extracted binary data, but is covered by the
    /// checksum. It can be accessed with [`Self::witness_version`].
    ///
    /// NOTE: We do not enforce any restrictions on the HRP, use [`SegwitHrpstring::has_valid_hrp`]
    /// to get strict BIP conformance (also [`Hrp::is_valid_on_mainnet`] and friends).
    #[inline]
    pub fn new(s: &'s str) -> Result<Self, SegwitHrpstringError> {
        let len = s.len();
        if len > segwit::MAX_STRING_LENGTH {
            return Err(SegwitHrpstringError::TooLong(len));
        }

        let unchecked = UncheckedHrpstring::new(s)?;

        let data_part = unchecked.data_part_ascii();

        if data_part.is_empty() {
            return Err(SegwitHrpstringError::NoData);
        }

        // Unwrap ok since check_characters (in `Self::new`) checked the bech32-ness of this char.
        let witness_version = Fe32::from_char(data_part[0].into()).unwrap();
        if witness_version.to_u8() > 16 {
            return Err(SegwitHrpstringError::InvalidWitnessVersion(witness_version));
        }

        let checked: CheckedHrpstring<'s> = match witness_version {
            VERSION_0 => unchecked.validate_and_remove_checksum::<Bech32>()?,
            _ => unchecked.validate_and_remove_checksum::<Bech32m>()?,
        };

        checked.validate_segwit()
    }

    /// Parses an HRP string, treating the first data character as a witness version.
    ///
    /// ## WARNING
    ///
    /// You almost certainly do not want to use this function.
    ///
    /// It is provided for backwards comparability to parse addresses that have an non-zero witness
    /// version because [BIP-173] explicitly allows using the bech32 checksum with any witness
    /// version however [BIP-350] specifies all witness version > 0 now MUST use bech32m.
    ///
    /// [BIP-173]: https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki
    /// [BIP-350]: https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki
    #[inline]
    pub fn new_bech32(s: &'s str) -> Result<Self, SegwitHrpstringError> {
        let unchecked = UncheckedHrpstring::new(s)?;
        let data_part = unchecked.data_part_ascii();

        // Unwrap ok since check_characters (in `Self::new`) checked the bech32-ness of this char.
        let witness_version = Fe32::from_char(data_part[0].into()).unwrap();
        if witness_version.to_u8() > 16 {
            return Err(SegwitHrpstringError::InvalidWitnessVersion(witness_version));
        }

        let checked = unchecked.validate_and_remove_checksum::<Bech32>()?;
        checked.validate_segwit()
    }

    /// Returns `true` if the HRP is "bc" or "tb".
    ///
    /// BIP-173 requires that the HRP is "bc" or "tb" but software in the Bitcoin ecosystem uses
    /// other HRPs, specifically "bcrt" for regtest addresses. We provide this function in order to
    /// be BIP-173 compliant but their are no restrictions on the HRP of [`SegwitHrpstring`].
    #[inline]
    pub fn has_valid_hrp(&self) -> bool { self.hrp().is_valid_segwit() }

    /// Returns the human-readable part.
    #[inline]
    pub fn hrp(&self) -> Hrp { self.hrp }

    /// Returns the witness version.
    #[inline]
    pub fn witness_version(&self) -> Fe32 { self.witness_version }

    /// Returns a partial slice of the data part, as ASCII bytes, everything after the witness
    /// version and before the checksum.
    ///
    /// The byte values are guaranteed to be valid bech32 characters.
    ///
    /// # Examples
    ///
    /// ```
    /// use bech32::{Bech32, primitives::decode::SegwitHrpstring};
    ///
    /// let addr = "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
    /// let ascii = "ar0srrr7xfkvy5l643lydnw9re59gtzz";
    ///
    /// let segwit = SegwitHrpstring::new(&addr).unwrap();
    /// assert!(segwit.data_part_ascii_no_witver_no_checksum().iter().eq(ascii.as_bytes().iter()))
    /// ```
    #[inline]
    pub fn data_part_ascii_no_witver_no_checksum(&self) -> &'s [u8] { self.ascii }

    /// Returns an iterator that yields the data part, excluding the witness version, of the parsed
    /// bech32 encoded string.
    ///
    /// Converts the ASCII bytes representing field elements to the respective field elements, then
    /// converts the stream of field elements to a stream of bytes.
    ///
    /// Use `self.witness_version()` to get the witness version.
    #[inline]
    pub fn byte_iter(&self) -> ByteIter<'_> {
        ByteIter { iter: AsciiToFe32Iter { iter: self.ascii.iter().copied() }.fes_to_bytes() }
    }
}

/// Checks whether a given HRP string has data part characters in the bech32 alphabet (incl.
/// checksum characters), and that the whole string has consistent casing (hrp and data part).
///
/// # Returns
///
/// The byte-index into the string where the '1' separator occurs, or an error if it does not.
fn check_characters(s: &str) -> Result<usize, CharError> {
    use CharError::*;

    let mut has_upper = false;
    let mut has_lower = false;
    let mut req_bech32 = true;
    let mut sep_pos = None;
    for (n, ch) in s.char_indices().rev() {
        if ch == SEP && sep_pos.is_none() {
            req_bech32 = false;
            sep_pos = Some(n);
        }
        if req_bech32 {
            Fe32::from_char(ch).map_err(|_| InvalidChar(ch))?;
        }
        if ch.is_ascii_uppercase() {
            has_upper = true;
        } else if ch.is_ascii_lowercase() {
            has_lower = true;
        }
    }
    if has_upper && has_lower {
        Err(MixedCase)
    } else if let Some(pos) = sep_pos {
        Ok(pos)
    } else {
        Err(MissingSeparator)
    }
}

/// An iterator over a parsed HRP string data as bytes.
pub struct ByteIter<'s> {
    iter: FesToBytes<AsciiToFe32Iter<'s>>,
}

impl<'s> Iterator for ByteIter<'s> {
    type Item = u8;
    #[inline]
    fn next(&mut self) -> Option<u8> { self.iter.next() }
    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) { self.iter.size_hint() }
}

impl<'s> ExactSizeIterator for ByteIter<'s> {
    #[inline]
    fn len(&self) -> usize { self.iter.len() }
}

/// An iterator over a parsed HRP string data as field elements.
pub struct Fe32Iter<'s> {
    iter: AsciiToFe32Iter<'s>,
}

impl<'s> Iterator for Fe32Iter<'s> {
    type Item = Fe32;
    #[inline]
    fn next(&mut self) -> Option<Fe32> { self.iter.next() }
    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) { self.iter.size_hint() }
}

/// Iterator adaptor that maps an iterator of valid bech32 character ASCII bytes to an
/// iterator of field elements.
///
/// # Panics
///
/// If any `u8` in the input iterator is out of range for an [`Fe32`]. Should only be used on data
/// that has already been checked for validity (eg, by using `check_characters`).
pub struct AsciiToFe32Iter<'s> {
    iter: iter::Copied<slice::Iter<'s, u8>>,
}

impl<'s> Iterator for AsciiToFe32Iter<'s> {
    type Item = Fe32;
    #[inline]
    fn next(&mut self) -> Option<Fe32> { self.iter.next().map(Fe32::from_char_unchecked) }
    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        // Each ASCII character is an fe32 so iterators are the same size.
        self.iter.size_hint()
    }
}

impl<'s> ExactSizeIterator for AsciiToFe32Iter<'s> {
    #[inline]
    fn len(&self) -> usize { self.iter.len() }
}

/// An error while constructing a [`SegwitHrpstring`] type.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum SegwitHrpstringError {
    /// Error while parsing the encoded address string.
    Unchecked(UncheckedHrpstringError),
    /// No data found after removing the checksum.
    NoData,
    /// String exceeds maximum allowed length.
    TooLong(usize),
    /// Invalid witness version (must be 0-16 inclusive).
    InvalidWitnessVersion(Fe32),
    /// Invalid padding on the witness data.
    Padding(PaddingError),
    /// Invalid witness length.
    WitnessLength(WitnessLengthError),
    /// Invalid checksum.
    Checksum(ChecksumError),
}

#[rustfmt::skip]
impl fmt::Display for SegwitHrpstringError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use SegwitHrpstringError::*;

        match *self {
            Unchecked(ref e) => write_err!(f, "parsing unchecked hrpstring failed"; e),
            NoData => write!(f, "no data found after removing the checksum"),
            TooLong(len) =>
                write!(f, "encoded length {} exceeds spec limit {} chars", len, segwit::MAX_STRING_LENGTH),
            InvalidWitnessVersion(fe) =>
                write!(f, "invalid segwit witness version: {} (bech32 character: '{}')", fe.to_u8(), fe),
            Padding(ref e) => write_err!(f, "invalid padding on the witness data"; e),
            WitnessLength(ref e) => write_err!(f, "invalid witness length"; e),
            Checksum(ref e) => write_err!(f, "invalid checksum"; e),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for SegwitHrpstringError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        use SegwitHrpstringError::*;

        match *self {
            Unchecked(ref e) => Some(e),
            Padding(ref e) => Some(e),
            WitnessLength(ref e) => Some(e),
            Checksum(ref e) => Some(e),
            NoData | TooLong(_) | InvalidWitnessVersion(_) => None,
        }
    }
}

impl From<UncheckedHrpstringError> for SegwitHrpstringError {
    #[inline]
    fn from(e: UncheckedHrpstringError) -> Self { Self::Unchecked(e) }
}

impl From<WitnessLengthError> for SegwitHrpstringError {
    #[inline]
    fn from(e: WitnessLengthError) -> Self { Self::WitnessLength(e) }
}

impl From<PaddingError> for SegwitHrpstringError {
    #[inline]
    fn from(e: PaddingError) -> Self { Self::Padding(e) }
}

impl From<ChecksumError> for SegwitHrpstringError {
    #[inline]
    fn from(e: ChecksumError) -> Self { Self::Checksum(e) }
}

/// An error while constructing a [`CheckedHrpstring`] type.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum CheckedHrpstringError {
    /// Error while parsing the encoded address string.
    Parse(UncheckedHrpstringError),
    /// Invalid checksum.
    Checksum(ChecksumError),
}

impl fmt::Display for CheckedHrpstringError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use CheckedHrpstringError::*;

        match *self {
            Parse(ref e) => write_err!(f, "parse failed"; e),
            Checksum(ref e) => write_err!(f, "invalid checksum"; e),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for CheckedHrpstringError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        use CheckedHrpstringError::*;

        match *self {
            Parse(ref e) => Some(e),
            Checksum(ref e) => Some(e),
        }
    }
}

impl From<UncheckedHrpstringError> for CheckedHrpstringError {
    #[inline]
    fn from(e: UncheckedHrpstringError) -> Self { Self::Parse(e) }
}

impl From<ChecksumError> for CheckedHrpstringError {
    #[inline]
    fn from(e: ChecksumError) -> Self { Self::Checksum(e) }
}

/// Errors when parsing a bech32 encoded string.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum UncheckedHrpstringError {
    /// An error with the characters of the input string.
    Char(CharError),
    /// The human-readable part is invalid.
    Hrp(hrp::Error),
}

impl fmt::Display for UncheckedHrpstringError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use UncheckedHrpstringError::*;

        match *self {
            Char(ref e) => write_err!(f, "character error"; e),
            Hrp(ref e) => write_err!(f, "invalid human-readable part"; e),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for UncheckedHrpstringError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        use UncheckedHrpstringError::*;

        match *self {
            Char(ref e) => Some(e),
            Hrp(ref e) => Some(e),
        }
    }
}

impl From<CharError> for UncheckedHrpstringError {
    #[inline]
    fn from(e: CharError) -> Self { Self::Char(e) }
}

impl From<hrp::Error> for UncheckedHrpstringError {
    #[inline]
    fn from(e: hrp::Error) -> Self { Self::Hrp(e) }
}

/// Character errors in a bech32 encoded string.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum CharError {
    /// String does not contain the separator character.
    MissingSeparator,
    /// No characters after the separator.
    NothingAfterSeparator,
    /// Some part of the string contains an invalid character.
    InvalidChar(char),
    /// The whole string must be of one case.
    MixedCase,
}

impl fmt::Display for CharError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use CharError::*;

        match *self {
            MissingSeparator => write!(f, "missing human-readable separator, \"{}\"", SEP),
            NothingAfterSeparator => write!(f, "invalid data - no characters after the separator"),
            InvalidChar(n) => write!(f, "invalid character (code={})", n),
            MixedCase => write!(f, "mixed-case strings not allowed"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for CharError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        use CharError::*;

        match *self {
            MissingSeparator | NothingAfterSeparator | InvalidChar(_) | MixedCase => None,
        }
    }
}

/// Errors in the checksum of a bech32 encoded string.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum ChecksumError {
    /// String exceeds maximum allowed length.
    CodeLength(CodeLengthError),
    /// The checksum residue is not valid for the data.
    InvalidResidue,
    /// The checksummed string is not a valid length.
    InvalidLength,
}

impl fmt::Display for ChecksumError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use ChecksumError::*;

        match *self {
            CodeLength(ref e) => write_err!(f, "string exceeds maximum allowed length"; e),
            InvalidResidue => write!(f, "the checksum residue is not valid for the data"),
            InvalidLength => write!(f, "the checksummed string is not a valid length"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ChecksumError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        use ChecksumError::*;

        match *self {
            CodeLength(ref e) => Some(e),
            InvalidResidue | InvalidLength => None,
        }
    }
}

/// Encoding HRP and data into a bech32 string exceeds the checksum code length.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub struct CodeLengthError {
    /// The length of the string if encoded with checksum.
    pub encoded_length: usize,
    /// The checksum specific code length.
    pub code_length: usize,
}

impl fmt::Display for CodeLengthError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "encoded length {} exceeds maximum (code length) {}",
            self.encoded_length, self.code_length
        )
    }
}

#[cfg(feature = "std")]
impl std::error::Error for CodeLengthError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> { None }
}

/// Encoding HRP, witver, and program into an address exceeds maximum allowed.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub struct SegwitCodeLengthError(pub usize);

impl fmt::Display for SegwitCodeLengthError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "encoded length {} exceeds maximum (code length) {}",
            self.0,
            segwit::MAX_STRING_LENGTH
        )
    }
}

#[cfg(feature = "std")]
impl std::error::Error for SegwitCodeLengthError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> { None }
}

impl From<CodeLengthError> for SegwitCodeLengthError {
    fn from(e: CodeLengthError) -> Self { Self(e.encoded_length) }
}

/// Error validating the padding bits on the witness data.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum PaddingError {
    /// The data payload has too many bits of padding.
    TooMuch,
    /// The data payload is padded with non-zero bits.
    NonZero,
}

impl fmt::Display for PaddingError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use PaddingError::*;

        match *self {
            TooMuch => write!(f, "the data payload has too many bits of padding"),
            NonZero => write!(f, "the data payload is padded with non-zero bits"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for PaddingError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        use PaddingError::*;

        match *self {
            TooMuch | NonZero => None,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn bip_173_invalid_parsing_fails() {
        use UncheckedHrpstringError::*;

        let invalid: Vec<(&str, UncheckedHrpstringError)> = vec!(
            ("\u{20}1nwldj5",
             // TODO: Rust >= 1.59.0 use Hrp(hrp::Error::InvalidAsciiByte('\u{20}'.try_into().unwrap()))),
             Hrp(hrp::Error::InvalidAsciiByte(32))),
            ("\u{7F}1axkwrx",
             Hrp(hrp::Error::InvalidAsciiByte(127))),
            ("\u{80}1eym55h",
             Hrp(hrp::Error::NonAsciiChar('\u{80}'))),
            ("an84characterslonghumanreadablepartthatcontainsthetheexcludedcharactersbioandnumber11d6pts4",
             Hrp(hrp::Error::TooLong(84))),
            ("pzry9x0s0muk",
             Char(CharError::MissingSeparator)),
            ("1pzry9x0s0muk",
             Hrp(hrp::Error::Empty)),
            ("x1b4n0q5v",
             Char(CharError::InvalidChar('b'))),
            // "li1dgmt3" in separate test because error is a checksum error.
            ("de1lg7wt\u{ff}",
             Char(CharError::InvalidChar('\u{ff}'))),
            // "A1G7SGD8" in separate test because error is a checksum error.
            ("10a06t8",
             Hrp(hrp::Error::Empty)),
            ("1qzzfhee",
             Hrp(hrp::Error::Empty)),
        );

        for (s, want) in invalid {
            let got = UncheckedHrpstring::new(s).unwrap_err();
            assert_eq!(got, want);
        }
    }

    #[test]
    fn bip_173_invalid_parsing_fails_invalid_checksum() {
        use ChecksumError::*;

        let err = UncheckedHrpstring::new("li1dgmt3")
            .expect("string parses correctly")
            .validate_checksum::<Bech32>()
            .unwrap_err();
        assert_eq!(err, InvalidLength);

        let err = UncheckedHrpstring::new("A1G7SGD8")
            .expect("string parses correctly")
            .validate_checksum::<Bech32>()
            .unwrap_err();
        assert_eq!(err, InvalidResidue);
    }

    #[test]
    fn bip_350_invalid_parsing_fails() {
        use UncheckedHrpstringError::*;

        let invalid: Vec<(&str, UncheckedHrpstringError)> = vec!(
            ("\u{20}1xj0phk",
             // TODO: Rust >= 1.59.0 use Hrp(hrp::Error::InvalidAsciiByte('\u{20}'.try_into().unwrap()))),
             Hrp(hrp::Error::InvalidAsciiByte(32))),
            ("\u{7F}1g6xzxy",
             Hrp(hrp::Error::InvalidAsciiByte(127))),
            ("\u{80}1g6xzxy",
             Hrp(hrp::Error::NonAsciiChar('\u{80}'))),
            ("an84characterslonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1569pvx",
             Hrp(hrp::Error::TooLong(84))),
            ("qyrz8wqd2c9m",
             Char(CharError::MissingSeparator)),
            ("1qyrz8wqd2c9m",
             Hrp(hrp::Error::Empty)),
            ("y1b0jsk6g",
             Char(CharError::InvalidChar('b'))),
            ("lt1igcx5c0",
             Char(CharError::InvalidChar('i'))),
            // "in1muywd" in separate test because error is a checksum error.
            ("mm1crxm3i",
             Char(CharError::InvalidChar('i'))),
            ("au1s5cgom",
             Char(CharError::InvalidChar('o'))),
            // "M1VUXWEZ" in separate test because error is a checksum error.
            ("16plkw9",
             Hrp(hrp::Error::Empty)),
            ("1p2gdwpf",
             Hrp(hrp::Error::Empty)),

        );

        for (s, want) in invalid {
            let got = UncheckedHrpstring::new(s).unwrap_err();
            assert_eq!(got, want);
        }
    }

    #[test]
    fn bip_350_invalid_because_of_invalid_checksum() {
        use ChecksumError::*;

        // Note the "bc1p2" test case is not from the bip test vectors.
        let invalid: Vec<&str> = vec!["in1muywd", "bc1p2"];

        for s in invalid {
            let err =
                UncheckedHrpstring::new(s).unwrap().validate_checksum::<Bech32m>().unwrap_err();
            assert_eq!(err, InvalidLength);
        }

        let err = UncheckedHrpstring::new("M1VUXWEZ")
            .unwrap()
            .validate_checksum::<Bech32m>()
            .unwrap_err();
        assert_eq!(err, InvalidResidue);
    }

    #[test]
    fn check_hrp_uppercase_returns_lower() {
        let addr = "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4";
        let unchecked = UncheckedHrpstring::new(addr).expect("failed to parse address");
        assert_eq!(unchecked.hrp(), Hrp::parse_unchecked("bc"));
    }

    #[test]
    #[cfg(feature = "alloc")]
    fn check_hrp_max_length() {
        let hrps =
            "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio";

        let hrp = Hrp::parse_unchecked(hrps);
        let s = crate::encode::<Bech32>(hrp, &[]).expect("failed to encode empty buffer");

        let unchecked = UncheckedHrpstring::new(&s).expect("failed to parse address");
        assert_eq!(unchecked.hrp(), hrp);
    }

    #[test]
    fn mainnet_valid_addresses() {
        let addresses = vec![
            "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq",
            "23451QAR0SRRR7XFKVY5L643LYDNW9RE59GTZZLKULZK",
        ];
        for valid in addresses {
            assert!(CheckedHrpstring::new::<Bech32>(valid).is_ok())
        }
    }

    macro_rules! check_invalid_segwit_addresses {
        ($($test_name:ident, $reason:literal, $address:literal);* $(;)?) => {
            $(
                #[test]
                fn $test_name() {
                    let res = SegwitHrpstring::new($address);
                    if res.is_ok() {
                        panic!("{} sting should not be valid: {}", $address, $reason);
                    }
                }
            )*
        }
    }
    check_invalid_segwit_addresses! {
        invalid_segwit_address_0, "missing hrp", "1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
        invalid_segwit_address_1, "missing data-checksum", "91111";
        invalid_segwit_address_2, "invalid witness version", "bc14r0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq";
        invalid_segwit_address_3, "invalid checksum length", "bc1q5mdq";
        invalid_segwit_address_4, "missing data", "bc1qwf5mdq";
        invalid_segwit_address_5, "invalid program length", "bc14r0srrr7xfkvy5l643lydnw9rewf5mdq";
    }
}
