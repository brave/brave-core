// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Key Exchange group implementation for Curve25519

use curve25519_dalek::montgomery::MontgomeryPoint;
use curve25519_dalek::scalar::{self, Scalar};
use curve25519_dalek::traits::Identity;
use digest::core_api::BlockSizeUser;
use digest::{FixedOutput, HashMarker, OutputSizeUser};
use generic_array::typenum::{IsLess, IsLessOrEqual, U256, U32};
use generic_array::GenericArray;
use rand::{CryptoRng, RngCore};
use subtle::ConstantTimeEq;

use super::KeGroup;
use crate::errors::InternalError;

/// Implementation for Curve25519.
pub struct Curve25519;

/// The implementation of such a subgroup for Curve25519
impl KeGroup for Curve25519 {
    type Pk = MontgomeryPoint;
    type PkLen = U32;
    type Sk = [u8; 32];
    type SkLen = U32;

    fn serialize_pk(pk: Self::Pk) -> GenericArray<u8, Self::PkLen> {
        pk.to_bytes().into()
    }

    fn deserialize_pk(bytes: &[u8]) -> Result<Self::Pk, InternalError> {
        bytes
            .try_into()
            .ok()
            .map(MontgomeryPoint)
            .filter(|pk| pk != &MontgomeryPoint::identity())
            .ok_or(InternalError::PointError)
    }

    fn random_sk<R: RngCore + CryptoRng>(rng: &mut R) -> Self::Sk {
        loop {
            // Sample 32 random bytes and then clamp, as described in https://cr.yp.to/ecdh.html
            let mut scalar_bytes = [0u8; 32];
            rng.fill_bytes(&mut scalar_bytes);
            let scalar = scalar::clamp_integer(scalar_bytes);

            if scalar != Scalar::ZERO.to_bytes() {
                break scalar;
            }
        }
    }

    fn hash_to_scalar<'a, H>(_input: &[&[u8]], _dst: &[&[u8]]) -> Result<Self::Sk, InternalError>
    where
        H: BlockSizeUser + Default + FixedOutput + HashMarker,
        H::OutputSize: IsLess<U256> + IsLessOrEqual<H::BlockSize>,
    {
        unimplemented!()
    }

    fn derive_auth_keypair<CS: voprf::CipherSuite>(
        seed: GenericArray<u8, Self::SkLen>,
    ) -> Result<Self::Sk, InternalError>
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        Ok(scalar::clamp_integer(seed.into()))
    }

    fn is_zero_scalar(scalar: Self::Sk) -> subtle::Choice {
        scalar.ct_eq(&Scalar::ZERO.to_bytes())
    }

    fn public_key(sk: Self::Sk) -> Self::Pk {
        MontgomeryPoint::mul_base_clamped(sk)
    }

    fn diffie_hellman(pk: Self::Pk, sk: Self::Sk) -> GenericArray<u8, Self::PkLen> {
        Self::serialize_pk(pk.mul_clamped(sk))
    }

    fn serialize_sk(sk: Self::Sk) -> GenericArray<u8, Self::SkLen> {
        sk.into()
    }

    fn deserialize_sk(bytes: &[u8]) -> Result<Self::Sk, InternalError> {
        bytes
            .try_into()
            .ok()
            .and_then(|bytes| {
                let scalar = scalar::clamp_integer(bytes);
                (scalar == bytes).then_some(scalar)
            })
            .filter(|scalar| scalar != &Scalar::ZERO.to_bytes())
            .ok_or(InternalError::PointError)
    }
}
