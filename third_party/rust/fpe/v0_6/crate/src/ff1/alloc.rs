//! FF1 NumeralString implementations that require a global allocator.

use core::iter;

use alloc::{vec, vec::Vec};

use num_bigint::{BigInt, BigUint, Sign};
use num_traits::{
    identities::{One, Zero},
    ToPrimitive,
};

use super::{NumeralString, Operations};

fn pow(x: u32, e: usize) -> BigUint {
    let mut res = BigUint::one();
    for _ in 0..e {
        res *= x;
    }
    res
}

/// Extension trait adding FF1-relevant methods to `BigUint`.
trait Numeral {
    /// Type used for byte representations.
    type Bytes: AsRef<[u8]>;

    /// Returns the integer interpreted from the given bytes in big-endian order.
    fn from_bytes(s: impl Iterator<Item = u8>) -> Self;

    /// Returns the big-endian byte representation of this integer.
    fn to_bytes(&self, b: usize) -> Self::Bytes;

    /// Computes `(self + other) mod radix^m`.
    fn add_mod_exp(self, other: Self, radix: u32, m: usize) -> Self;

    /// Computes `(self - other) mod radix^m`.
    fn sub_mod_exp(self, other: Self, radix: u32, m: usize) -> Self;
}

impl Numeral for BigUint {
    type Bytes = Vec<u8>;

    fn from_bytes(s: impl Iterator<Item = u8>) -> Self {
        BigUint::from_bytes_be(&s.collect::<Vec<_>>())
    }

    fn to_bytes(&self, b: usize) -> Vec<u8> {
        if self.is_zero() {
            // Because self.to_bytes_be() returns vec![0u8] for zero, instead of vec![], we would
            // end up with a subtraction overflow on empty input (since (b - bytes.len()) < 0 or
            // (0 - 1) < 0). This optimization side-steps that special case.
            vec![0; b]
        } else {
            let mut bytes = self.to_bytes_le();
            let padding = b - bytes.len();
            bytes.reserve_exact(padding);
            bytes.extend(iter::repeat(0).take(padding));
            bytes.reverse();
            bytes
        }
    }

    fn add_mod_exp(self, other: Self, radix: u32, m: usize) -> Self {
        (self + other) % pow(radix, m)
    }

    fn sub_mod_exp(self, other: Self, radix: u32, m: usize) -> Self {
        let modulus = BigInt::from(pow(radix, m));
        let mut c = (BigInt::from(self) - BigInt::from(other)) % &modulus;
        if c.sign() == Sign::Minus {
            // use ((x % m) + m) % m to ensure it is in range
            c += &modulus;
            c %= modulus;
        }
        c.to_biguint().unwrap()
    }
}

/// A numeral string that supports radixes in [2..2^16).
#[cfg_attr(test, derive(Debug))]
pub struct FlexibleNumeralString(Vec<u16>);

impl From<Vec<u16>> for FlexibleNumeralString {
    fn from(v: Vec<u16>) -> Self {
        FlexibleNumeralString(v)
    }
}

impl From<FlexibleNumeralString> for Vec<u16> {
    fn from(fns: FlexibleNumeralString) -> Self {
        fns.0
    }
}

impl NumeralString for FlexibleNumeralString {
    type Ops = Self;

    fn is_valid(&self, radix: u32) -> bool {
        self.0.iter().all(|n| (u32::from(*n) < radix))
    }

    fn numeral_count(&self) -> usize {
        self.0.len()
    }

    fn split(&self) -> (Self, Self) {
        let mut front = self.0.clone();
        let back = front.split_off(self.0.len() / 2);
        (FlexibleNumeralString(front), FlexibleNumeralString(back))
    }

    fn concat(mut a: Self, mut b: Self) -> Self {
        a.0.append(&mut b.0);
        a
    }
}

impl Operations for FlexibleNumeralString {
    type Bytes = Vec<u8>;

    fn numeral_count(&self) -> usize {
        self.0.len()
    }

    fn to_be_bytes(&self, radix: u32, b: usize) -> Self::Bytes {
        self.num_radix(radix).to_bytes(b)
    }

