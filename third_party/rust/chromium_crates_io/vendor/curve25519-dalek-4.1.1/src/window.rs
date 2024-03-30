// -*- mode: rust; -*-
//
// This file is part of curve25519-dalek.
// Copyright (c) 2016-2021 isis lovecruft
// Copyright (c) 2016-2019 Henry de Valence
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>
// - Henry de Valence <hdevalence@hdevalence.ca>

//! Code for fixed- and sliding-window functionality

#![allow(non_snake_case)]

use core::fmt::Debug;

use cfg_if::cfg_if;

use subtle::Choice;
use subtle::ConditionallyNegatable;
use subtle::ConditionallySelectable;
use subtle::ConstantTimeEq;

use crate::traits::Identity;

use crate::backend::serial::curve_models::AffineNielsPoint;
use crate::backend::serial::curve_models::ProjectiveNielsPoint;
use crate::edwards::EdwardsPoint;

#[cfg(feature = "zeroize")]
use zeroize::Zeroize;

macro_rules! impl_lookup_table {
    (Name = $name:ident, Size = $size:expr, SizeNeg = $neg:expr, SizeRange = $range:expr, ConversionRange = $conv_range:expr) => {
        /// A lookup table of precomputed multiples of a point \\(P\\), used to
        /// compute \\( xP \\) for \\( -8 \leq x \leq 8 \\).
        ///
        /// The computation of \\( xP \\) is done in constant time by the `select` function.
        ///
        /// Since `LookupTable` does not implement `Index`, it's more difficult
        /// to accidentally use the table directly.  Unfortunately the table is
        /// only `pub(crate)` so that we can write hardcoded constants, so it's
        /// still technically possible.  It would be nice to prevent direct
        /// access to the table.
        #[derive(Copy, Clone)]
        pub struct $name<T>(pub(crate) [T; $size]);

        impl<T> $name<T>
        where
            T: Identity + ConditionallySelectable + ConditionallyNegatable,
        {
            /// Given \\(-8 \leq x \leq 8\\), return \\(xP\\) in constant time.
            pub fn select(&self, x: i8) -> T {
                debug_assert!(x >= $neg);
                debug_assert!(x as i16 <= $size as i16); // XXX We have to convert to i16s here for the radix-256 case.. this is wrong.

                // Compute xabs = |x|
                let xmask = x as i16 >> 7;
                let xabs = (x as i16 + xmask) ^ xmask;

                // Set t = 0 * P = identity
                let mut t = T::identity();
                for j in $range {
                    // Copy `points[j-1] == j*P` onto `t` in constant time if `|x| == j`.
                    let c = (xabs as u16).ct_eq(&(j as u16));
                    t.conditional_assign(&self.0[j - 1], c);
                }
                // Now t == |x| * P.

                let neg_mask = Choice::from((xmask & 1) as u8);
                t.conditional_negate(neg_mask);
                // Now t == x * P.

                t
            }
        }

        impl<T: Copy + Default> Default for $name<T> {
            fn default() -> $name<T> {
                $name([T::default(); $size])
            }
        }

        impl<T: Debug> Debug for $name<T> {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                write!(f, "{:?}(", stringify!($name))?;

                for x in self.0.iter() {
                    write!(f, "{:?}", x)?;
                }

                write!(f, ")")
            }
        }

        impl<'a> From<&'a EdwardsPoint> for $name<ProjectiveNielsPoint> {
            fn from(P: &'a EdwardsPoint) -> Self {
                let mut points = [P.as_projective_niels(); $size];
                for j in $conv_range {
                    points[j + 1] = (P + &points[j]).as_extended().as_projective_niels();
                }
                $name(points)
            }
        }

        impl<'a> From<&'a EdwardsPoint> for $name<AffineNielsPoint> {
            fn from(P: &'a EdwardsPoint) -> Self {
                let mut points = [P.as_affine_niels(); $size];
                // XXX batch inversion would be good if perf mattered here
                for j in $conv_range {
                    points[j + 1] = (P + &points[j]).as_extended().as_affine_niels()
                }
                $name(points)
            }
        }

        #[cfg(feature = "zeroize")]
        impl<T> Zeroize for $name<T>
        where
            T: Copy + Default + Zeroize,
        {
            fn zeroize(&mut self) {
                self.0.iter_mut().zeroize();
            }
        }
    };
} // End macro_rules! impl_lookup_table

// The first one has to be named "LookupTable" because it's used as a constructor for consts.
// This is radix-16
impl_lookup_table! {
    Name = LookupTable,
    Size = 8,
    SizeNeg = -8,
    SizeRange = 1..9,
    ConversionRange = 0..7
}

