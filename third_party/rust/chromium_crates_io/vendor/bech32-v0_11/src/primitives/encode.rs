// SPDX-License-Identifier: MIT

//! Bech32 address encoding.
//!
//! This module provides types and iterators that can be used to encode data as a bech32 address in
//! a variety of ways without any allocations, generating, verifying, and appending checksums,
//! prepending HRP strings etc.
//!
//! In general, directly using these adaptors is not very ergonomic, and users are recommended to
//! instead use the crate level API.
//!
//! WARNING: This module does not enforce the maximum length of an encoded bech32 string (90 chars).
//!
//! # Examples
//!
//! ```
//! use bech32::{Bech32, ByteIterExt, Fe32IterExt, Fe32, Hrp};
//!
//! let witness_prog = [
//!     0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4,
//!     0x54, 0x94, 0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23,
//!     0xf1, 0x43, 0x3b, 0xd6,
//! ];
//!
//! // Get a stream of characters representing the bech32 encoded
//! // address using "bc" for the human-readable part.
//! let hrp = Hrp::parse("bc").expect("bc is valid hrp string");
//! let chars = witness_prog
//!     .iter()
//!     .copied()
//!     .bytes_to_fes()
//!     .with_checksum::<Bech32>(&hrp)
//!     .with_witness_version(Fe32::Q) // Optionally add witness version.
//!     .chars();
//!
//! #[cfg(feature = "alloc")]
//! {
//!     let addr = chars.collect::<String>();
//!     assert_eq!(addr.to_uppercase(), "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4");
//! }
//! ```

use core::iter::Iterator;
use core::marker::PhantomData;

use crate::primitives::checksum::HrpFe32Iter;
use crate::primitives::hrp::{self, Hrp};
use crate::primitives::iter::Checksummed;
use crate::{Checksum, Fe32};

/// The `Encoder` builds iterators that can be used to encode field elements into a bech32 address.
///
/// Construct the encoder by calling [`Fe32IterExt::with_checksum`] on an iterator of field
/// elements, optionally prefix the data with a witness version, and then get the encoding as either
/// a stream of characters ([`Encoder::chars`]) or a stream of field elements ([`Encoder::fes`]).
///
/// # Examples
///
/// ```
/// use bech32::{Bech32, ByteIterExt, Fe32IterExt, Hrp};
///
/// let data = [0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4];
///
/// let hrp = Hrp::parse("abc").expect("bc is valid hrp string");
/// let chars = data
///     .iter()
///     .copied()
///     .bytes_to_fes()
///     .with_checksum::<Bech32>(&hrp)
///     .chars();
/// ```
/// [`Fe32IterExt::with_checksum`]: crate::Fe32IterExt::with_checksum
#[derive(Clone, PartialEq, Eq)]
pub struct Encoder<'hrp, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    /// The field elements to encode.
    data: I,
    /// The human-readable part used at the front of the address encoding.
    hrp: &'hrp Hrp,
    /// The witness version, if present.
    witness_version: Option<Fe32>,
    /// Checksum marker.
    marker: PhantomData<Ck>,
}

impl<'hrp, I, Ck> Encoder<'hrp, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    /// Constructs a new bech32 encoder.
    #[inline]
    pub fn new(data: I, hrp: &'hrp Hrp) -> Self {
        Self { data, hrp, witness_version: None, marker: PhantomData::<Ck> }
    }

    /// Adds `witness_version` to the encoder (as first byte of encoded data).
    ///
    /// Note, caller to guarantee that witness version is within valid range (0-16).
    #[inline]
    pub fn with_witness_version(mut self, witness_version: Fe32) -> Self {
        self.witness_version = Some(witness_version);
        self
    }

    /// Returns an iterator that yields the bech32 encoded address as field ASCII characters.
    #[inline]
    pub fn chars(self) -> CharIter<'hrp, I, Ck> {
        let witver_iter = WitnessVersionIter::new(self.witness_version, self.data);
        CharIter::new(self.hrp, witver_iter)
    }

    /// Returns an iterator that yields the bech32 encoded address as field ASCII characters, as
    /// byte values.
    #[inline]
    pub fn bytes(self) -> ByteIter<'hrp, I, Ck> {
        let char_iter = self.chars();
        ByteIter::new(char_iter)
    }

    /// Returns an iterator that yields the field elements that go into the checksum, as well as the checksum at the end.
    ///
    /// Each field element yielded has been input into the checksum algorithm (including the HRP as it is fed into the algorithm).
    #[inline]
    pub fn fes(self) -> Fe32Iter<'hrp, I, Ck> {
        let witver_iter = WitnessVersionIter::new(self.witness_version, self.data);
        Fe32Iter::new(self.hrp, witver_iter)
    }
}