    fn add_mod_exp(self, other: impl Iterator<Item = u8>, radix: u32, m: usize) -> Self {
        let other = BigUint::from_bytes(other);
        let c = self.num_radix(radix).add_mod_exp(other, radix, m);
        Self::str_radix(c, radix, m)
    }

    fn sub_mod_exp(self, other: impl Iterator<Item = u8>, radix: u32, m: usize) -> Self {
        let other = BigUint::from_bytes(other);
        let c = self.num_radix(radix).sub_mod_exp(other, radix, m);
        Self::str_radix(c, radix, m)
    }
}

impl FlexibleNumeralString {
    fn num_radix(&self, radix: u32) -> BigUint {
        let mut res = BigUint::zero();
        for i in &self.0 {
            res *= radix;
            res += BigUint::from(*i);
        }
        res
    }

    fn str_radix(mut x: BigUint, radix: u32, m: usize) -> Self {
        let mut res = vec![0; m];
        for i in 0..m {
            res[m - 1 - i] = (&x % radix).to_u16().unwrap();
            x /= radix;
        }
        FlexibleNumeralString(res)
    }
}

/// A numeral string with radix 2.
#[cfg_attr(test, derive(Debug))]
pub struct BinaryNumeralString(Vec<u8>);

impl BinaryNumeralString {
    /// Creates a BinaryNumeralString from a byte slice, with each byte
    /// interpreted in little-endian bit order.
    pub fn from_bytes_le(s: &[u8]) -> Self {
        BinaryNumeralString(s.to_vec())
    }

    /// Returns a Vec<u8>, with each byte written from the BinaryNumeralString
    /// in little-endian bit order.
    pub fn to_bytes_le(&self) -> Vec<u8> {
        self.0.to_vec()
    }
}

impl NumeralString for BinaryNumeralString {
    type Ops = BinaryOps;

    fn is_valid(&self, radix: u32) -> bool {
        // This struct is valid for radix 2 by construction.
        radix == 2
    }

    fn numeral_count(&self) -> usize {
        self.0.len() * 8
    }

    fn split(&self) -> (Self::Ops, Self::Ops) {
        let n = self.numeral_count();
        let u = n / 2;
        let v = n - u;
        let a_end = (u + 7) / 8;
        let b_start = u / 8;

        // FF1 processes the two halves of a numeral string as big-endian integers in the
        // given radix, via the `NUM_radix()` operation. We are operating on binary data
        // with a radix of 2, which means the "bit string" is interpreted as big endian.
        //
        // However, `BinaryNumeralString::from_bytes_le` uses little-endian bit order when
        // parsing a byte encoding into a bit string (which indeed it should, otherwise
        // the byte encoding would be mixed-endian which no one should have to suffer).
        //
        // The strategy taken in `FlexibleNumeralString` (which `BinaryNumeralString`
        // previously also used) is to parse the little-endian byte string into (what is
        // effectively) a `Vec<bool>`, and then read that as a big-endian bit pattern to
        // compute the corresponding `BigUint` arithmetic value. For binary data that is
        // a multiple of 8 bits in length we can do better, but we need to take care about
        // how the data is parsed at each step.
        //
        // Say the input was 5 bytes (for the sake of illustration, so we can show both
        // multiple bytes and how half-bytes / "nibbles" are handled). Let's draw out the
        // bytes, annotated with the least and most significant bytes (LSB, MSB) and bits
        // (lsb, msb), and the numeral string indices for each bit:
        //
        // LSB                                       MSB
        //  | 0..7 | 8..15 | 16..23 | 24..31 | 32..39 |
        // lsb    msb
        //
        // We need to split this into two pieces that have the same numeral string indices
        // but *opposite* endianness interpretation of the numerals (lsn, msn):
        //
        //    msn      lsn
        // a = |  0..19 |
        // b = | 20..39 |
        //
        // We also want to store the bits so we can parse with `BigUint::from_bytes_le`
        // (which avoids an unnecessary allocation per FF1 round). This means that we need
        // the bits to be arranged within the sub-string bytes as follows:
        //
        // LSB                                 MSB
        //  | 19..12 | 11...4 |  3...0 / [0; 4] |
        //  | 39..32 | 31..24 | 23..20 / [0; 4] |
        // lsb      msb
        //
        // If instead we were using a radix of 2^8 = 256, then we would be operating on a
        // "byte string" and the bit ordering of each byte would not matter. Alas.
        let a_subslice = self.0[..a_end].iter();
        let b_subslice = self.0[b_start..].iter();

        let (a, b) = if u % 8 == 0 {
            // Simple case: no shifting necessary, just splitting and reversing.
            assert_eq!(a_end, b_start);

            (
                a_subslice.map(|b| b.reverse_bits()).rev().collect(),
                b_subslice.map(|b| b.reverse_bits()).rev().collect(),
            )
        } else {
            let mut a_processed = a_subslice
                .scan(0, |carried: &mut u8, next: &u8| {
                    // We need to shift `a` "forward" by 4 bits. This will cause the
                    // top nibble to be dropped, which is fine because the subslices
                    // we created from `self.0` overlapped by 1 byte.
                    //
                    // MSB  next       carried
                    //  | ... /  N  |  C  / ... |
                    //        |  N  /  C  | ...
                    let shifted = (next << 4) | (*carried >> 4);
                    *carried = *next;
                    Some(shifted.reverse_bits())
                })
                .collect::<Vec<_>>();

            // Because we call `Iterator::scan` on `a` (which erases knowledge about the
            // iterator's length, as filtering can occur) before reversing it, we can't
            // use `Iterator::rev` (which only works on known-length iterators). Since we
            // know we have prepared the bits correctly within each byte, we perform the
            // byte reversal inside the `Vec` instead.
            a_processed.reverse();

            (
                a_processed,
                b_subslice
                    .map(|b| b.reverse_bits())
                    // Clear (what will become) the most significant nibble.
                    .enumerate()
                    .map(|(i, b)| if i == 0 { b & 0x0f } else { b })
                    .rev()
                    .collect(),
            )
        };

        (BinaryOps::new(a, u), BinaryOps::new(b, v))
    }

