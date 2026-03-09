// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Handles the serialization of each of the components used in the VOPRF
//! protocol

use core::ops::Add;

use digest::core_api::BlockSizeUser;
use digest::OutputSizeUser;
use generic_array::sequence::Concat;
use generic_array::typenum::{IsLess, IsLessOrEqual, Sum, Unsigned, U256};
use generic_array::{ArrayLength, GenericArray};

use crate::{
    BlindedElement, CipherSuite, Error, EvaluationElement, Group, OprfClient, OprfServer,
    PoprfClient, PoprfServer, Proof, Result, VoprfClient, VoprfServer,
};

//////////////////////////////////////////////////////////
// Serialization and Deserialization for High-Level API //
// ==================================================== //
//////////////////////////////////////////////////////////

/// Length of [`OprfClient`] in bytes for serialization.
pub type OprfClientLen<CS> = <<CS as CipherSuite>::Group as Group>::ScalarLen;

impl<CS: CipherSuite> OprfClient<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, OprfClientLen<CS>> {
        CS::Group::serialize_scalar(self.blind)
    }

    /// Deserialization from bytes
    ///
    /// # Errors
    /// [`Error::Deserialization`] if failed to deserialize `input`.
    pub fn deserialize(mut input: &[u8]) -> Result<Self> {
        let blind = deserialize_scalar::<CS::Group>(&mut input)?;

        Ok(Self { blind })
    }
}

/// Length of [`VoprfClient`] in bytes for serialization.
pub type VoprfClientLen<CS> = Sum<
    <<CS as CipherSuite>::Group as Group>::ScalarLen,
    <<CS as CipherSuite>::Group as Group>::ElemLen,
>;

impl<CS: CipherSuite> VoprfClient<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, VoprfClientLen<CS>>
    where
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ElemLen>,
        VoprfClientLen<CS>: ArrayLength<u8>,
    {
        <CS::Group as Group>::serialize_scalar(self.blind)
            .concat(<CS::Group as Group>::serialize_elem(self.blinded_element))
    }

    /// Deserialization from bytes
    ///
    /// # Errors
    /// [`Error::Deserialization`] if failed to deserialize `input`.
    pub fn deserialize(mut input: &[u8]) -> Result<Self> {
        let blind = deserialize_scalar::<CS::Group>(&mut input)?;
        let blinded_element = deserialize_elem::<CS::Group>(&mut input)?;

        Ok(Self {
            blind,
            blinded_element,
        })
    }
}

/// Length of [`PoprfClient`] in bytes for serialization.
pub type PoprfClientLen<CS> = Sum<
    <<CS as CipherSuite>::Group as Group>::ScalarLen,
    <<CS as CipherSuite>::Group as Group>::ElemLen,
>;

impl<CS: CipherSuite> PoprfClient<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, PoprfClientLen<CS>>
    where
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ElemLen>,
        PoprfClientLen<CS>: ArrayLength<u8>,
    {
        <CS::Group as Group>::serialize_scalar(self.blind)
            .concat(<CS::Group as Group>::serialize_elem(self.blinded_element))
    }

    /// Deserialization from bytes
    ///
    /// # Errors
    /// [`Error::Deserialization`] if failed to deserialize `input`.
    pub fn deserialize(mut input: &[u8]) -> Result<Self> {
        let blind = deserialize_scalar::<CS::Group>(&mut input)?;
        let blinded_element = deserialize_elem::<CS::Group>(&mut input)?;

        Ok(Self {
            blind,
            blinded_element,
        })
    }
}

/// Length of [`OprfServer`] in bytes for serialization.
pub type OprfServerLen<CS> = <<CS as CipherSuite>::Group as Group>::ScalarLen;

impl<CS: CipherSuite> OprfServer<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, OprfServerLen<CS>> {
        CS::Group::serialize_scalar(self.sk)
    }

    /// Deserialization from bytes
    ///
    /// # Errors
    /// [`Error::Deserialization`] if failed to deserialize `input`.
    pub fn deserialize(mut input: &[u8]) -> Result<Self> {
        let sk = deserialize_scalar::<CS::Group>(&mut input)?;

        Ok(Self { sk })
    }
}

/// Length of [`VoprfServer`] in bytes for serialization.
pub type VoprfServerLen<CS> = Sum<
    <<CS as CipherSuite>::Group as Group>::ScalarLen,
    <<CS as CipherSuite>::Group as Group>::ElemLen,