/// Iterator adaptor that just prepends a single character to a field element stream.
///
/// More ergonomic to use than `std::iter::once(fe).chain(iter)`.
pub struct WitnessVersionIter<I>
where
    I: Iterator<Item = Fe32>,
{
    witness_version: Option<Fe32>,
    iter: I,
}

impl<I> WitnessVersionIter<I>
where
    I: Iterator<Item = Fe32>,
{
    /// Creates a [`WitnessVersionIter`].
    #[inline]
    pub fn new(witness_version: Option<Fe32>, iter: I) -> Self { Self { witness_version, iter } }
}

impl<I> Iterator for WitnessVersionIter<I>
where
    I: Iterator<Item = Fe32>,
{
    type Item = Fe32;

    #[inline]
    fn next(&mut self) -> Option<Fe32> { self.witness_version.take().or_else(|| self.iter.next()) }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let (min, max) = self.iter.size_hint();
        match self.witness_version {
            Some(_) => (min + 1, max.map(|max| max + 1)),
            None => (min, max),
        }
    }
}

/// Iterator adaptor which takes a stream of field elements, converts it to characters prefixed by
/// an HRP (and separator), and suffixed by the checksum i.e., converts the data in a stream of
/// field elements into stream of characters representing the encoded bech32 string.
pub struct CharIter<'hrp, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    /// `None` once the hrp has been yielded.
    hrp_iter: Option<hrp::LowercaseCharIter<'hrp>>,
    /// Iterator over field elements made up of the optional witness version, the data to be
    /// encoded, plus the checksum.
    checksummed: Checksummed<WitnessVersionIter<I>, Ck>,
}

impl<'hrp, I, Ck> CharIter<'hrp, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    /// Adapts the `Fe32Iter` iterator to yield characters representing the bech32 encoding.
    #[inline]
    pub fn new(hrp: &'hrp Hrp, data: WitnessVersionIter<I>) -> Self {
        let checksummed = Checksummed::new_hrp(*hrp, data);
        Self { hrp_iter: Some(hrp.lowercase_char_iter()), checksummed }
    }
}

impl<'a, I, Ck> Iterator for CharIter<'a, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    type Item = char;

    #[inline]
    fn next(&mut self) -> Option<char> {
        if let Some(ref mut hrp_iter) = self.hrp_iter {
            match hrp_iter.next() {
                Some(c) => return Some(c),
                None => {
                    self.hrp_iter = None;
                    return Some('1');
                }
            }
        }

        self.checksummed.next().map(|fe| fe.to_char())
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        match &self.hrp_iter {
            // We have yielded the hrp and separator already.
            None => self.checksummed.size_hint(),
            // Yet to finish yielding the hrp (and the separator).
            Some(hrp_iter) => {
                let (hrp_min, hrp_max) = hrp_iter.size_hint();
                let (chk_min, chk_max) = self.checksummed.size_hint();

                let min = hrp_min + 1 + chk_min; // +1 for the separator.

                // To provide a max boundary we need to have gotten a value from the hrp iter as well as the
                // checksummed iter, otherwise we have to return None since we cannot know the maximum.
                let max = match (hrp_max, chk_max) {
                    (Some(hrp_max), Some(chk_max)) => Some(hrp_max + 1 + chk_max),
                    (_, _) => None,
                };

                (min, max)
            }
        }
    }
}

/// Iterator adaptor which takes a stream of ASCII field elements (an encoded string) and yields a stream of bytes.
///
/// This is equivalent to using the `CharsIter` and the casting each character to a byte. Doing
/// so is technically sound because we only yield ASCII characters but it makes for ugly code so
/// we provide this iterator also.
pub struct ByteIter<'hrp, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    char_iter: CharIter<'hrp, I, Ck>,
}

impl<'hrp, I, Ck> ByteIter<'hrp, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    /// Adapts the `CharIter` iterator to yield bytes representing the bech32 encoding as ASCII bytes.
    #[inline]
    pub fn new(char_iter: CharIter<'hrp, I, Ck>) -> Self { Self { char_iter } }
}