    fn concat(a: Self::Ops, b: Self::Ops) -> Self {
        // If you're reading this, you've either already scrolled passed the comment in
        // `Self::split` that explains what we are doing here, or you followed a direct
        // link to this GitHub line. In either case, scroll up if you're confused by what
        // we are doing in this method.
        BinaryNumeralString(if a.num_bits % 8 == 0 {
            // Simple case: no shifting necessary, just reversing and joining.
            b.data
                .into_iter()
                .chain(a.data.into_iter())
                .map(|b| b.reverse_bits())
                .rev()
                .collect()
        } else {
            // We need to shift `a` "backward" by 4 bits. We do this by shifting it
            // "forward" by 4 bits before reversing the bytes.

            // Save the least significant nibble of `a`, which slots into the empty nibble
            // in what is currently the MSB of `b`, and will become the join interface.
            let a_last = (a.data[0] & 0x0f) << 4;

            let a_processed = a
                .data
                .into_iter()
                .scan(0, |carried: &mut u8, next: u8| {
                    // MSB  next       carried
                    //  | ... /  N  | ... /  C  |
                    //        |  N  /  C  | ...
                    let shifted = (next << 4) | *carried;
                    *carried = next >> 4;
                    Some(shifted.reverse_bits())
                })
                // Skip the first byte, containing the nibble we saved above.
                .skip(1);

            let b_processed = b
                .data
                .into_iter()
                // Double-reverse to make the enumeration simpler.
                .rev()
                .enumerate()
                .rev()
                // Slot the saved nibble from `a` into the space in `b`.
                .map(|(i, b)| if i == 0 { a_last | b } else { b })
                .map(|b| b.reverse_bits());

            // Because we call `Iterator::scan` on `a` (which erases knowledge about the
            // iterator's length, as filtering can occur) before reversing it, we can't
            // use `Iterator::rev` (which only works on known-length iterators). Since we
            // know their concatenation is an integer number of bytes, we perform the
            // byte reversal inside the `Vec` instead.
            let mut tmp = b_processed.chain(a_processed).collect::<Vec<_>>();
            tmp.reverse();
            tmp
        })
    }
}

