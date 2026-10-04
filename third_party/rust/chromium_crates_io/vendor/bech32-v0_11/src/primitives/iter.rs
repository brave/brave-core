// SPDX-License-Identifier: MIT

//! Iterator Adaptors.
//!
//! Iterator extension traits and blanket implementations to convert:
//!
//! - `BytesToFes`: An iterator over bytes to an iterator over field elements.
//! - `FesToBytes`: An iterator over field elements to an iterator over bytes.
//! - `Checksummed`: An iterator over field elements that appends the checksum.
//!
//! WARNING: This module does not enforce the maximum length of an encoded bech32 string (90 chars).
//!
//! # Examples
//!
//! ```
//! use bech32::{Bech32, ByteIterExt, Fe32IterExt, Fe32, Hrp};
//!
//! let data = [
//!     0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4,
//!     0x54, 0x94, 0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23,
//!     0xf1, 0x43, 0x3b, 0xd6,
//! ];
//!
//! // Convert byte data to GF32 field elements.
//! let fe_iter = data.iter().copied().bytes_to_fes();
//!
//! // Convert field elements back to bytes.
//! let byte_iter = fe_iter.fes_to_bytes();
//!
//! # assert!(data.iter().copied().eq(byte_iter));
//! ```

use crate::primitives::checksum::{self, Checksum, PackedFe32};
use crate::primitives::encode::Encoder;
use crate::primitives::gf32::Fe32;
use crate::primitives::hrp::Hrp;

/// Extension trait for byte iterators which provides an adaptor to GF32 elements.
pub trait ByteIterExt: Sized + Iterator<Item = u8> {
    /// Adapts the byte iterator to output GF32 field elements instead.
    ///
    /// If the total number of bits is not a multiple of 5 we pad with 0s
    #[inline]
    fn bytes_to_fes(mut self) -> BytesToFes<Self> {
        BytesToFes { last_byte: self.next(), bit_offset: 0, iter: self }
    }
}

impl<I> ByteIterExt for I where I: Iterator<Item = u8> {}

/// Extension trait for field element iterators.
pub trait Fe32IterExt: Sized + Iterator<Item = Fe32> {
    /// Adapts the `Fe32` iterator to output bytes instead.
    ///
    /// If the total number of bits is not a multiple of 8, any trailing bits
    /// are simply dropped.
    #[inline]
    fn fes_to_bytes(mut self) -> FesToBytes<Self> {
        FesToBytes { last_fe: self.next(), bit_offset: 0, iter: self }
    }

    /// Adapts the Fe32 iterator to encode the field elements into a bech32 address.
    #[inline]
    fn with_checksum<Ck: Checksum>(self, hrp: &Hrp) -> Encoder<'_, Self, Ck> {
        Encoder::new(self, hrp)
    }
}

impl<I> Fe32IterExt for I where I: Iterator<Item = Fe32> {}

/// Iterator adaptor that converts bytes to GF32 elements.
///
/// If the total number of bits is not a multiple of 5, it right-pads with 0 bits.
#[derive(Clone, PartialEq, Eq)]
pub struct BytesToFes<I: Iterator<Item = u8>> {
    last_byte: Option<u8>,
    bit_offset: usize,
    iter: I,
}

impl<I> Iterator for BytesToFes<I>
where
    I: Iterator<Item = u8>,
{
    type Item = Fe32;

    #[inline]
    fn next(&mut self) -> Option<Fe32> {
        use core::cmp::Ordering::*;

        let bit_offset = {
            let ret = self.bit_offset;
            self.bit_offset = (self.bit_offset + 5) % 8;
            ret
        };

        if let Some(last) = self.last_byte {
            match bit_offset.cmp(&3) {
                Less => Some(Fe32((last >> (3 - bit_offset)) & 0x1f)),
                Equal => {
                    self.last_byte = self.iter.next();
                    Some(Fe32(last & 0x1f))
                }
                Greater => {
                    self.last_byte = self.iter.next();
                    let next = self.last_byte.unwrap_or(0);
                    Some(Fe32(((last << (bit_offset - 3)) | (next >> (11 - bit_offset))) & 0x1f))
                }
            }
        } else {
            None
        }
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let (min, max) = self.iter.size_hint();
        let (min, max) = match self.last_byte {
            // +1 because we set last_byte with call to `next`.
            Some(_) => (min + 1, max.map(|max| max + 1)),
            None => (min, max),
        };

        let min = bytes_len_to_fes_len(min);
        let max = max.map(bytes_len_to_fes_len);

        (min, max)
    }
}

