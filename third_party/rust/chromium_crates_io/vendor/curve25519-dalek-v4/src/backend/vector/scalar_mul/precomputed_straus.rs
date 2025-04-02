// -*- mode: rust; -*-
//
// This file is part of curve25519-dalek.
// Copyright (c) 2019 Henry de Valence.
// See LICENSE for licensing information.
//
// Authors:
// - Henry de Valence <hdevalence@hdevalence.ca>

//! Precomputation for Straus's method.

#![allow(non_snake_case)]

#[curve25519_dalek_derive::unsafe_target_feature_specialize(
    "avx2",
    conditional("avx512ifma,avx512vl", nightly)
)]
pub mod spec {

    use alloc::vec::Vec;

    use core::borrow::Borrow;
    use core::cmp::Ordering;

    #[for_target_feature("avx2")]
    use crate::backend::vector::avx2::{CachedPoint, ExtendedPoint};

    #[for_target_feature("avx512ifma")]
    use crate::backend::vector::ifma::{CachedPoint, ExtendedPoint};

    use crate::edwards::EdwardsPoint;
    use crate::scalar::Scalar;
    use crate::traits::Identity;
    use crate::traits::VartimePrecomputedMultiscalarMul;
    use crate::window::{NafLookupTable5, NafLookupTable8};

    pub struct VartimePrecomputedStraus {
        static_lookup_tables: Vec<NafLookupTable8<CachedPoint>>,
    }

    impl VartimePrecomputedMultiscalarMul for VartimePrecomputedStraus {
        type Point = EdwardsPoint;

        fn new<I>(static_points: I) -> Self
        where
            I: IntoIterator,
            I::Item: Borrow<EdwardsPoint>,
        {
            Self {
                static_lookup_tables: static_points
                    .into_iter()
                    .map(|P| NafLookupTable8::<CachedPoint>::from(P.borrow()))
                    .collect(),
            }
        }

        fn optional_mixed_multiscalar_mul<I, J, K>(
            &self,
            static_scalars: I,
            dynamic_scalars: J,
            dynamic_points: K,
        ) -> Option<EdwardsPoint>
        where
            I: IntoIterator,
            I::Item: Borrow<Scalar>,
            J: IntoIterator,
            J::Item: Borrow<Scalar>,
            K: IntoIterator<Item = Option<EdwardsPoint>>,
        {
            let static_nafs = static_scalars
                .into_iter()
                .map(|c| c.borrow().non_adjacent_form(5))
                .collect::<Vec<_>>();
            let dynamic_nafs: Vec<_> = dynamic_scalars
                .into_iter()
                .map(|c| c.borrow().non_adjacent_form(5))
                .collect::<Vec<_>>();

            let dynamic_lookup_tables = dynamic_points
                .into_iter()
                .map(|P_opt| P_opt.map(|P| NafLookupTable5::<CachedPoint>::from(&P)))
                .collect::<Option<Vec<_>>>()?;

            let sp = self.static_lookup_tables.len();
            let dp = dynamic_lookup_tables.len();
            assert_eq!(sp, static_nafs.len());
            assert_eq!(dp, dynamic_nafs.len());

            // We could save some doublings by looking for the highest
            // nonzero NAF coefficient, but since we might have a lot of
            // them to search, it's not clear it's worthwhile to check.
            let mut R = ExtendedPoint::identity();
            for j in (0..256).rev() {
                R = R.double();

                for i in 0..dp {
                    let t_ij = dynamic_nafs[i][j];
                    match t_ij.cmp(&0) {
                        Ordering::Greater => {
                            R = &R + &dynamic_lookup_tables[i].select(t_ij as usize);
                        }
                        Ordering::Less => {
                            R = &R - &dynamic_lookup_tables[i].select(-t_ij as usize);
                        }
                        Ordering::Equal => {}
                    }
                }

                #[allow(clippy::needless_range_loop)]
                for i in 0..sp {
                    let t_ij = static_nafs[i][j];
                    match t_ij.cmp(&0) {
                        Ordering::Greater => {
                            R = &R + &self.static_lookup_tables[i].select(t_ij as usize);
                        }
                        Ordering::Less => {
                            R = &R - &self.static_lookup_tables[i].select(-t_ij as usize);
                        }
                        Ordering::Equal => {}
                    }
                }
            }

            Some(R.into())
        }
    }
}
