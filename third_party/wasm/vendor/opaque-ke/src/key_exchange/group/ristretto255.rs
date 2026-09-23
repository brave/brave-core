// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Key Exchange group implementation for ristretto255

use curve25519_dalek::constants::RISTRETTO_BASEPOINT_POINT;
use curve25519_dalek::ristretto::{CompressedRistretto, RistrettoPoint};
use curve25519_dalek::scalar::Scalar;
use curve25519_dalek::traits::Identity;
use digest::core_api::BlockSizeUser;
use digest::{FixedOutput, HashMarker};
use generic_array::typenum::{IsLess, IsLessOrEqual, U256, U32};
use generic_array::GenericArray;
use rand::{CryptoRng, RngCore};
use subtle::ConstantTimeEq;
use voprf::Group;

use super::KeGroup;
use crate::errors::InternalError;

/// Implementation for Ristretto255.
// This is necessary because Rust lacks specialization, otherwise we could
// implement `KeGroup` for `voprf::Ristretto255`.
pub struct Ristretto255;

impl KeGroup for Ristretto255 {
    type Pk = RistrettoPoint;
    type PkLen = U32;
    type Sk = Scalar;
    type SkLen = U32;

    fn serialize_pk(pk: Self::Pk) -> GenericArray<u8, Self::PkLen> {
        pk.compress().to_bytes().into()
    }

    fn deserialize_pk(bytes: &[u8]) -> Result<Self::Pk, InternalError> {
        CompressedRistretto::from_slice(bytes)
            .map_err(|_| InternalError::PointError)?
            .decompress()
            .filter(|point| point != &RistrettoPoint::identity())
            .ok_or(InternalError::PointError)
    }

    fn random_sk<R: RngCore + CryptoRng>(rng: &mut R) -> Self::Sk {
        loop {
            let scalar = {
                #[cfg(not(test))]
                {
                    Scalar::random(rng)
                }

                // Tests need an exact conversion from bytes to scalar, sampling only 32 bytes
                // from rng
                #[cfg(test)]
                {
                    let mut scalar_bytes = [0u8; 32];
                    rng.fill_bytes(&mut scalar_bytes);
                    Scalar::from_bytes_mod_order(scalar_bytes)
                }
            };

            if scalar != Scalar::ZERO {
                break scalar;
            }
        }
    }

    // Implements the `HashToScalar()` function from
    // <https://www.ietf.org/archive/id/draft-irtf-cfrg-voprf-19.html#section-4>
    fn hash_to_scalar<'a, H>(input: &[&[u8]], dst: &[&[u8]]) -> Result<Self::Sk, InternalError>
    where
        H: BlockSizeUser + Default + FixedOutput + HashMarker,
        H::OutputSize: IsLess<U256> + IsLessOrEqual<H::BlockSize>,
    {
        <voprf::Ristretto255 as Group>::hash_to_scalar::<H>(input, dst)
            .map_err(InternalError::OprfInternalError)
    }

    fn is_zero_scalar(scalar: Self::Sk) -> subtle::Choice {
        scalar.ct_eq(&Scalar::ZERO)
    }

    fn public_key(sk: Self::Sk) -> Self::Pk {
        RISTRETTO_BASEPOINT_POINT * sk
    }

    fn diffie_hellman(pk: Self::Pk, sk: Self::Sk) -> GenericArray<u8, Self::PkLen> {
        Self::serialize_pk(pk * sk)
    }

    fn serialize_sk(sk: Self::Sk) -> GenericArray<u8, Self::SkLen> {
        sk.to_bytes().into()
    }

    fn deserialize_sk(bytes: &[u8]) -> Result<Self::Sk, InternalError> {
        bytes
            .try_into()
            .ok()
            .and_then(|bytes| Scalar::from_canonical_bytes(bytes).into())
            .filter(|scalar| scalar != &Scalar::ZERO)
            .ok_or(InternalError::PointError)
    }
}

#[cfg(feature = "ristretto255-voprf")]
impl voprf::CipherSuite for Ristretto255 {
    const ID: &'static str = voprf::Ristretto255::ID;

    type Group = <voprf::Ristretto255 as voprf::CipherSuite>::Group;

    type Hash = <voprf::Ristretto255 as voprf::CipherSuite>::Hash;
}

impl Group for Ristretto255 {
    type Elem = <voprf::Ristretto255 as Group>::Elem;

    type ElemLen = <voprf::Ristretto255 as Group>::ElemLen;

    type Scalar = <voprf::Ristretto255 as Group>::Scalar;

    type ScalarLen = <voprf::Ristretto255 as Group>::ScalarLen;

    fn hash_to_curve<H>(
        input: &[&[u8]],
        dst: &[&[u8]],
    ) -> voprf::Result<Self::Elem, voprf::InternalError>
    where
        H: BlockSizeUser + Default + FixedOutput + HashMarker,
        H::OutputSize: IsLess<U256> + IsLessOrEqual<H::BlockSize>,
    {
        <voprf::Ristretto255 as Group>::hash_to_curve::<H>(input, dst)
    }

    fn hash_to_scalar<H>(
        input: &[&[u8]],
        dst: &[&[u8]],
    ) -> voprf::Result<Self::Scalar, voprf::InternalError>
    where
        H: BlockSizeUser + Default + FixedOutput + HashMarker,
        H::OutputSize: IsLess<U256> + IsLessOrEqual<H::BlockSize>,
    {
        <voprf::Ristretto255 as Group>::hash_to_scalar::<H>(input, dst)
    }

    fn base_elem() -> Self::Elem {
        <voprf::Ristretto255 as Group>::base_elem()
    }

    fn identity_elem() -> Self::Elem {
        <voprf::Ristretto255 as Group>::identity_elem()
    }

    fn serialize_elem(elem: Self::Elem) -> GenericArray<u8, Self::ElemLen> {
        <voprf::Ristretto255 as Group>::serialize_elem(elem)
    }

    fn deserialize_elem(element_bits: &[u8]) -> voprf::Result<Self::Elem> {
        <voprf::Ristretto255 as Group>::deserialize_elem(element_bits)
    }

    fn random_scalar<R: RngCore + CryptoRng>(rng: &mut R) -> Self::Scalar {
        <voprf::Ristretto255 as Group>::random_scalar(rng)
    }

    fn invert_scalar(scalar: Self::Scalar) -> Self::Scalar {
        <voprf::Ristretto255 as Group>::invert_scalar(scalar)
    }

    fn is_zero_scalar(scalar: Self::Scalar) -> subtle::Choice {
        <voprf::Ristretto255 as Group>::is_zero_scalar(scalar)
    }

    fn serialize_scalar(scalar: Self::Scalar) -> GenericArray<u8, Self::ScalarLen> {
        <voprf::Ristretto255 as Group>::serialize_scalar(scalar)
    }

    fn deserialize_scalar(scalar_bits: &[u8]) -> voprf::Result<Self::Scalar> {
        <voprf::Ristretto255 as Group>::deserialize_scalar(scalar_bits)
    }
}
