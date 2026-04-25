// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

use digest::core_api::{BlockSizeUser, OutputSizeUser};
use digest::Output;
use generic_array::typenum::{IsLess, IsLessOrEqual, Le, NonZero, U256};
use generic_array::{ArrayLength, GenericArray};
use rand::{CryptoRng, RngCore};
use zeroize::ZeroizeOnDrop;

use crate::ciphersuite::{CipherSuite, OprfHash};
use crate::errors::ProtocolError;
use crate::hash::{Hash, ProxyHash};
use crate::key_exchange::group::KeGroup;
use crate::keypair::{PrivateKey, PublicKey, SecretKey};

pub trait KeyExchange<D: Hash, G: KeGroup>
where
    D::Core: ProxyHash,
    <D::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<D::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    type KE1State: Deserialize + Serialize + ZeroizeOnDrop + Clone;
    type KE2State: Deserialize + Serialize + ZeroizeOnDrop + Clone;
    type KE1Message: Deserialize + Serialize + ZeroizeOnDrop + Clone;
    type KE2Message: Deserialize + Serialize + ZeroizeOnDrop + Clone;
    type KE3Message: Deserialize + Serialize + ZeroizeOnDrop + Clone;

    fn generate_ke1<OprfCs: voprf::CipherSuite, R: RngCore + CryptoRng>(
        rng: &mut R,
    ) -> Result<(Self::KE1State, Self::KE1Message), ProtocolError>
    where
        <OprfCs::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<OprfCs::Hash as BlockSizeUser>::BlockSize>;

    #[allow(clippy::too_many_arguments)]
    fn generate_ke2<
        'a,
        'b,
        'c,
        'd,
        OprfCs: voprf::CipherSuite,
        R: RngCore + CryptoRng,
        S: SecretKey<G>,
    >(
        rng: &mut R,
        l1_bytes: impl Iterator<Item = &'a [u8]>,
        l2_bytes: impl Iterator<Item = &'b [u8]>,
        ke1_message: Self::KE1Message,
        client_s_pk: PublicKey<G>,
        server_s_sk: S,
        id_u: impl Iterator<Item = &'c [u8]>,
        id_s: impl Iterator<Item = &'d [u8]>,
        context: &[u8],
    ) -> Result<GenerateKe2Result<Self, D, G>, ProtocolError<S::Error>>
    where
        <OprfCs::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<OprfCs::Hash as BlockSizeUser>::BlockSize>;

    #[allow(clippy::too_many_arguments)]
    fn generate_ke3<'a, 'b, 'c, 'd>(
        l2_component: impl Iterator<Item = &'a [u8]>,
        ke2_message: Self::KE2Message,
        ke1_state: &Self::KE1State,
        serialized_credential_request: impl Iterator<Item = &'b [u8]>,
        server_s_pk: PublicKey<G>,
        client_s_sk: PrivateKey<G>,
        id_u: impl Iterator<Item = &'c [u8]>,
        id_s: impl Iterator<Item = &'d [u8]>,
        context: &[u8],
    ) -> Result<GenerateKe3Result<Self, D, G>, ProtocolError>;

    fn finish_ke(
        ke3_message: Self::KE3Message,
        ke2_state: &Self::KE2State,
    ) -> Result<Output<D>, ProtocolError>;
}

pub trait Deserialize: Sized {
    fn deserialize(input: &[u8]) -> Result<Self, ProtocolError>;
}

pub trait Serialize {
    type Len: ArrayLength<u8>;

    fn serialize(&self) -> GenericArray<u8, Self::Len>;
}

#[cfg(not(test))]
pub type GenerateKe2Result<K, D, G> = (
    <K as KeyExchange<D, G>>::KE2State,
    <K as KeyExchange<D, G>>::KE2Message,
);
#[cfg(test)]
pub type GenerateKe2Result<K, D, G> = (
    <K as KeyExchange<D, G>>::KE2State,
    <K as KeyExchange<D, G>>::KE2Message,
    Output<D>,
    Output<D>,
);
#[cfg(not(test))]
pub type GenerateKe3Result<K, D, G> = (Output<D>, <K as KeyExchange<D, G>>::KE3Message);
#[cfg(test)]
pub type GenerateKe3Result<K, D, G> = (
    Output<D>,
    <K as KeyExchange<D, G>>::KE3Message,
    Output<D>,
    Output<D>,
);

pub type Ke1StateLen<CS: CipherSuite> =
    <<CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE1State as Serialize>::Len;
pub type Ke1MessageLen<CS: CipherSuite> =
    <<CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE1Message as Serialize>::Len;
pub type Ke2StateLen<CS: CipherSuite> =
    <<CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE2State as Serialize>::Len;
pub type Ke2MessageLen<CS: CipherSuite> =
    <<CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE2Message as Serialize>::Len;
pub type Ke3MessageLen<CS: CipherSuite> =
    <<CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE3Message as Serialize>::Len;
