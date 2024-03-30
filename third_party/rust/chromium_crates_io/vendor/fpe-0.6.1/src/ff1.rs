//! A Rust implementation of the FF1 algorithm, specified in
//! [NIST Special Publication 800-38G](http://dx.doi.org/10.6028/NIST.SP.800-38G).

use core::cmp;

use cipher::{
    generic_array::GenericArray, Block, BlockCipher, BlockEncrypt, BlockEncryptMut, InnerIvInit,
    KeyInit,
};

#[cfg(test)]
use static_assertions::const_assert;

mod error;
pub use error::{InvalidRadix, NumeralStringError};

#[cfg(feature = "alloc")]
mod alloc;
#[cfg(feature = "alloc")]
pub use self::alloc::{BinaryNumeralString, FlexibleNumeralString};

#[cfg(test)]
mod proptests;

#[cfg(test)]
mod test_vectors;

/// The minimum allowed numeral string length for any radix.
const MIN_NS_LEN: u32 = 2;
/// The maximum allowed numeral string length for any radix.
const MAX_NS_LEN: usize = u32::MAX as usize;

/// The minimum allowed value of radix^minlen.
///
/// Defined in [NIST SP 800-38G Revision 1](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-38Gr1-draft.pdf).
const MIN_NS_DOMAIN_SIZE: u64 = 1_000_000;

/// `minlen` such that `2^minlen >= MIN_NS_DOMAIN_SIZE`.
const MIN_RADIX_2_NS_LEN: u32 = 20;

#[cfg(test)]
const_assert!((1 << MIN_RADIX_2_NS_LEN) >= MIN_NS_DOMAIN_SIZE);

#[derive(Debug, PartialEq)]
enum Radix {
    /// A radix in [2..2^16]. It uses floating-point arithmetic.
    Any { radix: u32, min_len: u32 },
    /// A radix 2^i for i in [1..16]. It does not use floating-point arithmetic.
    PowerTwo {
        radix: u32,
        min_len: u32,
        log_radix: u8,
    },
}

impl Radix {
    fn from_u32(radix: u32) -> Result<Self, InvalidRadix> {
        // radix must be in range [2..=2^16]
        if !(2..=(1 << 16)).contains(&radix) {
            return Err(InvalidRadix(radix));
        }

        Ok(if radix.count_ones() == 1 {
            let log_radix = 31 - radix.leading_zeros();
            Radix::PowerTwo {
                radix,
                min_len: cmp::max((MIN_RADIX_2_NS_LEN + log_radix - 1) / log_radix, MIN_NS_LEN),
                log_radix: u8::try_from(log_radix).unwrap(),
            }
        } else {
            // If radix is exactly 2^16 then the code below would overflow a u32.
            let radix_u64 = u64::from(radix);
            let mut min_len = 1u32;
            let mut domain = radix_u64;
            while domain < MIN_NS_DOMAIN_SIZE {
                domain *= radix_u64;
                min_len += 1;
            }
            Radix::Any { radix, min_len }
        })
    }

    fn check_ns_length(&self, ns_len: usize) -> Result<(), NumeralStringError> {
        let min_len = match *self {
            Radix::Any { min_len, .. } => min_len as usize,
            Radix::PowerTwo { min_len, .. } => min_len as usize,
        };
        let max_len = MAX_NS_LEN;

        if ns_len < min_len {
            Err(NumeralStringError::TooShort { ns_len, min_len })
        } else if ns_len > max_len {
            Err(NumeralStringError::TooLong { ns_len, max_len })
        } else {
            Ok(())
        }
    }

    /// Calculates b = ceil(ceil(v * log2(radix)) / 8).
    fn calculate_b(&self, v: usize) -> usize {
        use libm::{ceil, log2};
        match *self {
            Radix::Any { radix, .. } => ceil(v as f64 * log2(f64::from(radix)) / 8f64) as usize,
            Radix::PowerTwo { log_radix, .. } => ((v * log_radix as usize) + 7) / 8,
        }
    }

    fn to_u32(&self) -> u32 {
        match *self {
            Radix::Any { radix, .. } => radix,
            Radix::PowerTwo { radix, .. } => radix,
        }
    }
}

