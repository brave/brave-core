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

//! **INTERNALS:** Pluggable implementations for different architectures.
//!
//! The backend code is split into two parts: a serial backend,
//! and a vector backend.
//!
//! The [`serial`] backend contains 32- and 64-bit implementations of
//! field arithmetic and scalar arithmetic, as well as implementations
//! of point operations using the mixed-model strategy (passing
//! between different curve models depending on the operation).
//!
//! The [`vector`] backend contains implementations of vectorized
//! field arithmetic, used to implement point operations using a novel
//! implementation strategy derived from parallel formulas of Hisil,
//! Wong, Carter, and Dawson.
//!
//! Because the two strategies give rise to different curve models,
//! it's not possible to reuse exactly the same scalar multiplication
//! code (or to write it generically), so both serial and vector
//! backends contain matching implementations of scalar multiplication
//! algorithms.  These are intended to be selected by a `#[cfg]`-based
//! type alias.
//!
//! The [`vector`] backend is selected by the `simd_backend` cargo
//! feature; it uses the [`serial`] backend for non-vectorized operations.

use crate::EdwardsPoint;
use crate::Scalar;

pub mod serial;

#[cfg(curve25519_dalek_backend = "simd")]
pub mod vector;

#[derive(Copy, Clone)]
enum BackendKind {
    #[cfg(curve25519_dalek_backend = "simd")]
    Avx2,
    #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
    Avx512,
    Serial,
}

#[inline]
fn get_selected_backend() -> BackendKind {
    #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
    {
        cpufeatures::new!(cpuid_avx512, "avx512ifma", "avx512vl");
        let token_avx512: cpuid_avx512::InitToken = cpuid_avx512::init();
        if token_avx512.get() {
            return BackendKind::Avx512;
        }
    }

    #[cfg(curve25519_dalek_backend = "simd")]
    {
        cpufeatures::new!(cpuid_avx2, "avx2");
        let token_avx2: cpuid_avx2::InitToken = cpuid_avx2::init();
        if token_avx2.get() {
            return BackendKind::Avx2;
        }
    }

    BackendKind::Serial
}

#[allow(missing_docs)]
#[cfg(feature = "alloc")]
pub fn pippenger_optional_multiscalar_mul<I, J>(scalars: I, points: J) -> Option<EdwardsPoint>
where
    I: IntoIterator,
    I::Item: core::borrow::Borrow<Scalar>,
    J: IntoIterator<Item = Option<EdwardsPoint>>,
{
    use crate::traits::VartimeMultiscalarMul;

    match get_selected_backend() {
        #[cfg(curve25519_dalek_backend = "simd")]
        BackendKind::Avx2 =>
            self::vector::scalar_mul::pippenger::spec_avx2::Pippenger::optional_multiscalar_mul::<I, J>(scalars, points),
        #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
        BackendKind::Avx512 =>
            self::vector::scalar_mul::pippenger::spec_avx512ifma_avx512vl::Pippenger::optional_multiscalar_mul::<I, J>(scalars, points),
        BackendKind::Serial =>
            self::serial::scalar_mul::pippenger::Pippenger::optional_multiscalar_mul::<I, J>(scalars, points),
    }
}

#[cfg(feature = "alloc")]
pub(crate) enum VartimePrecomputedStraus {
    #[cfg(curve25519_dalek_backend = "simd")]
    Avx2(self::vector::scalar_mul::precomputed_straus::spec_avx2::VartimePrecomputedStraus),
    #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
    Avx512ifma(
        self::vector::scalar_mul::precomputed_straus::spec_avx512ifma_avx512vl::VartimePrecomputedStraus,
    ),
    Scalar(self::serial::scalar_mul::precomputed_straus::VartimePrecomputedStraus),
}

#[cfg(feature = "alloc")]
impl VartimePrecomputedStraus {
    pub fn new<I>(static_points: I) -> Self
    where
        I: IntoIterator,
        I::Item: core::borrow::Borrow<EdwardsPoint>,
    {
        use crate::traits::VartimePrecomputedMultiscalarMul;

        match get_selected_backend() {
            #[cfg(curve25519_dalek_backend = "simd")]
            BackendKind::Avx2 =>
                VartimePrecomputedStraus::Avx2(self::vector::scalar_mul::precomputed_straus::spec_avx2::VartimePrecomputedStraus::new(static_points)),
            #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
            BackendKind::Avx512 =>
                VartimePrecomputedStraus::Avx512ifma(self::vector::scalar_mul::precomputed_straus::spec_avx512ifma_avx512vl::VartimePrecomputedStraus::new(static_points)),
            BackendKind::Serial =>
                VartimePrecomputedStraus::Scalar(self::serial::scalar_mul::precomputed_straus::VartimePrecomputedStraus::new(static_points))
        }
    }