impl<'a, I, Ck> Iterator for ByteIter<'a, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    type Item = u8;

    #[inline]
    fn next(&mut self) -> Option<u8> { self.char_iter.next().map(|c| c as u8) }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) { self.char_iter.size_hint() }
}

/// Iterator adaptor for a checksummed iterator that inputs the HRP into the checksum algorithm
/// before yielding the HRP as field elements followed by the data then checksum.
pub struct Fe32Iter<'hrp, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    /// `None` once the hrp field elements have been yielded.
    hrp_iter: Option<HrpFe32Iter<'hrp>>,
    /// Iterator over field elements made up of the optional witness version, the data to be
    /// encoded, plus the checksum.
    checksummed: Checksummed<WitnessVersionIter<I>, Ck>,
}

impl<'hrp, I, Ck> Fe32Iter<'hrp, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    /// Creates a [`Fe32Iter`] which yields all the field elements which go into the checksum algorithm.
    #[inline]
    pub fn new(hrp: &'hrp Hrp, data: WitnessVersionIter<I>) -> Self {
        let hrp_iter = HrpFe32Iter::new(hrp);
        let checksummed = Checksummed::new_hrp(*hrp, data);
        Self { hrp_iter: Some(hrp_iter), checksummed }
    }
}

impl<'hrp, I, Ck> Iterator for Fe32Iter<'hrp, I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    type Item = Fe32;
    #[inline]
    fn next(&mut self) -> Option<Fe32> {
        if let Some(ref mut hrp_iter) = &mut self.hrp_iter {
            match hrp_iter.next() {
                Some(fe) => return Some(fe),
                None => self.hrp_iter = None,
            }
        }
        self.checksummed.next()
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let hrp = match &self.hrp_iter {
            Some(hrp_iter) => hrp_iter.size_hint(),
            None => (0, Some(0)),
        };

        let data = self.checksummed.size_hint();

        let min = hrp.0 + data.0;
        let max = hrp.1.zip(data.1).map(|(hrp, data)| hrp + data);

        (min, max)
    }
}

#[cfg(test)]
mod tests {
    use crate::{Bech32, ByteIterExt, Fe32, Fe32IterExt, Hrp};

    // Tests below using this data, are based on the test vector (from BIP-173):
    // BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4: 0014751e76e8199196d454941c45d1b3a323f1433bd6
    #[rustfmt::skip]
    const DATA: [u8; 20] = [
        0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4,
        0x54, 0x94, 0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23,
        0xf1, 0x43, 0x3b, 0xd6,
    ];

    #[test]
    fn hrpstring_iter() {
        let iter = DATA.iter().copied().bytes_to_fes();

        let hrp = Hrp::parse_unchecked("bc");
        let iter = iter.with_checksum::<Bech32>(&hrp).with_witness_version(Fe32::Q).chars();

        assert!(iter.eq("bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4".chars()));
    }

    #[test]
    #[cfg(feature = "alloc")]
    fn hrpstring_iter_collect() {
        let iter = DATA.iter().copied().bytes_to_fes();

        let hrp = Hrp::parse_unchecked("bc");
        let iter = iter.with_checksum::<Bech32>(&hrp).with_witness_version(Fe32::Q).chars();

        let encoded = iter.collect::<String>();
        assert_eq!(encoded, "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4");
    }

    #[test]
    fn hrpstring_iter_size_hint() {
        let char_len = "w508d6qejxtdg4y5r3zarvary0c5xw7k".len();
        let iter = DATA.iter().copied().bytes_to_fes();

        let hrp = Hrp::parse_unchecked("bc");
        let iter = iter.with_checksum::<Bech32>(&hrp).with_witness_version(Fe32::Q).chars();

        let checksummed_len = 2 + 1 + 1 + char_len + 6; // bc + SEP + Q + chars + checksum
        assert_eq!(iter.size_hint().0, checksummed_len);
    }

    #[test]
    #[cfg(feature = "alloc")]
    fn hrpstring_iter_bytes() {
        let hrp = Hrp::parse_unchecked("bc");
        let fes = DATA.iter().copied().bytes_to_fes();
        let iter = fes.with_checksum::<Bech32>(&hrp).with_witness_version(Fe32::Q);

        let chars = iter.clone().chars();
        let bytes = iter.bytes();

        for (c, b) in chars.zip(bytes) {
            assert_eq!(c as u8, b)
        }
    }
}
