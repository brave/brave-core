// -*- mode: rust; -*-
//
// This file is part of reddsa.
// Copyright (c) 2019-2021 Zcash Foundation
// Copyright (c) 2017-2021 isis agora lovecruft, Henry de Valence
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>
// - Henry de Valence <hdevalence@hdevalence.ca>
// - Deirdre Connolly <deirdre@zfnd.org>

//! Traits and types that support variable-time multiscalar multiplication.

use alloc::vec::Vec;
use core::{borrow::Borrow, fmt::Debug};

use jubjub::{ExtendedNielsPoint, ExtendedPoint};

#[cfg(test)]
pub(crate) mod tests;

/// A trait for variable-time multiscalar multiplication without precomputation.
pub trait VartimeMultiscalarMul {
    /// The type of scalar being multiplied, e.g., `jubjub::Scalar`.
    type Scalar;
    /// The type of point being multiplied, e.g., `jubjub::AffinePoint`.
    type Point;

    /// Given an iterator of public scalars and an iterator of
    /// `Option`s of points, compute either `Some(Q)`, where
    /// $$
    /// Q = c\_1 P\_1 + \cdots + c\_n P\_n,
    /// $$
    /// if all points were `Some(P_i)`, or else return `None`.
    fn optional_multiscalar_mul<I, J>(scalars: I, points: J) -> Option<Self::Point>
    where
        I: IntoIterator,
        I::Item: Borrow<Self::Scalar>,
        J: IntoIterator<Item = Option<Self::Point>>;

    /// Given an iterator of public scalars and an iterator of
    /// public points, compute
    /// $$
    /// Q = c\_1 P\_1 + \cdots + c\_n P\_n,
    /// $$
    /// using variable-time operations.
    ///
    /// It is an error to call this function with two iterators of different lengths.
    fn vartime_multiscalar_mul<I, J>(scalars: I, points: J) -> Self::Point
    where
        I: IntoIterator,
        I::Item: Borrow<Self::Scalar>,
        J: IntoIterator,
        J::Item: Borrow<Self::Point>,
        Self::Point: Clone,
    {
        Self::optional_multiscalar_mul(
            scalars,
            points.into_iter().map(|p| Some(p.borrow().clone())),
        )
        .unwrap()
    }
}

/// Produces the non-adjacent form (NAF) of a 32-byte scalar.
pub trait NonAdjacentForm {
    /// Returns the scalar represented as a little-endian byte array.
    fn inner_to_bytes(&self) -> [u8; 32];

    /// Returns the number of coefficients in the NAF.
    ///
    /// Claim: The length of the NAF requires at most one more coefficient than the length of the
    /// binary representation of the scalar. [^1]
    ///
    /// This trait works with scalars of at most 256 binary bits, so the default implementation
    /// returns 257. However, some (sub)groups' orders don't reach 256 bits and their scalars don't
    /// need the full 256 bits. Setting the corresponding NAF length for a particular curve will
    /// speed up the multiscalar multiplication since the number of loop iterations required for the
    /// multiplication is equal to the length of the NAF.
    ///
    /// [^1]: The proof is left as an exercise to the reader.
    fn naf_length() -> usize {
        257
    }

    /// Computes the width-`w` non-adjacent form (width-`w` NAF) of the scalar.
    ///
    /// Thanks to [`curve25519-dalek`].
    ///
    /// [`curve25519-dalek`]: https://github.com/dalek-cryptography/curve25519-dalek/blob/3e189820da03cc034f5fa143fc7b2ccb21fffa5e/src/scalar.rs#L907
    fn non_adjacent_form(&self, w: usize) -> Vec<i8> {
        // required by the NAF definition
        debug_assert!(w >= 2);
        // required so that the NAF digits fit in i8
        debug_assert!(w <= 8);

        use byteorder::{ByteOrder, LittleEndian};

        let naf_length = Self::naf_length();
        let mut naf = vec![0; naf_length];

        let mut x_u64 = [0u64; 5];
        LittleEndian::read_u64_into(&self.inner_to_bytes(), &mut x_u64[0..4]);

        let width = 1 << w;
        let window_mask = width - 1;

        let mut pos = 0;
        let mut carry = 0;

        while pos < naf_length {
            // Construct a buffer of bits of the scalar, starting at bit `pos`
            let u64_idx = pos / 64;
            let bit_idx = pos % 64;
            let bit_buf: u64 = if bit_idx < 64 - w {
                // This window's bits are contained in a single u64
                x_u64[u64_idx] >> bit_idx
            } else {
                // Combine the current u64's bits with the bits from the next u64
                (x_u64[u64_idx] >> bit_idx) | (x_u64[1 + u64_idx] << (64 - bit_idx))
            };

            // Add the carry into the current window
            let window = carry + (bit_buf & window_mask);

            if window & 1 == 0 {
                // If the window value is even, preserve the carry and continue.
                // Why is the carry preserved?
                // If carry == 0 and window & 1 == 0, then the next carry should be 0
                // If carry == 1 and window & 1 == 0, then bit_buf & 1 == 1 so the next carry should be 1
                pos += 1;
                continue;
            }

            if window < width / 2 {
                carry = 0;
                naf[pos] = window as i8;
            } else {
                carry = 1;
                naf[pos] = (window as i8).wrapping_sub(width as i8);
            }

            pos += w;
        }

        naf
    }
}