/// Type representing FF1 operations that can be performed on a sub-section of a
/// [`NumeralString`].
pub trait Operations: Sized {
    /// Type used for byte representations.
    type Bytes: AsRef<[u8]>;

    /// Returns the number of numerals in this numeral sub-string.
    fn numeral_count(&self) -> usize;

    /// Returns a `b`-byte big-endian representation of the number that is
    /// encoded in base `radix` by this numeral string. The numerals are
    /// valued in decreasing order of significance (big-endian order).
    /// This corresponds to $STR^{b}_{256}(NUM_{radix}(X))$ in the NIST spec.
    fn to_be_bytes(&self, radix: u32, b: usize) -> Self::Bytes;

    /// Computes `(self + other) mod radix^m`.
    fn add_mod_exp(self, other: impl Iterator<Item = u8>, radix: u32, m: usize) -> Self;

    /// Computes `(self - other) mod radix^m`.
    fn sub_mod_exp(self, other: impl Iterator<Item = u8>, radix: u32, m: usize) -> Self;
}

/// For a given base, a finite, ordered sequence of numerals for the base.
pub trait NumeralString: Sized {
    /// Type used for FF1 computations.
    ///
    /// This may be `Self`, or it may be a type that can operate more efficiently.
    type Ops: Operations;

    /// Returns whether this numeral string is valid for the base radix.
    fn is_valid(&self, radix: u32) -> bool;

    /// Returns the number of numerals in this numeral string.
    fn numeral_count(&self) -> usize;

    /// Splits this numeral string of length `n` into two sections of lengths
    /// `u = floor(n / 2)` and `v = n - u` that can be used for FF1 computations.
    fn split(&self) -> (Self::Ops, Self::Ops);

    /// Concatenates two strings used for FF1 computations into a single numeral string.
    fn concat(a: Self::Ops, b: Self::Ops) -> Self;
}

#[derive(Clone)]
struct Prf<CIPH: BlockCipher + BlockEncrypt> {
    state: cbc::Encryptor<CIPH>,
    // Contains the output when offset = 0, and partial input otherwise
    buf: [Block<CIPH>; 1],
    offset: usize,
}

impl<CIPH: BlockCipher + BlockEncrypt + Clone> Prf<CIPH> {
    fn new(ciph: &CIPH) -> Self {
        let ciph = ciph.clone();
        Prf {
            state: cbc::Encryptor::inner_iv_init(ciph, GenericArray::from_slice(&[0; 16])),
            buf: [Block::<CIPH>::default()],
            offset: 0,
        }
    }

    fn update(&mut self, mut data: &[u8]) {
        while !data.is_empty() {
            let to_read = cmp::min(self.buf[0].len() - self.offset, data.len());
            self.buf[0][self.offset..self.offset + to_read].copy_from_slice(&data[..to_read]);
            self.offset += to_read;
            data = &data[to_read..];

            if self.offset == self.buf[0].len() {
                self.state.encrypt_blocks_mut(&mut self.buf);
                self.offset = 0;
            }
        }
    }

    /// Returns the current PRF output.
    ///
    /// The caller MUST ensure that the PRF has processed an integer number of blocks.
    fn output(&self) -> &Block<CIPH> {
        assert_eq!(self.offset, 0);
        &self.buf[0]
    }
}

fn generate_s<'a, CIPH: BlockEncrypt>(
    ciph: &'a CIPH,
    r: &'a Block<CIPH>,
    d: usize,
) -> impl Iterator<Item = u8> + 'a {
    r.clone()
        .into_iter()
        .chain((1..((d + 15) / 16) as u128).flat_map(move |j| {
            let mut block = r.clone();
            for (b, j) in block.iter_mut().zip(j.to_be_bytes().iter()) {
                *b ^= j;
            }
            ciph.encrypt_block(&mut block);
            block.into_iter()
        }))
        .take(d)
}

/// A struct for performing FF1 encryption and decryption operations.
pub struct FF1<CIPH: BlockCipher> {
    ciph: CIPH,
    radix: Radix,
}

impl<CIPH: BlockCipher + KeyInit> FF1<CIPH> {
    /// Creates a new FF1 object for the given key and radix.
    ///
    /// Returns an error if the given radix is not in [2..2^16].
    pub fn new(key: &[u8], radix: u32) -> Result<Self, InvalidRadix> {
        let ciph = CIPH::new(GenericArray::from_slice(key));
        let radix = Radix::from_u32(radix)?;
        Ok(FF1 { ciph, radix })
    }
}

