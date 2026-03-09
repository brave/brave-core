// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

use digest::core_api::BlockSizeUser;
use digest::{FixedOutput, HashMarker};
use elliptic_curve::group::cofactor::CofactorGroup;
use elliptic_curve::hash2curve::{ExpandMsgXmd, FromOkm, GroupDigest};
use elliptic_curve::sec1::{FromEncodedPoint, ModulusSize, ToEncodedPoint};
use elliptic_curve::{
    AffinePoint, Field, FieldBytesSize, Group, ProjectivePoint, PublicKey, Scalar, SecretKey,
};
use generic_array::typenum::{IsLess, IsLessOrEqual, U256};
use generic_array::GenericArray;
use rand::{CryptoRng, RngCore};

use super::KeGroup;
use crate::errors::InternalError;

impl<G> KeGroup for G
where
    G: GroupDigest,
    FieldBytesSize<Self>: ModulusSize,
    AffinePoint<Self>: FromEncodedPoint<Self> + ToEncodedPoint<Self>,
    ProjectivePoint<Self>: CofactorGroup + ToEncodedPoint<Self>,
    Scalar<Self>: FromOkm,
{
    type Pk = ProjectivePoint<Self>;

    type PkLen = <FieldBytesSize<Self> as ModulusSize>::CompressedPointSize;

    type Sk = Scalar<Self>;

    type SkLen = FieldBytesSize<Self>;

    fn serialize_pk(pk: Self::Pk) -> GenericArray<u8, Self::PkLen> {
        GenericArray::clone_from_slice(pk.to_encoded_point(true).as_bytes())
    }

    fn deserialize_pk(bytes: &[u8]) -> Result<Self::Pk, InternalError> {
        PublicKey::<Self>::from_sec1_bytes(bytes)
            .map(|public_key| public_key.to_projective())
            .map_err(|_| InternalError::PointError)
    }

    fn random_sk<R: RngCore + CryptoRng>(rng: &mut R) -> Self::Sk {
        *SecretKey::<Self>::random(rng).to_nonzero_scalar()
    }

    // Implements the `HashToScalar()` function from
    // <https://www.ietf.org/archive/id/draft-irtf-cfrg-voprf-19.html#section-4>
    fn hash_to_scalar<H>(input: &[&[u8]], dst: &[&[u8]]) -> Result<Self::Sk, InternalError>
    where
        H: BlockSizeUser + Default + FixedOutput + HashMarker,
        H::OutputSize: IsLess<U256> + IsLessOrEqual<H::BlockSize>,
    {
        Self::hash_to_scalar::<ExpandMsgXmd<H>>(input, dst)
            .map_err(|_| InternalError::HashToScalar)
            .and_then(|scalar| {
                if bool::from(scalar.is_zero()) {
                    Err(InternalError::HashToScalar)
                } else {
                    Ok(scalar)
                }
            })
    }

    fn public_key(sk: Self::Sk) -> Self::Pk {
        ProjectivePoint::<Self>::generator() * sk
    }

    fn is_zero_scalar(scalar: Self::Sk) -> subtle::Choice {
        scalar.is_zero()
    }

    fn diffie_hellman(pk: Self::Pk, sk: Self::Sk) -> GenericArray<u8, Self::PkLen> {
        Self::serialize_pk(pk * sk)
    }

    fn serialize_sk(sk: Self::Sk) -> GenericArray<u8, Self::SkLen> {
        sk.into()
    }

    fn deserialize_sk(bytes: &[u8]) -> Result<Self::Sk, InternalError> {
        SecretKey::<Self>::from_slice(bytes)
            .map(|secret_key| *secret_key.to_nonzero_scalar())
            .map_err(|_| InternalError::PointError)
    }
}