    pub fn optional_mixed_multiscalar_mul<I, J, K>(
        &self,
        static_scalars: I,
        dynamic_scalars: J,
        dynamic_points: K,
    ) -> Option<EdwardsPoint>
    where
        I: IntoIterator,
        I::Item: core::borrow::Borrow<Scalar>,
        J: IntoIterator,
        J::Item: core::borrow::Borrow<Scalar>,
        K: IntoIterator<Item = Option<EdwardsPoint>>,
    {
        use crate::traits::VartimePrecomputedMultiscalarMul;

        match self {
            #[cfg(curve25519_dalek_backend = "simd")]
            VartimePrecomputedStraus::Avx2(inner) => inner.optional_mixed_multiscalar_mul(
                static_scalars,
                dynamic_scalars,
                dynamic_points,
            ),
            #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
            VartimePrecomputedStraus::Avx512ifma(inner) => inner.optional_mixed_multiscalar_mul(
                static_scalars,
                dynamic_scalars,
                dynamic_points,
            ),
            VartimePrecomputedStraus::Scalar(inner) => inner.optional_mixed_multiscalar_mul(
                static_scalars,
                dynamic_scalars,
                dynamic_points,
            ),
        }
    }
}

#[allow(missing_docs)]
#[cfg(feature = "alloc")]
pub fn straus_multiscalar_mul<I, J>(scalars: I, points: J) -> EdwardsPoint
where
    I: IntoIterator,
    I::Item: core::borrow::Borrow<Scalar>,
    J: IntoIterator,
    J::Item: core::borrow::Borrow<EdwardsPoint>,
{
    use crate::traits::MultiscalarMul;

    match get_selected_backend() {
        #[cfg(curve25519_dalek_backend = "simd")]
        BackendKind::Avx2 => {
            self::vector::scalar_mul::straus::spec_avx2::Straus::multiscalar_mul::<I, J>(
                scalars, points,
            )
        }
        #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
        BackendKind::Avx512 => {
            self::vector::scalar_mul::straus::spec_avx512ifma_avx512vl::Straus::multiscalar_mul::<
                I,
                J,
            >(scalars, points)
        }
        BackendKind::Serial => {
            self::serial::scalar_mul::straus::Straus::multiscalar_mul::<I, J>(scalars, points)
        }
    }
}

#[allow(missing_docs)]
#[cfg(feature = "alloc")]
pub fn straus_optional_multiscalar_mul<I, J>(scalars: I, points: J) -> Option<EdwardsPoint>
where
    I: IntoIterator,
    I::Item: core::borrow::Borrow<Scalar>,
    J: IntoIterator<Item = Option<EdwardsPoint>>,
{
    use crate::traits::VartimeMultiscalarMul;

    match get_selected_backend() {
        #[cfg(curve25519_dalek_backend = "simd")]
        BackendKind::Avx2 => {
            self::vector::scalar_mul::straus::spec_avx2::Straus::optional_multiscalar_mul::<I, J>(
                scalars, points,
            )
        }
        #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
        BackendKind::Avx512 => {
            self::vector::scalar_mul::straus::spec_avx512ifma_avx512vl::Straus::optional_multiscalar_mul::<
                I,
                J,
            >(scalars, points)
        }
        BackendKind::Serial => {
            self::serial::scalar_mul::straus::Straus::optional_multiscalar_mul::<I, J>(
                scalars, points,
            )
        }
    }
}

/// Perform constant-time, variable-base scalar multiplication.
pub fn variable_base_mul(point: &EdwardsPoint, scalar: &Scalar) -> EdwardsPoint {
    match get_selected_backend() {
        #[cfg(curve25519_dalek_backend = "simd")]
        BackendKind::Avx2 => self::vector::scalar_mul::variable_base::spec_avx2::mul(point, scalar),
        #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
        BackendKind::Avx512 => {
            self::vector::scalar_mul::variable_base::spec_avx512ifma_avx512vl::mul(point, scalar)
        }
        BackendKind::Serial => self::serial::scalar_mul::variable_base::mul(point, scalar),
    }
}

/// Compute \\(aA + bB\\) in variable time, where \\(B\\) is the Ed25519 basepoint.
#[allow(non_snake_case)]
pub fn vartime_double_base_mul(a: &Scalar, A: &EdwardsPoint, b: &Scalar) -> EdwardsPoint {
    match get_selected_backend() {
        #[cfg(curve25519_dalek_backend = "simd")]
        BackendKind::Avx2 => self::vector::scalar_mul::vartime_double_base::spec_avx2::mul(a, A, b),
        #[cfg(all(curve25519_dalek_backend = "simd", nightly))]
        BackendKind::Avx512 => {
            self::vector::scalar_mul::vartime_double_base::spec_avx512ifma_avx512vl::mul(a, A, b)
        }
        BackendKind::Serial => self::serial::scalar_mul::vartime_double_base::mul(a, A, b),
    }
}