>;

impl<CS: CipherSuite> VoprfServer<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, VoprfServerLen<CS>>
    where
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ElemLen>,
        VoprfServerLen<CS>: ArrayLength<u8>,
    {
        CS::Group::serialize_scalar(self.sk).concat(CS::Group::serialize_elem(self.pk))
    }

    /// Deserialization from bytes
    ///
    /// # Errors
    /// [`Error::Deserialization`] if failed to deserialize `input`.
    pub fn deserialize(mut input: &[u8]) -> Result<Self> {
        let sk = deserialize_scalar::<CS::Group>(&mut input)?;
        let pk = deserialize_elem::<CS::Group>(&mut input)?;

        Ok(Self { sk, pk })
    }
}

/// Length of [`PoprfServer`] in bytes for serialization.
pub type PoprfServerLen<CS> = Sum<
    <<CS as CipherSuite>::Group as Group>::ScalarLen,
    <<CS as CipherSuite>::Group as Group>::ElemLen,
>;

impl<CS: CipherSuite> PoprfServer<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, PoprfServerLen<CS>>
    where
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ElemLen>,
        PoprfServerLen<CS>: ArrayLength<u8>,
    {
        CS::Group::serialize_scalar(self.sk).concat(CS::Group::serialize_elem(self.pk))
    }

    /// Deserialization from bytes
    ///
    /// # Errors
    /// [`Error::Deserialization`] if failed to deserialize `input`.
    pub fn deserialize(mut input: &[u8]) -> Result<Self> {
        let sk = deserialize_scalar::<CS::Group>(&mut input)?;
        let pk = deserialize_elem::<CS::Group>(&mut input)?;

        Ok(Self { sk, pk })
    }
}

/// Length of [`Proof`] in bytes for serialization.
pub type ProofLen<CS> = Sum<
    <<CS as CipherSuite>::Group as Group>::ScalarLen,
    <<CS as CipherSuite>::Group as Group>::ScalarLen,
>;

impl<CS: CipherSuite> Proof<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, ProofLen<CS>>
    where
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ScalarLen>,
        ProofLen<CS>: ArrayLength<u8>,
    {
        CS::Group::serialize_scalar(self.c_scalar)
            .concat(CS::Group::serialize_scalar(self.s_scalar))
    }

    /// Deserialization from bytes
    ///
    /// # Errors
    /// [`Error::Deserialization`] if failed to deserialize `input`.
    pub fn deserialize(mut input: &[u8]) -> Result<Self> {
        let c_scalar = deserialize_scalar::<CS::Group>(&mut input)?;
        let s_scalar = deserialize_scalar::<CS::Group>(&mut input)?;

        Ok(Proof { c_scalar, s_scalar })
    }
}

/// Length of [`BlindedElement`] in bytes for serialization.
pub type BlindedElementLen<CS> = <<CS as CipherSuite>::Group as Group>::ElemLen;

impl<CS: CipherSuite> BlindedElement<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, BlindedElementLen<CS>> {
        CS::Group::serialize_elem(self.0)
    }

    /// Deserialization from bytes
    ///
    /// # Errors
    /// [`Error::Deserialization`] if failed to deserialize `input`.
    pub fn deserialize(mut input: &[u8]) -> Result<Self> {
        let value = deserialize_elem::<CS::Group>(&mut input)?;

        Ok(Self(value))
    }
}

/// Length of [`EvaluationElement`] in bytes for serialization.
pub type EvaluationElementLen<CS> = <<CS as CipherSuite>::Group as Group>::ElemLen;

impl<CS: CipherSuite> EvaluationElement<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, EvaluationElementLen<CS>> {
        CS::Group::serialize_elem(self.0)
    }

    /// Deserialization from bytes
    ///
    /// # Errors
    /// [`Error::Deserialization`] if failed to deserialize `input`.
    pub fn deserialize(mut input: &[u8]) -> Result<Self> {
        let value = deserialize_elem::<CS::Group>(&mut input)?;

        Ok(Self(value))
    }
}

fn deserialize_elem<G: Group>(input: &mut &[u8]) -> Result<G::Elem> {
    let input = input
        .take_ext(G::ElemLen::USIZE)
        .ok_or(Error::Deserialization)?;
    G::deserialize_elem(input)
}

fn deserialize_scalar<G: Group>(input: &mut &[u8]) -> Result<G::Scalar> {
    let input = input
        .take_ext(G::ScalarLen::USIZE)
        .ok_or(Error::Deserialization)?;
    G::deserialize_scalar(input)
}