impl<CIPH: BlockCipher + BlockEncrypt + Clone> FF1<CIPH> {
    /// Encrypts the given numeral string.
    ///
    /// Returns an error if the numeral string is not in the required radix.
    #[allow(clippy::many_single_char_names)]
    pub fn encrypt<NS: NumeralString>(
        &self,
        tweak: &[u8],
        x: &NS,
    ) -> Result<NS, NumeralStringError> {
        if !x.is_valid(self.radix.to_u32()) {
            return Err(NumeralStringError::InvalidForRadix(self.radix.to_u32()));
        }
        self.radix.check_ns_length(x.numeral_count())?;

        let n = x.numeral_count();
        let t = tweak.len();

        // 1. Let u = floor(n / 2); v = n - u
        // 2. Let A = X[1..u]; B = X[u + 1..n].
        let (mut x_a, mut x_b) = x.split();
        let u = x_a.numeral_count();
        let v = x_b.numeral_count();

        // 3. Let b = ceil(ceil(v * log2(radix)) / 8).
        let b = self.radix.calculate_b(v);

        // 4. Let d = 4 * ceil(b / 4) + 4.
        let d = 4 * ((b + 3) / 4) + 4;

        // 5. Let P = [1, 2, 1] || [radix] || [10] || [u mod 256] || [n] || [t].
        let mut p = [1, 2, 1, 0, 0, 0, 10, u as u8, 0, 0, 0, 0, 0, 0, 0, 0];
        p[3..6].copy_from_slice(&self.radix.to_u32().to_be_bytes()[1..]);
        p[8..12].copy_from_slice(&(n as u32).to_be_bytes());
        p[12..16].copy_from_slice(&(t as u32).to_be_bytes());

        //  6i. Let Q = T || [0]^((-t-b-1) mod 16) || [i] || [NUM(B, radix)].
        // 6ii. Let R = PRF(P || Q).
        let mut prf = Prf::new(&self.ciph);
        prf.update(&p);
        prf.update(tweak);
        for _ in 0..((((-(t as i32) - (b as i32) - 1) % 16) + 16) % 16) {
            prf.update(&[0]);
        }
        for i in 0..10 {
            let mut prf = prf.clone();
            prf.update(&[i]);
            prf.update(x_b.to_be_bytes(self.radix.to_u32(), b).as_ref());
            let r = prf.output();

            // 6iii. Let S be the first d bytes of R.
            let s = generate_s(&self.ciph, r, d);

            // 6iv. Let y = NUM(S).
            // 6v. If i is even, let m = u; else, let m = v.
            // 6vi. Let c = (NUM(A, radix) + y) mod radix^m.
            // 6vii. Let C = STR(c, radix).
            let m = if i % 2 == 0 { u } else { v };
            let x_c = x_a.add_mod_exp(s, self.radix.to_u32(), m);

            // 6viii. Let A = B.
            x_a = x_b;

            // 6ix. Let B = C.
            x_b = x_c;
        }

        // 7. Return A || B.
        Ok(NS::concat(x_a, x_b))
    }