impl NonAdjacentForm for jubjub::Scalar {
    fn inner_to_bytes(&self) -> [u8; 32] {
        self.to_bytes()
    }

    /// The NAF length for Jubjub is 253 since Jubjub's order is about 2<sup>251.85</sup>.
    fn naf_length() -> usize {
        253
    }
}

/// Holds odd multiples 1A, 3A, ..., 15A of a point A.
#[derive(Copy, Clone)]
pub(crate) struct LookupTable5<T>(pub(crate) [T; 8]);

impl<T: Copy> LookupTable5<T> {
    /// Given public, odd \\( x \\) with \\( 0 < x < 2^4 \\), return \\(xA\\).
    pub fn select(&self, x: usize) -> T {
        debug_assert_eq!(x & 1, 1);
        debug_assert!(x < 16);

        self.0[x / 2]
    }
}

impl<T: Debug> Debug for LookupTable5<T> {
    fn fmt(&self, f: &mut ::core::fmt::Formatter) -> ::core::fmt::Result {
        write!(f, "LookupTable5({:?})", self.0)
    }
}

impl<'a> From<&'a ExtendedPoint> for LookupTable5<ExtendedNielsPoint> {
    #[allow(non_snake_case)]
    fn from(A: &'a ExtendedPoint) -> Self {
        let mut Ai = [A.to_niels(); 8];
        let A2 = A.double();
        for i in 0..7 {
            Ai[i + 1] = (A2 + Ai[i]).to_niels();
        }
        // Now Ai = [A, 3A, 5A, 7A, 9A, 11A, 13A, 15A]
        LookupTable5(Ai)
    }
}

impl VartimeMultiscalarMul for ExtendedPoint {
    type Scalar = jubjub::Scalar;
    type Point = ExtendedPoint;

    /// Variable-time multiscalar multiplication using a non-adjacent form of
    /// width (5).
    ///
    /// The non-adjacent form has signed, odd digits.  Using only odd digits
    /// halves the table size (since we only need odd multiples), or gives fewer
    /// additions for the same table size.
    ///
    /// As the name implies, the runtime varies according to the values of the
    /// inputs, thus is not safe for computing over secret data, but is great
    /// for computing over public data, such as validating signatures.
    #[allow(non_snake_case)]
    fn optional_multiscalar_mul<I, J>(scalars: I, points: J) -> Option<ExtendedPoint>
    where
        I: IntoIterator,
        I::Item: Borrow<Self::Scalar>,
        J: IntoIterator<Item = Option<ExtendedPoint>>,
    {
        let nafs: Vec<_> = scalars
            .into_iter()
            .map(|c| c.borrow().non_adjacent_form(5))
            .collect();

        let lookup_tables = points
            .into_iter()
            .map(|P_opt| P_opt.map(|P| LookupTable5::<ExtendedNielsPoint>::from(&P)))
            .collect::<Option<Vec<_>>>()?;

        let mut r = ExtendedPoint::identity();
        let naf_size = Self::Scalar::naf_length();

        for i in (0..naf_size).rev() {
            let mut t = r.double();

            for (naf, lookup_table) in nafs.iter().zip(lookup_tables.iter()) {
                #[allow(clippy::comparison_chain)]
                if naf[i] > 0 {
                    t += lookup_table.select(naf[i] as usize);
                } else if naf[i] < 0 {
                    t -= lookup_table.select(-naf[i] as usize);
                }
            }

            r = t;
        }

        Some(r)
    }
}
