//! This module contains the `Field` abstraction that allows us to write
//! code that generalizes over a pair of fields.

use core::mem::size_of;

use static_assertions::const_assert;

#[cfg(feature = "sqrt-table")]
use alloc::{boxed::Box, vec::Vec};
#[cfg(feature = "sqrt-table")]
use core::marker::PhantomData;

#[cfg(feature = "sqrt-table")]
use subtle::Choice;

const_assert!(size_of::<usize>() >= 4);

/// An internal trait that exposes additional operations related to calculating square roots of
/// prime-order finite fields.
pub(crate) trait SqrtTableHelpers: ff::PrimeField {
    /// Raise this field element to the power $(t-1)/2$.
    ///
    /// Field implementations may override this to use an efficient addition chain.
    fn pow_by_t_minus1_over2(&self) -> Self;

    /// Gets the lower 32 bits of this field element when expressed
    /// canonically.
    fn get_lower_32(&self) -> u32;
}

/// Parameters for a perfect hash function used in square root computation.
#[cfg(feature = "sqrt-table")]
#[cfg_attr(docsrs, doc(cfg(feature = "sqrt-table")))]
#[derive(Debug)]
struct SqrtHasher<F: SqrtTableHelpers> {
    hash_xor: u32,
    hash_mod: usize,
    marker: PhantomData<F>,
}

#[cfg(feature = "sqrt-table")]
impl<F: SqrtTableHelpers> SqrtHasher<F> {
    /// Returns a perfect hash of x for use with SqrtTables::inv.
    fn hash(&self, x: &F) -> usize {
        // This is just the simplest constant-time perfect hash construction that could
        // possibly work. The 32 low-order bits are unique within the 2^S order subgroup,
        // then the xor acts as "salt" to injectively randomize the output when taken modulo
        // `hash_mod`. Since the table is small, we do not need anything more complicated.
        ((x.get_lower_32() ^ self.hash_xor) as usize) % self.hash_mod
    }
}

/// Tables used for square root computation.
#[cfg(feature = "sqrt-table")]
#[cfg_attr(docsrs, doc(cfg(feature = "sqrt-table")))]
#[derive(Debug)]
pub(crate) struct SqrtTables<F: SqrtTableHelpers> {
    hasher: SqrtHasher<F>,
    inv: Vec<u8>,
    g0: Box<[F; 256]>,
    g1: Box<[F; 256]>,
    g2: Box<[F; 256]>,
    g3: Box<[F; 129]>,
}

#[cfg(feature = "sqrt-table")]
impl<F: SqrtTableHelpers> SqrtTables<F> {
    /// Build tables given parameters for the perfect hash.
    pub fn new(hash_xor: u32, hash_mod: usize) -> Self {
        use alloc::vec;

        let hasher = SqrtHasher {
            hash_xor,
            hash_mod,
            marker: PhantomData,
        };

        let mut gtab = (0..4).scan(F::ROOT_OF_UNITY, |gi, _| {
            // gi == ROOT_OF_UNITY^(256^i)
            let gtab_i: Vec<F> = (0..256)
                .scan(F::ONE, |acc, _| {
                    let res = *acc;
                    *acc *= *gi;
                    Some(res)
                })
                .collect();
            *gi = gtab_i[255] * *gi;
            Some(gtab_i)
        });
        let gtab_0 = gtab.next().unwrap();
        let gtab_1 = gtab.next().unwrap();
        let gtab_2 = gtab.next().unwrap();
        let mut gtab_3 = gtab.next().unwrap();
        assert_eq!(gtab.next(), None);

        // Now invert gtab[3].
        let mut inv: Vec<u8> = vec![1; hash_mod];
        for (j, gtab_3_j) in gtab_3.iter().enumerate() {
            let hash = hasher.hash(gtab_3_j);
            // 1 is the last value to be assigned, so this ensures there are no collisions.
            assert!(inv[hash] == 1);
            inv[hash] = ((256 - j) & 0xFF) as u8;
        }

        gtab_3.truncate(129);

        SqrtTables::<F> {
            hasher,
            inv,
            g0: gtab_0.into_boxed_slice().try_into().unwrap(),
            g1: gtab_1.into_boxed_slice().try_into().unwrap(),
            g2: gtab_2.into_boxed_slice().try_into().unwrap(),
            g3: gtab_3.into_boxed_slice().try_into().unwrap(),
        }
    }