trait SliceExt {
    fn take_ext<'a>(self: &mut &'a Self, take: usize) -> Option<&'a Self>;
}

impl<T> SliceExt for [T] {
    fn take_ext<'a>(self: &mut &'a Self, take: usize) -> Option<&'a Self> {
        if take > self.len() {
            return None;
        }

        let (front, back) = self.split_at(take);
        *self = back;
        Some(front)
    }
}

#[cfg(feature = "serde")]
pub(crate) mod serde {
    use core::marker::PhantomData;

    use generic_array::GenericArray;
    use serde::de::{Deserializer, Error};
    use serde::ser::Serializer;
    use serde::{Deserialize, Serialize};

    use crate::Group;

    pub(crate) struct Element<G: Group>(PhantomData<G>);

    impl<'de, G: Group> Element<G> {
        pub(crate) fn deserialize<D>(deserializer: D) -> Result<G::Elem, D::Error>
        where
            D: Deserializer<'de>,
        {
            GenericArray::<_, G::ElemLen>::deserialize(deserializer)
                .and_then(|bytes| G::deserialize_elem(&bytes).map_err(D::Error::custom))
        }

        pub(crate) fn serialize<S>(self_: &G::Elem, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
        {
            G::serialize_elem(*self_).serialize(serializer)
        }
    }

    pub(crate) struct Scalar<G: Group>(PhantomData<G>);

    impl<'de, G: Group> Scalar<G> {
        pub(crate) fn deserialize<D>(deserializer: D) -> Result<G::Scalar, D::Error>
        where
            D: Deserializer<'de>,
        {
            GenericArray::<_, G::ScalarLen>::deserialize(deserializer)
                .and_then(|bytes| G::deserialize_scalar(&bytes).map_err(D::Error::custom))
        }

        pub(crate) fn serialize<S>(self_: &G::Scalar, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
        {
            G::serialize_scalar(*self_).serialize(serializer)
        }
    }
}

#[cfg(test)]
mod test {
    use proptest::collection::vec;
    use proptest::prelude::*;

    use crate::{
        BlindedElement, EvaluationElement, OprfClient, OprfServer, PoprfClient, PoprfServer, Proof,
        VoprfClient, VoprfServer,
    };

    macro_rules! test_deserialize {
        ($item:ident, $bytes:ident) => {
            #[cfg(feature = "ristretto255")]
            {
                let _ = $item::<crate::Ristretto255>::deserialize(&$bytes[..]);
            }

            let _ = $item::<p256::NistP256>::deserialize(&$bytes[..]);
            let _ = $item::<p384::NistP384>::deserialize(&$bytes[..]);
            let _ = $item::<p521::NistP521>::deserialize(&$bytes[..]);
        };
    }

    proptest! {
        #[test]
        fn test_nocrash_oprf_client(bytes in vec(any::<u8>(), 0..200)) {
            test_deserialize!(OprfClient, bytes);
        }

        #[test]
        fn test_nocrash_voprf_client(bytes in vec(any::<u8>(), 0..200)) {
            test_deserialize!(VoprfClient, bytes);
        }

        #[test]
        fn test_nocrash_poprf_client(bytes in vec(any::<u8>(), 0..200)) {
            test_deserialize!(PoprfClient, bytes);
        }

        #[test]
        fn test_nocrash_oprf_server(bytes in vec(any::<u8>(), 0..200)) {
            test_deserialize!(OprfServer, bytes);
        }

        #[test]
        fn test_nocrash_voprf_server(bytes in vec(any::<u8>(), 0..200)) {
            test_deserialize!(VoprfServer, bytes);
        }

        #[test]
        fn test_nocrash_poprf_server(bytes in vec(any::<u8>(), 0..200)) {
            test_deserialize!(PoprfServer, bytes);
        }


        #[test]
        fn test_nocrash_blinded_element(bytes in vec(any::<u8>(), 0..200)) {
            test_deserialize!(BlindedElement, bytes);
        }

        #[test]
        fn test_nocrash_evaluation_element(bytes in vec(any::<u8>(), 0..200)) {
            test_deserialize!(EvaluationElement, bytes);
        }

        #[test]
        fn test_nocrash_proof(bytes in vec(any::<u8>(), 0..200)) {
            test_deserialize!(Proof, bytes);
        }
    }
}