pub struct BinaryOps {
    /// The numeral string sub-section.
    ///
    /// Each byte is bit-big-endian relative to the bit string, so that the individual
    /// bytes have the correct value, but the bytes are stored in little-endian order to
    /// make loading into `BigUint` more efficient.
    data: Vec<u8>,
    num_bits: usize,
}

impl Operations for BinaryOps {
    type Bytes = Vec<u8>;

    fn numeral_count(&self) -> usize {
        self.num_bits
    }

    fn to_be_bytes(&self, radix: u32, b: usize) -> Self::Bytes {
        self.num_radix(radix).to_bytes(b)
    }

    fn add_mod_exp(self, other: impl Iterator<Item = u8>, radix: u32, m: usize) -> Self {
        assert_eq!(self.num_bits, m);
        let other = BigUint::from_bytes(other);
        let c = self.num_radix(radix).add_mod_exp(other, radix, m);
        self.str_radix(c)
    }

    fn sub_mod_exp(self, other: impl Iterator<Item = u8>, radix: u32, m: usize) -> Self {
        assert_eq!(self.num_bits, m);
        let other = BigUint::from_bytes(other);
        let c = self.num_radix(radix).sub_mod_exp(other, radix, m);
        self.str_radix(c)
    }
}

impl BinaryOps {
    fn new(data: Vec<u8>, num_bits: usize) -> Self {
        assert_eq!(data.len(), (num_bits + 7) / 8);
        BinaryOps { data, num_bits }
    }

    fn num_radix(&self, radix: u32) -> BigUint {
        // Check that radix == 2
        assert_eq!(radix, 2);
        BigUint::from_bytes_le(&self.data)
    }

    /// Replace `self` with `STR(x, 2)`.
    fn str_radix(mut self, x: BigUint) -> Self {
        let data = x.to_bytes_le();
        self.data[..data.len()].copy_from_slice(&data);
        self.data[data.len()..].fill(0);
        self
    }
}

#[cfg(test)]
mod tests {
    use aes::{Aes128, Aes192, Aes256};

    use super::{BinaryNumeralString, FlexibleNumeralString};
    use crate::ff1::{
        test_vectors::{self, AesType},
        NumeralString, NumeralStringError, FF1,
    };

    #[test]
    fn ns_is_valid() {
        let radix = 10;
        let ns = FlexibleNumeralString::from(vec![0, 5, 9]);
        assert!(ns.is_valid(radix));

        let ns = FlexibleNumeralString::from(vec![0, 5, 10]);
        assert!(!ns.is_valid(radix));
    }

    #[test]
    fn radix_2_length_limits() {
        let ff = FF1::<Aes128>::new(&[0; 16], 2).unwrap();

        assert_eq!(
            ff.encrypt(&[], &BinaryNumeralString::from_bytes_le(&[]))
                .unwrap_err(),
            NumeralStringError::TooShort {
                ns_len: 0,
                min_len: 20,
            },
        );
        assert_eq!(
            ff.encrypt(&[], &BinaryNumeralString::from_bytes_le(&[0]))
                .unwrap_err(),
            NumeralStringError::TooShort {
                ns_len: 8,
                min_len: 20,
            },
        );
        assert_eq!(
            ff.encrypt(&[], &BinaryNumeralString::from_bytes_le(&[0; 2]))
                .unwrap_err(),
            NumeralStringError::TooShort {
                ns_len: 16,
                min_len: 20,
            },
        );
        assert!(ff
            .encrypt(&[], &BinaryNumeralString::from_bytes_le(&[0; 3]))
            .is_ok());
    }