    /// Computes:
    ///
    /// * (true,  sqrt(num/div)),                 if num and div are nonzero and num/div is a square in the field;
    /// * (true,  0),                             if num is zero;
    /// * (false, 0),                             if num is nonzero and div is zero;
    /// * (false, sqrt(ROOT_OF_UNITY * num/div)), if num and div are nonzero and num/div is a nonsquare in the field;
    ///
    /// where ROOT_OF_UNITY is a generator of the order 2^n subgroup (and therefore a nonsquare).
    ///
    /// The choice of root from sqrt is unspecified.
    pub fn sqrt_ratio(&self, num: &F, div: &F) -> (Choice, F) {
        // Based on:
        // * [Sarkar2020](https://eprint.iacr.org/2020/1407)
        // * [BDLSY2012](https://cr.yp.to/papers.html#ed25519)
        //
        // We need to calculate uv and v, where v = u^((T-1)/2), u = num/div, and p-1 = T * 2^S.
        // We can rewrite as follows:
        //
        //      v = (num/div)^((T-1)/2)
        //        = num^((T-1)/2) * div^(p-1 - (T-1)/2)    [Fermat's Little Theorem]
        //        =       "       * div^(T * 2^S - (T-1)/2)
        //        =       "       * div^((2^(S+1) - 1)*(T-1)/2 + 2^S)
        //        = (num * div^(2^(S+1) - 1))^((T-1)/2) * div^(2^S)
        //
        // Let  w = (num * div^(2^(S+1) - 1))^((T-1)/2) * div^(2^S - 1).
        // Then v = w * div, and uv = num * v / div = num * w.
        //
        // We calculate:
        //
        //      s = div^(2^S - 1) using an addition chain
        //      t = div^(2^(S+1) - 1) = s^2 * div
        //      w = (num * t)^((T-1)/2) * s using another addition chain
        //
        // then u and uv as above. The addition chains are given in
        // https://github.com/zcash/pasta/blob/master/addchain_sqrt.py .
        // The overall cost of this part is similar to a single full-width exponentiation,
        // regardless of S.

        let sqr = |x: F, i: u32| (0..i).fold(x, |x, _| x.square());

        // s = div^(2^S - 1)
        let s = (0..5).fold(*div, |d: F, i| sqr(d, 1 << i) * d);

        // t == div^(2^(S+1) - 1)
        let t = s.square() * div;

        // w = (num * t)^((T-1)/2) * s
        let w = (t * num).pow_by_t_minus1_over2() * s;

        // v == u^((T-1)/2)
        let v = w * div;

        // uv = u * v
        let uv = w * num;

        let res = self.sqrt_common(&uv, &v);

        let sqdiv = res.square() * div;
        let is_square = (sqdiv - num).is_zero();
        let is_nonsquare = (sqdiv - F::ROOT_OF_UNITY * num).is_zero();
        assert!(bool::from(
            num.is_zero() | div.is_zero() | (is_square ^ is_nonsquare)
        ));

        (is_square, res)
    }

    /// Same as sqrt_ratio(u, one()) but more efficient.
    pub fn sqrt_alt(&self, u: &F) -> (Choice, F) {
        let v = u.pow_by_t_minus1_over2();
        let uv = *u * v;

        let res = self.sqrt_common(&uv, &v);

        let sq = res.square();
        let is_square = (sq - u).is_zero();
        let is_nonsquare = (sq - F::ROOT_OF_UNITY * u).is_zero();
        assert!(bool::from(u.is_zero() | (is_square ^ is_nonsquare)));

        (is_square, res)
    }

    /// Common part of sqrt_ratio and sqrt_alt: return their result given v = u^((T-1)/2) and uv = u * v.
    fn sqrt_common(&self, uv: &F, v: &F) -> F {
        let sqr = |x: F, i: u32| (0..i).fold(x, |x, _| x.square());
        let inv = |x: F| self.inv[self.hasher.hash(&x)] as usize;

        let x3 = *uv * v;
        let x2 = sqr(x3, 8);
        let x1 = sqr(x2, 8);
        let x0 = sqr(x1, 8);

        // i = 0, 1
        let mut t_ = inv(x0); // = t >> 16
                              // 1 == x0 * ROOT_OF_UNITY^(t_ << 24)
        assert!(t_ < 0x100);
        let alpha = x1 * self.g2[t_];

        // i = 2
        t_ += inv(alpha) << 8; // = t >> 8
                               // 1 == x1 * ROOT_OF_UNITY^(t_ << 16)
        assert!(t_ < 0x10000);
        let alpha = x2 * self.g1[t_ & 0xFF] * self.g2[t_ >> 8];

        // i = 3
        t_ += inv(alpha) << 16; // = t
                                // 1 == x2 * ROOT_OF_UNITY^(t_ << 8)
        assert!(t_ < 0x1000000);
        let alpha = x3 * self.g0[t_ & 0xFF] * self.g1[(t_ >> 8) & 0xFF] * self.g2[t_ >> 16];

        t_ += inv(alpha) << 24; // = t << 1
                                // 1 == x3 * ROOT_OF_UNITY^t_
        t_ = (((t_ as u64) + 1) >> 1) as usize;
        assert!(t_ <= 0x80000000);

        *uv * self.g0[t_ & 0xFF]
            * self.g1[(t_ >> 8) & 0xFF]
            * self.g2[(t_ >> 16) & 0xFF]
            * self.g3[t_ >> 24]
    }
}

/// Compute a + b + carry, returning the result and the new carry over.
#[inline(always)]
pub(crate) const fn adc(a: u64, b: u64, carry: u64) -> (u64, u64) {
    let ret = (a as u128) + (b as u128) + (carry as u128);
    (ret as u64, (ret >> 64) as u64)
}

/// Compute a - (b + borrow), returning the result and the new borrow.
#[inline(always)]
pub(crate) const fn sbb(a: u64, b: u64, borrow: u64) -> (u64, u64) {
    let ret = (a as u128).wrapping_sub((b as u128) + ((borrow >> 63) as u128));
    (ret as u64, (ret >> 64) as u64)
}

/// Compute a + (b * c) + carry, returning the result and the new carry over.
#[inline(always)]
pub(crate) const fn mac(a: u64, b: u64, c: u64, carry: u64) -> (u64, u64) {
    let ret = (a as u128) + ((b as u128) * (c as u128)) + (carry as u128);
    (ret as u64, (ret >> 64) as u64)
}
