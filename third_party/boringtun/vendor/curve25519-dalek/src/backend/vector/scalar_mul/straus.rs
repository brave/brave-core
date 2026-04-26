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

#![allow(non_snake_case)]

#[curve25519_dalek_derive::unsafe_target_feature_specialize(
    "avx2",
    conditional("avx512ifma,avx512vl", nightly)
)]
pub mod spec {

    use alloc::vec::Vec;

    use core::borrow::Borrow;
    use core::cmp::Ordering;

    #[cfg(feature = "zeroize")]
    use zeroize::Zeroizing;

    #[for_target_feature("avx2")]
    use crate::backend::vector::avx2::{CachedPoint, ExtendedPoint};

    #[for_target_feature("avx512ifma")]
    use crate::backend::vector::ifma::{CachedPoint, ExtendedPoint};

    use crate::edwards::EdwardsPoint;
    use crate::scalar::Scalar;
    use crate::traits::{Identity, MultiscalarMul, VartimeMultiscalarMul};
    use crate::window::{LookupTable, NafLookupTable5};

    /// Multiscalar multiplication using interleaved window / Straus'
    /// method.  See the `Straus` struct in the serial backend for more
    /// details.
    ///
    /// This exists as a seperate implementation from that one because the
    /// AVX2 code uses different curve models (it does not pass between
    /// multiple models during scalar mul), and it has to convert the
    /// point representation on the fly.
    pub struct Straus {}

    impl MultiscalarMul for Straus {
        type Point = EdwardsPoint;

        fn multiscalar_mul<I, J>(scalars: I, points: J) -> EdwardsPoint
        where
            I: IntoIterator,
            I::Item: Borrow<Scalar>,
            J: IntoIterator,
            J::Item: Borrow<EdwardsPoint>,
        {
            // Construct a lookup table of [P,2P,3P,4P,5P,6P,7P,8P]
            // for each input point P
            let lookup_tables: Vec<_> = points
                .into_iter()
                .map(|point| LookupTable::<CachedPoint>::from(point.borrow()))
                .collect();

            let scalar_digits_vec: Vec<_> = scalars
                .into_iter()
                .map(|s| s.borrow().as_radix_16())
                .collect();
            // Pass ownership to a `Zeroizing` wrapper
            #[cfg(feature = "zeroize")]
            let scalar_digits_vec = Zeroizing::new(scalar_digits_vec);

            let mut Q = ExtendedPoint::identity();
            for j in (0..64).rev() {
                Q = Q.mul_by_pow_2(4);
                let it = scalar_digits_vec.iter().zip(lookup_tables.iter());
                for (s_i, lookup_table_i) in it {
                    // Q = Q + s_{i,j} * P_i
                    Q = &Q + &lookup_table_i.select(s_i[j]);
                }
            }
            Q.into()
        }
    }

    impl VartimeMultiscalarMul for Straus {
        type Point = EdwardsPoint;

        fn optional_multiscalar_mul<I, J>(scalars: I, points: J) -> Option<EdwardsPoint>
        where
            I: IntoIterator,
            I::Item: Borrow<Scalar>,
            J: IntoIterator<Item = Option<EdwardsPoint>>,
        {
            let nafs: Vec<_> = scalars
                .into_iter()
                .map(|c| c.borrow().non_adjacent_form(5))
                .collect();
            let lookup_tables: Vec<_> = points
                .into_iter()
                .map(|P_opt| P_opt.map(|P| NafLookupTable5::<CachedPoint>::from(&P)))
                .collect::<Option<Vec<_>>>()?;

            let mut Q = ExtendedPoint::identity();

            for i in (0..256).rev() {
                Q = Q.double();

                for (naf, lookup_table) in nafs.iter().zip(lookup_tables.iter()) {
                    match naf[i].cmp(&0) {
                        Ordering::Greater => {
                            Q = &Q + &lookup_table.select(naf[i] as usize);
                        }
                        Ordering::Less => {
                            Q = &Q - &lookup_table.select(-naf[i] as usize);
                        }
                        Ordering::Equal => {}
                    }
                }
            }

            Some(Q.into())
        }
    }
}