    #[test]
    fn radix_10_length_limits() {
        let ff = FF1::<Aes128>::new(&[0; 16], 10).unwrap();

        assert_eq!(
            ff.encrypt(&[], &FlexibleNumeralString::from(vec![]))
                .unwrap_err(),
            NumeralStringError::TooShort {
                ns_len: 0,
                min_len: 6,
            },
        );
        assert_eq!(
            ff.encrypt(&[], &FlexibleNumeralString::from(vec![0]))
                .unwrap_err(),
            NumeralStringError::TooShort {
                ns_len: 1,
                min_len: 6,
            },
        );
        assert_eq!(
            ff.encrypt(&[], &FlexibleNumeralString::from(vec![0; 2]))
                .unwrap_err(),
            NumeralStringError::TooShort {
                ns_len: 2,
                min_len: 6,
            },
        );
        assert_eq!(
            ff.encrypt(&[], &FlexibleNumeralString::from(vec![0; 5]))
                .unwrap_err(),
            NumeralStringError::TooShort {
                ns_len: 5,
                min_len: 6,
            },
        );
        assert!(ff
            .encrypt(&[], &FlexibleNumeralString::from(vec![0; 6]))
            .is_ok());
    }

    #[test]
    fn flexible_split_round_trip() {
        for tv in test_vectors::get() {
            {
                let pt = FlexibleNumeralString::from(tv.pt.clone());
                let (a, b) = pt.split();
                assert_eq!(FlexibleNumeralString::concat(a, b).0, tv.pt);
            }

            {
                let ct = FlexibleNumeralString::from(tv.ct.clone());
                let (a, b) = ct.split();
                assert_eq!(FlexibleNumeralString::concat(a, b).0, tv.ct);
            }
        }
    }

    #[test]
    fn flexible() {
        for tv in test_vectors::get() {
            let (ct, pt) = match tv.aes {
                AesType::AES128 => {
                    let ff = FF1::<Aes128>::new(&tv.key, tv.radix).unwrap();
                    (
                        ff.encrypt(&tv.tweak, &FlexibleNumeralString::from(tv.pt.clone())),
                        ff.decrypt(&tv.tweak, &FlexibleNumeralString::from(tv.ct.clone())),
                    )
                }
                AesType::AES192 => {
                    let ff = FF1::<Aes192>::new(&tv.key, tv.radix).unwrap();
                    (
                        ff.encrypt(&tv.tweak, &FlexibleNumeralString::from(tv.pt.clone())),
                        ff.decrypt(&tv.tweak, &FlexibleNumeralString::from(tv.ct.clone())),
                    )
                }
                AesType::AES256 => {
                    let ff = FF1::<Aes256>::new(&tv.key, tv.radix).unwrap();
                    (
                        ff.encrypt(&tv.tweak, &FlexibleNumeralString::from(tv.pt.clone())),
                        ff.decrypt(&tv.tweak, &FlexibleNumeralString::from(tv.ct.clone())),
                    )
                }
            };
            assert_eq!(Vec::from(ct.unwrap()), tv.ct);
            assert_eq!(Vec::from(pt.unwrap()), tv.pt);
        }
    }

    #[test]
    fn binary_split_round_trip() {
        for tv in test_vectors::get().filter(|tv| tv.binary.is_some()) {
            let tvb = tv.binary.unwrap();

            {
                let pt = BinaryNumeralString::from_bytes_le(&tvb.pt);
                let (a, b) = pt.split();
                assert_eq!(BinaryNumeralString::concat(a, b).to_bytes_le(), tvb.pt);
            }

            {
                let ct = BinaryNumeralString::from_bytes_le(&tvb.ct);
                let (a, b) = ct.split();
                assert_eq!(BinaryNumeralString::concat(a, b).to_bytes_le(), tvb.ct);
            }
        }
    }

    #[test]
    fn binary() {
        for tv in test_vectors::get().filter(|tv| tv.binary.is_some()) {
            assert_eq!(tv.aes, AesType::AES256);
            let tvb = tv.binary.unwrap();

            let (bct, bpt) = {
                let ff = FF1::<Aes256>::new(&tv.key, tv.radix).unwrap();
                (
                    ff.encrypt(&tv.tweak, &BinaryNumeralString::from_bytes_le(&tvb.pt))
                        .unwrap(),
                    ff.decrypt(&tv.tweak, &BinaryNumeralString::from_bytes_le(&tvb.ct))
                        .unwrap(),
                )
            };
            assert_eq!(bpt.to_bytes_le(), tvb.pt);
            assert_eq!(bct.to_bytes_le(), tvb.ct);
            assert_eq!(bpt.0, tvb.pt);
            assert_eq!(bct.0, tvb.ct);
        }
    }
}