/// The number of fes encoded by n bytes, rounded up because we pad the fes.
fn bytes_len_to_fes_len(bytes: usize) -> usize {
    let bits = bytes * 8;
    (bits + 4) / 5
}

impl<I> ExactSizeIterator for BytesToFes<I>
where
    I: Iterator<Item = u8> + ExactSizeIterator,
{
    #[inline]
    fn len(&self) -> usize {
        let len = match self.last_byte {
            Some(_) => self.iter.len() + 1,
            None => self.iter.len(),
        };
        bytes_len_to_fes_len(len)
    }
}

/// Iterator adaptor that converts GF32 elements to bytes.
///
/// If the total number of bits is not a multiple of 8, any trailing bits are dropped.
///
/// Note that if there are 5 or more trailing bits, the result will be that an entire field element
/// is dropped. If this occurs, the input was an invalid length for a bech32 string, but this
/// iterator does not do any checks for this.
#[derive(Clone, PartialEq, Eq)]
pub struct FesToBytes<I: Iterator<Item = Fe32>> {
    last_fe: Option<Fe32>,
    bit_offset: usize,
    iter: I,
}

impl<I> Iterator for FesToBytes<I>
where
    I: Iterator<Item = Fe32>,
{
    type Item = u8;

    fn next(&mut self) -> Option<u8> {
        let bit_offset = {
            let ret = self.bit_offset;
            self.bit_offset = (self.bit_offset + 8) % 5;
            ret
        };

        if let Some(last) = self.last_fe {
            let mut ret = last.0 << (3 + bit_offset);

            self.last_fe = self.iter.next();
            let next1 = self.last_fe?;
            if bit_offset > 2 {
                self.last_fe = self.iter.next();
                let next2 = self.last_fe?;
                ret |= next1.0 << (bit_offset - 2);
                ret |= next2.0 >> (7 - bit_offset);
            } else {
                ret |= next1.0 >> (2 - bit_offset);
                if self.bit_offset == 0 {
                    self.last_fe = self.iter.next();
                }
            }

            Some(ret)
        } else {
            None
        }
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        // If the total number of bits is not a multiple of 8, any trailing bits are dropped.
        let fes_len_to_bytes_len = |n| n * 5 / 8;

        let (fes_min, fes_max) = self.iter.size_hint();
        // +1 because we set last_fe with call to `next`.
        let min = fes_len_to_bytes_len(fes_min + 1);
        let max = fes_max.map(|max| fes_len_to_bytes_len(max + 1));
        (min, max)
    }
}

// If the total number of bits is not a multiple of 8, any trailing bits are dropped.
fn fes_len_to_bytes_len(n: usize) -> usize { n * 5 / 8 }

impl<I> ExactSizeIterator for FesToBytes<I>
where
    I: Iterator<Item = Fe32> + ExactSizeIterator,
{
    #[inline]
    fn len(&self) -> usize {
        let len = match self.last_fe {
            Some(_) => self.iter.len() + 1,
            None => self.iter.len(),
        };
        fes_len_to_bytes_len(len)
    }
}

/// Iterator adaptor for field-element-yielding iterator, which tacks a checksum onto the end of the
/// yielded data.
#[derive(Clone, PartialEq, Eq)]
pub struct Checksummed<I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    iter: I,
    checksum_remaining: usize,
    checksum_engine: checksum::Engine<Ck>,
}

impl<I, Ck> Checksummed<I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    /// Creates a new checksummed iterator which adapts a data iterator of field elements by
    /// appending a checksum.
    #[inline]
    pub fn new(data: I) -> Checksummed<I, Ck> {
        Checksummed {
            iter: data,
            checksum_remaining: Ck::CHECKSUM_LENGTH,
            checksum_engine: checksum::Engine::new(),
        }
    }

    /// Creates a new checksummed iterator which adapts a data iterator of field elements by
    /// first inputting the [`Hrp`] and then appending a checksum.
    #[inline]
    pub fn new_hrp(hrp: Hrp, data: I) -> Checksummed<I, Ck> {
        let mut ret = Self::new(data);
        ret.checksum_engine.input_hrp(hrp);
        ret
    }
}