// The rest only get used to make basepoint tables
cfg_if! {
    if #[cfg(feature = "precomputed-tables")] {
        // radix-32
        impl_lookup_table! {
            Name = LookupTableRadix32,
            Size = 16,
            SizeNeg = -16,
            SizeRange = 1..17,
            ConversionRange = 0..15
        }
        // radix-64
        impl_lookup_table! {
            Name = LookupTableRadix64,
            Size = 32,
            SizeNeg = -32,
            SizeRange = 1..33,
            ConversionRange = 0..31
        }
        // radix-128
        impl_lookup_table! {
            Name = LookupTableRadix128,
            Size = 64,
            SizeNeg = -64,
            SizeRange = 1..65,
            ConversionRange = 0..63
        }
        // radix-256
        impl_lookup_table! {
            Name = LookupTableRadix256,
            Size = 128,
            SizeNeg = -128,
            SizeRange = 1..129,
            ConversionRange = 0..127
        }

        // For homogeneity we then alias it to "LookupTableRadix16".
        pub(crate) type LookupTableRadix16<T> = LookupTable<T>;
    }
}

/// Holds odd multiples 1A, 3A, ..., 15A of a point A.
#[derive(Copy, Clone)]
pub(crate) struct NafLookupTable5<T>(pub(crate) [T; 8]);

impl<T: Copy> NafLookupTable5<T> {
    /// Given public, odd \\( x \\) with \\( 0 < x < 2^4 \\), return \\(xA\\).
    pub fn select(&self, x: usize) -> T {
        debug_assert_eq!(x & 1, 1);
        debug_assert!(x < 16);

        self.0[x / 2]
    }
}

impl<T: Debug> Debug for NafLookupTable5<T> {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        write!(f, "NafLookupTable5({:?})", self.0)
    }
}

impl<'a> From<&'a EdwardsPoint> for NafLookupTable5<ProjectiveNielsPoint> {
    fn from(A: &'a EdwardsPoint) -> Self {
        let mut Ai = [A.as_projective_niels(); 8];
        let A2 = A.double();
        for i in 0..7 {
            Ai[i + 1] = (&A2 + &Ai[i]).as_extended().as_projective_niels();
        }
        // Now Ai = [A, 3A, 5A, 7A, 9A, 11A, 13A, 15A]
        NafLookupTable5(Ai)
    }
}

impl<'a> From<&'a EdwardsPoint> for NafLookupTable5<AffineNielsPoint> {
    fn from(A: &'a EdwardsPoint) -> Self {
        let mut Ai = [A.as_affine_niels(); 8];
        let A2 = A.double();
        for i in 0..7 {
            Ai[i + 1] = (&A2 + &Ai[i]).as_extended().as_affine_niels();
        }
        // Now Ai = [A, 3A, 5A, 7A, 9A, 11A, 13A, 15A]
        NafLookupTable5(Ai)
    }
}

/// Holds stuff up to 8. The only time we use tables this big is for precomputed basepoint tables
/// and multiscalar multiplication (which requires alloc).
#[cfg(any(feature = "precomputed-tables", feature = "alloc"))]
#[derive(Copy, Clone)]
pub(crate) struct NafLookupTable8<T>(pub(crate) [T; 64]);

#[cfg(any(feature = "precomputed-tables", feature = "alloc"))]
impl<T: Copy> NafLookupTable8<T> {
    pub fn select(&self, x: usize) -> T {
        debug_assert_eq!(x & 1, 1);
        debug_assert!(x < 128);

        self.0[x / 2]
    }
}

#[cfg(any(feature = "precomputed-tables", feature = "alloc"))]
impl<T: Debug> Debug for NafLookupTable8<T> {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        writeln!(f, "NafLookupTable8([")?;
        for i in 0..64 {
            writeln!(f, "\t{:?},", &self.0[i])?;
        }
        write!(f, "])")
    }
}

#[cfg(any(feature = "precomputed-tables", feature = "alloc"))]
impl<'a> From<&'a EdwardsPoint> for NafLookupTable8<ProjectiveNielsPoint> {
    fn from(A: &'a EdwardsPoint) -> Self {
        let mut Ai = [A.as_projective_niels(); 64];
        let A2 = A.double();
        for i in 0..63 {
            Ai[i + 1] = (&A2 + &Ai[i]).as_extended().as_projective_niels();
        }
        // Now Ai = [A, 3A, 5A, 7A, 9A, 11A, 13A, 15A, ..., 127A]
        NafLookupTable8(Ai)
    }
}

#[cfg(any(feature = "precomputed-tables", feature = "alloc"))]
impl<'a> From<&'a EdwardsPoint> for NafLookupTable8<AffineNielsPoint> {
    fn from(A: &'a EdwardsPoint) -> Self {
        let mut Ai = [A.as_affine_niels(); 64];
        let A2 = A.double();
        for i in 0..63 {
            Ai[i + 1] = (&A2 + &Ai[i]).as_extended().as_affine_niels();
        }
        // Now Ai = [A, 3A, 5A, 7A, 9A, 11A, 13A, 15A, ..., 127A]
        NafLookupTable8(Ai)
    }
}