    /// Decrypts the given numeral string.
    ///
    /// Returns an error if the numeral string is not in the required radix.
    #[allow(clippy::many_single_char_names)]
    pub fn decrypt<NS: NumeralString>(
        &self,
        tweak: &[u8],
        x: &NS,
    ) -> Result<NS, NumeralStringError> {
        if !x.is_valid(self.radix.to_u32()) {
            return Err(NumeralStringError::InvalidForRadix(self.radix.to_u32()));
        }
        self.radix.check_ns_length(x.numeral_count())?;

        let n = x.numeral_count();
        let t = tweak.len();

        // 1. Let u = floor(n / 2); v = n - u
        // 2. Let A = X[1..u]; B = X[u + 1..n].
        let (mut x_a, mut x_b) = x.split();
        let u = x_a.numeral_count();
        let v = x_b.numeral_count();

        // 3. Let b = ceil(ceil(v * log2(radix)) / 8).
        let b = self.radix.calculate_b(v);

        // 4. Let d = 4 * ceil(b / 4) + 4.
        let d = 4 * ((b + 3) / 4) + 4;

        // 5. Let P = [1, 2, 1] || [radix] || [10] || [u mod 256] || [n] || [t].
        let mut p = [1, 2, 1, 0, 0, 0, 10, u as u8, 0, 0, 0, 0, 0, 0, 0, 0];
        p[3..6].copy_from_slice(&self.radix.to_u32().to_be_bytes()[1..]);
        p[8..12].copy_from_slice(&(n as u32).to_be_bytes());
        p[12..16].copy_from_slice(&(t as u32).to_be_bytes());

        //  6i. Let Q = T || [0]^((-t-b-1) mod 16) || [i] || [NUM(A, radix)].
        // 6ii. Let R = PRF(P || Q).
        let mut prf = Prf::new(&self.ciph);
        prf.update(&p);
        prf.update(tweak);
        for _ in 0..((((-(t as i32) - (b as i32) - 1) % 16) + 16) % 16) {
            prf.update(&[0]);
        }
        for i in 0..10 {
            let i = 9 - i;
            let mut prf = prf.clone();
            prf.update(&[i]);
            prf.update(x_a.to_be_bytes(self.radix.to_u32(), b).as_ref());
            let r = prf.output();

            // 6iii. Let S be the first d bytes of R.
            let s = generate_s(&self.ciph, r, d);

            // 6iv. Let y = NUM(S).
            // 6v. If i is even, let m = u; else, let m = v.
            // 6vi. Let c = (NUM(B, radix) - y) mod radix^m.
            // 6vii. Let C = STR(c, radix).
            let m = if i % 2 == 0 { u } else { v };
            let x_c = x_b.sub_mod_exp(s, self.radix.to_u32(), m);

            // 6viii. Let B = A.
            x_b = x_a;

            // 6ix. Let A = C.
            x_a = x_c;
        }

        // 7. Return A || B.
        Ok(NS::concat(x_a, x_b))
    }
}

#[cfg(test)]
mod tests {
    use super::{InvalidRadix, Radix, MIN_NS_LEN, MIN_RADIX_2_NS_LEN};

    #[test]
    fn radix() {
        assert_eq!(Radix::from_u32(1), Err(InvalidRadix(1)));
        assert_eq!(
            Radix::from_u32(2),
            Ok(Radix::PowerTwo {
                radix: 2,
                min_len: MIN_RADIX_2_NS_LEN,
                log_radix: 1,
            })
        );
        assert_eq!(
            Radix::from_u32(3),
            Ok(Radix::Any {
                radix: 3,
                min_len: 13,
            })
        );
        assert_eq!(
            Radix::from_u32(4),
            Ok(Radix::PowerTwo {
                radix: 4,
                min_len: MIN_RADIX_2_NS_LEN / 2,
                log_radix: 2,
            })
        );
        assert_eq!(
            Radix::from_u32(5),
            Ok(Radix::Any {
                radix: 5,
                min_len: 9,
            })
        );
        assert_eq!(
            Radix::from_u32(6),
            Ok(Radix::Any {
                radix: 6,
                min_len: 8,
            })
        );
        assert_eq!(
            Radix::from_u32(7),
            Ok(Radix::Any {
                radix: 7,
                min_len: 8,
            })
        );
        assert_eq!(
            Radix::from_u32(8),
            Ok(Radix::PowerTwo {
                radix: 8,
                min_len: 7,
                log_radix: 3,
            })
        );
        assert_eq!(
            Radix::from_u32(10),
            Ok(Radix::Any {
                radix: 10,
                min_len: 6,
            })
        );
        assert_eq!(
            Radix::from_u32(32768),
            Ok(Radix::PowerTwo {
                radix: 32768,
                min_len: MIN_NS_LEN,
                log_radix: 15,
            })
        );
        assert_eq!(
            Radix::from_u32(65535),
            Ok(Radix::Any {
                radix: 65535,
                min_len: MIN_NS_LEN,
            })
        );
        assert_eq!(
            Radix::from_u32(65536),
            Ok(Radix::PowerTwo {
                radix: 65536,
                min_len: MIN_NS_LEN,
                log_radix: 16,
            })
        );
        assert_eq!(Radix::from_u32(65537), Err(InvalidRadix(65537)));
    }
}