impl<I, Ck> Iterator for Checksummed<I, Ck>
where
    I: Iterator<Item = Fe32>,
    Ck: Checksum,
{
    type Item = Fe32;

    #[inline]
    fn next(&mut self) -> Option<Fe32> {
        match self.iter.next() {
            Some(fe) => {
                self.checksum_engine.input_fe(fe);
                Some(fe)
            }
            None =>
                if self.checksum_remaining == 0 {
                    None
                } else {
                    if self.checksum_remaining == Ck::CHECKSUM_LENGTH {
                        self.checksum_engine.input_target_residue();
                    }
                    self.checksum_remaining -= 1;
                    Some(Fe32(self.checksum_engine.residue().unpack(self.checksum_remaining)))
                },
        }
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let add = self.checksum_remaining;
        let (min, max) = self.iter.size_hint();

        (min + add, max.map(|max| max + add))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Tests below using this data, are based on the test vector (from BIP-173):
    // BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4: 0014751e76e8199196d454941c45d1b3a323f1433bd6
    #[rustfmt::skip]
    const DATA: [u8; 20] = [
        0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4,
        0x54, 0x94, 0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23,
        0xf1, 0x43, 0x3b, 0xd6,
    ];

    #[test]
    fn byte_iter_ext() {
        assert!(DATA
            .iter()
            .copied()
            .bytes_to_fes()
            .map(Fe32::to_char)
            .eq("w508d6qejxtdg4y5r3zarvary0c5xw7k".chars()));
    }

    #[test]
    fn bytes_to_fes_size_hint() {
        let char_len = "w508d6qejxtdg4y5r3zarvary0c5xw7k".len();
        assert_eq!(DATA.iter().copied().bytes_to_fes().size_hint(), (char_len, Some(char_len)));
    }

    #[test]
    fn fe32_iter_ext() {
        let fe_iter = "w508d6qejxtdg4y5r3zarvary0c5xw7k"
            .bytes()
            .map(|b| Fe32::from_char(char::from(b)).unwrap());

        assert!(fe_iter.clone().fes_to_bytes().eq(DATA.iter().copied()));
    }

    #[test]
    fn fes_to_bytes_size_hint() {
        let fe_iter = "w508d6qejxtdg4y5r3zarvary0c5xw7k"
            .bytes()
            .map(|b| Fe32::from_char(char::from(b)).unwrap());

        let got_hint = fe_iter.clone().fes_to_bytes().size_hint();
        let want_hint = DATA.iter().size_hint();

        assert_eq!(got_hint, want_hint)
    }

    #[test]
    fn padding_bytes_trailing_0_bits_roundtrips() {
        // 5 * 8 % 5 = 0
        const BYTES: [u8; 5] = [0x75, 0x1e, 0x76, 0xe8, 0x19];
        assert!(BYTES.iter().copied().bytes_to_fes().fes_to_bytes().eq(BYTES.iter().copied()))
    }

    #[test]
    fn padding_bytes_trailing_1_bit_roundtrips() {
        // 2 * 8 % 5 = 1
        const BYTES: [u8; 2] = [0x75, 0x1e];
        assert!(BYTES.iter().copied().bytes_to_fes().fes_to_bytes().eq(BYTES.iter().copied()))
    }

    #[test]
    fn padding_bytes_trailing_2_bits_roundtrips() {
        // 4 * 8 % 5 = 2
        const BYTES: [u8; 4] = [0x75, 0x1e, 0x76, 0xe8];
        assert!(BYTES.iter().copied().bytes_to_fes().fes_to_bytes().eq(BYTES.iter().copied()))
    }

    #[test]
    fn padding_bytes_trailing_3_bits_roundtrips() {
        // 6 * 8 % 5 = 3
        const BYTES: [u8; 6] = [0x75, 0x1e, 0x76, 0xe8, 0x19, 0xab];
        assert!(BYTES.iter().copied().bytes_to_fes().fes_to_bytes().eq(BYTES.iter().copied()))
    }

    #[test]
    fn padding_bytes_trailing_4_bits_roundtrips() {
        // 3 * 8 % 5 = 4
        const BYTES: [u8; 3] = [0x75, 0x1e, 0x76];
        assert!(BYTES.iter().copied().bytes_to_fes().fes_to_bytes().eq(BYTES.iter().copied()))
    }

    #[test]
    fn padding_fes_trailing_0_bits_roundtrips() {
        // 8 * 5 % 8 = 0
        const FES: [Fe32; 8] =
            [Fe32::Q, Fe32::P, Fe32::Z, Fe32::R, Fe32::Y, Fe32::X, Fe32::G, Fe32::F];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    fn padding_fes_trailing_1_bit_zero_roundtrips() {
        // 5 * 5 % 8 = 1
        const FES: [Fe32; 5] = [Fe32::Q, Fe32::P, Fe32::Z, Fe32::R, Fe32::Q];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    #[should_panic]
    fn padding_fes_trailing_1_bit_non_zero_does_not_roundtrip() {
        // 5 * 5 % 8 = 1
        const FES: [Fe32; 5] = [Fe32::Q, Fe32::P, Fe32::Z, Fe32::R, Fe32::L];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    fn padding_fes_trailing_2_bits_zeros_roundtrips() {
        // 2 * 5 % 8 = 2
        const FES: [Fe32; 2] = [Fe32::P, Fe32::Q];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    #[should_panic]
    fn padding_fes_trailing_2_bits_non_zero_does_not_roundtrip() {
        // 2 * 5 % 8 = 2
        const FES: [Fe32; 2] = [Fe32::Q, Fe32::P];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    fn padding_fes_trailing_3_bits_zeros_roundtrips() {
        // 7 * 5 % 8 = 3
        const FES: [Fe32; 7] = [Fe32::Q, Fe32::P, Fe32::Z, Fe32::R, Fe32::Y, Fe32::X, Fe32::Q];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    #[should_panic]
    fn padding_fes_trailing_3_bits_non_zero_does_not_roundtrip() {
        // 7 * 5 % 8 = 3
        const FES: [Fe32; 7] = [Fe32::Q, Fe32::P, Fe32::Z, Fe32::R, Fe32::Y, Fe32::X, Fe32::P];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    fn padding_fes_trailing_4_bits_zeros_roundtrips() {
        // 4 * 5 % 8 = 4
        const FES: [Fe32; 4] = [Fe32::Q, Fe32::P, Fe32::Z, Fe32::Q];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    #[should_panic]
    fn padding_fes_trailing_4_bits_non_zero_does_not_roundtrip() {
        // 4 * 5 % 8 = 4
        const FES: [Fe32; 4] = [Fe32::Q, Fe32::P, Fe32::Z, Fe32::P];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    // Padding is never more than 4 bits so any additional bits will always fail to roundtrip.

    #[test]
    #[should_panic]
    fn padding_fes_trailing_5_bits_zeros_does_not_roundtrip() {
        // 1 * 5 % 8 = 5
        const FES: [Fe32; 1] = [Fe32::Q];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    #[should_panic]
    fn padding_fes_trailing_5_bits_non_zero_does_not_roundtrip() {
        // 1 * 5 % 8 = 5
        const FES: [Fe32; 1] = [Fe32::P];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    #[should_panic]
    fn padding_fes_trailing_6_bits_zeros_does_not_roundtrip() {
        // 6 * 5 % 8 = 6
        const FES: [Fe32; 6] = [Fe32::Q, Fe32::P, Fe32::Z, Fe32::R, Fe32::Q, Fe32::Q];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    #[should_panic]
    fn padding_fes_trailing_6_bits_non_zero_does_not_roundtrip() {
        // 6 * 5 % 8 = 6
        const FES: [Fe32; 6] = [Fe32::Q, Fe32::P, Fe32::Z, Fe32::R, Fe32::Y, Fe32::X];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    #[should_panic]
    fn padding_fes_trailing_7_bits_zeros_does_not_roundtrip() {
        // 3 * 5 % 8 = 7
        const FES: [Fe32; 3] = [Fe32::P, Fe32::Q, Fe32::Q];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }

    #[test]
    #[should_panic]
    fn padding_fes_trailing_7_bits_non_zero_does_not_roundtrip() {
        // 3 * 5 % 8 = 7
        const FES: [Fe32; 3] = [Fe32::Q, Fe32::P, Fe32::Q];
        assert!(FES.iter().copied().fes_to_bytes().bytes_to_fes().eq(FES.iter().copied()))
    }
}
