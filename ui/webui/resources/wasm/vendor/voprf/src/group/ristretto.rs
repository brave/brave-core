// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

use curve25519_dalek::constants::RISTRETTO_BASEPOINT_POINT;
use curve25519_dalek::ristretto::{CompressedRistretto, RistrettoPoint};
use curve25519_dalek::scalar::Scalar;
use curve25519_dalek::traits::Identity;
use digest::core_api::BlockSizeUser;
use digest::{FixedOutput, HashMarker};
use elliptic_curve::hash2curve::{ExpandMsg, ExpandMsgXmd, Expander};
use generic_array::typenum::{IsLess, IsLessOrEqual, U256, U32, U64};
use generic_array::GenericArray;
use rand_core::{CryptoRng, RngCore};
use subtle::ConstantTimeEq;

use super::Group;
use crate::{Error, InternalError, Result};

/// [`Group`] implementation for Ristretto255.
#[derive(Clone, Copy, Debug, Default, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct Ristretto255;

#[cfg(feature = "ristretto255-ciphersuite")]
impl crate::CipherSuite for Ristretto255 {
    const ID: &'static str = "ristretto255-SHA512";

    type Group = Ristretto255;

    type Hash = sha2::Sha512;
}

impl Group for Ristretto255 {
    type Elem = RistrettoPoint;

    type ElemLen = U32;

    type Scalar = Scalar;

    type ScalarLen = U32;

    // Implements the `hash_to_ristretto255()` function from
    // https://www.rfc-editor.org/rfc/rfc9380.html#appendix-B
    fn hash_to_curve<H>(input: &[&[u8]], dst: &[&[u8]]) -> Result<Self::Elem, InternalError>
    where
        H: BlockSizeUser + Default + FixedOutput + HashMarker,
        H::OutputSize: IsLess<U256> + IsLessOrEqual<H::BlockSize>,
    {
        let mut uniform_bytes = GenericArray::<_, U64>::default();
        ExpandMsgXmd::<H>::expand_message(input, dst, 64)
            .map_err(|_| InternalError::Input)?
            .fill_bytes(&mut uniform_bytes);

        Ok(RistrettoPoint::from_uniform_bytes(&uniform_bytes.into()))
    }

    // Implements the `HashToScalar()` function from
    // https://www.rfc-editor.org/rfc/rfc9497#section-4.1
    fn hash_to_scalar<H>(input: &[&[u8]], dst: &[&[u8]]) -> Result<Self::Scalar, InternalError>
    where
        H: BlockSizeUser + Default + FixedOutput + HashMarker,
        H::OutputSize: IsLess<U256> + IsLessOrEqual<H::BlockSize>,
    {
        let mut uniform_bytes = GenericArray::<_, U64>::default();
        ExpandMsgXmd::<H>::expand_message(input, dst, 64)
            .map_err(|_| InternalError::Input)?
            .fill_bytes(&mut uniform_bytes);

        Ok(Scalar::from_bytes_mod_order_wide(&uniform_bytes.into()))
    }

    fn base_elem() -> Self::Elem {
        RISTRETTO_BASEPOINT_POINT
    }

    fn identity_elem() -> Self::Elem {
        RistrettoPoint::identity()
    }

    // serialization of a group element
    fn serialize_elem(elem: Self::Elem) -> GenericArray<u8, Self::ElemLen> {
        elem.compress().to_bytes().into()
    }

    fn deserialize_elem(element_bits: &[u8]) -> Result<Self::Elem> {
        CompressedRistretto::from_slice(element_bits)
            .map_err(|_| Error::Deserialization)?
            .decompress()
            .filter(|point| point != &RistrettoPoint::identity())
            .ok_or(Error::Deserialization)
    }

    fn random_scalar<R: RngCore + CryptoRng>(rng: &mut R) -> Self::Scalar {
        loop {
            let scalar = Scalar::random(rng);

            if scalar != Scalar::ZERO {
                break scalar;
            }
        }
    }

    fn invert_scalar(scalar: Self::Scalar) -> Self::Scalar {
        scalar.invert()
    }

    fn is_zero_scalar(scalar: Self::Scalar) -> subtle::Choice {
        scalar.ct_eq(&Scalar::ZERO)
    }

    #[cfg(test)]
    fn zero_scalar() -> Self::Scalar {
        Scalar::ZERO
    }

    fn serialize_scalar(scalar: Self::Scalar) -> GenericArray<u8, Self::ScalarLen> {
        scalar.to_bytes().into()
    }

    fn deserialize_scalar(scalar_bits: &[u8]) -> Result<Self::Scalar> {
        scalar_bits
            .try_into()
            .ok()
            .and_then(|bytes| Scalar::from_canonical_bytes(bytes).into())
            .filter(|scalar| scalar != &Scalar::ZERO)
            .ok_or(Error::Deserialization)
    }
}
