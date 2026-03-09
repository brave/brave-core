// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Provides the main OPAQUE API

use core::ops::Add;

use derive_where::derive_where;
use digest::core_api::{BlockSizeUser, CoreProxy};
use digest::{Output, OutputSizeUser};
use generic_array::sequence::Concat;
use generic_array::typenum::{IsLess, IsLessOrEqual, Le, NonZero, Sum, Unsigned, U2, U256};
use generic_array::{ArrayLength, GenericArray};
use hkdf::{Hkdf, HkdfExtract};
use rand::{CryptoRng, RngCore};
use subtle::ConstantTimeEq;
use voprf::Group;

use crate::ciphersuite::{CipherSuite, OprfGroup, OprfHash};
use crate::envelope::{Envelope, EnvelopeLen};
use crate::errors::utils::check_slice_size;
use crate::errors::{InternalError, ProtocolError};
use crate::hash::{Hash, OutputSize, ProxyHash};
use crate::key_exchange::group::KeGroup;
use crate::key_exchange::traits::{
    Deserialize, Ke1MessageLen, Ke1StateLen, Ke2StateLen, KeyExchange, Serialize,
};
use crate::key_exchange::tripledh::NonceLen;
use crate::keypair::{KeyPair, PrivateKey, PublicKey, SecretKey};
use crate::ksf::Ksf;
use crate::messages::{CredentialRequestLen, RegistrationUploadLen};
use crate::serialization::Input;
use crate::{
    CredentialFinalization, CredentialRequest, CredentialResponse, RegistrationRequest,
    RegistrationResponse, RegistrationUpload,
};

///////////////
// Constants //
// ========= //
///////////////

const STR_CREDENTIAL_RESPONSE_PAD: &[u8; 21] = b"CredentialResponsePad";
const STR_MASKING_KEY: &[u8; 10] = b"MaskingKey";
const STR_OPRF_KEY: &[u8; 7] = b"OprfKey";
const STR_OPAQUE_DERIVE_KEY_PAIR: &[u8; 20] = b"OPAQUE-DeriveKeyPair";

////////////////////////////
// High-level API Structs //
// ====================== //
////////////////////////////

/// The state elements the server holds upon setup
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound(
        deserialize = "S: serde::Deserialize<'de>",
        serialize = "S: serde::Serialize"
    ))
)]
#[derive_where(Clone)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::KeGroup as KeGroup>::Pk, <CS::KeGroup as KeGroup>::Sk, S)]
pub struct ServerSetup<
    CS: CipherSuite,
    S: SecretKey<CS::KeGroup> = PrivateKey<<CS as CipherSuite>::KeGroup>,
> where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    oprf_seed: Output<OprfHash<CS>>,
    keypair: KeyPair<CS::KeGroup, S>,
    pub(crate) fake_keypair: KeyPair<CS::KeGroup>,
}

/// The state elements the client holds to register itself
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(
    Debug, Eq, Hash, PartialEq;
    voprf::OprfClient<CS::OprfCs>,
    voprf::BlindedElement<CS::OprfCs>,
)]
pub struct ClientRegistration<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    pub(crate) oprf_client: voprf::OprfClient<CS::OprfCs>,
    pub(crate) blinded_element: voprf::BlindedElement<CS::OprfCs>,
}

/// The state elements the server holds to record a registration
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::KeGroup as KeGroup>::Pk)]
pub struct ServerRegistration<CS: CipherSuite>(pub(crate) RegistrationUpload<CS>)
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero;

/// The state elements the client holds to perform a login
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound(
        deserialize = "<CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE1Message: \
                       serde::Deserialize<'de>, <CS::KeyExchange as KeyExchange<OprfHash<CS>, \
                       CS::KeGroup>>::KE1State: serde::Deserialize<'de>",
        serialize = "<CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE1Message: \
                     serde::Serialize, <CS::KeyExchange as KeyExchange<OprfHash<CS>, \
                     CS::KeGroup>>::KE1State: serde::Serialize"
    ))
)]
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(
    Debug, Eq, Hash, PartialEq;
    voprf::OprfClient<CS::OprfCs>,
    <CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE1State,
    CredentialRequest<CS>,
)]
pub struct ClientLogin<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    pub(crate) oprf_client: voprf::OprfClient<CS::OprfCs>,
    pub(crate) ke1_state: <CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE1State,
    pub(crate) credential_request: CredentialRequest<CS>,
}

/// The state elements the server holds to record a login
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound(
        deserialize = "<CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE2State: \
                       serde::Deserialize<'de>",
        serialize = "<CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE2State: \
                     serde::Serialize"
    ))
)]
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(
    Debug, Eq, Hash, PartialEq;
    <CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE2State,
)]
pub struct ServerLogin<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    ke2_state: <CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE2State,
}

////////////////////////////////
// High-level Implementations //
// ========================== //
////////////////////////////////

// Server Setup
// ============

impl<CS: CipherSuite> ServerSetup<CS, PrivateKey<CS::KeGroup>>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Generate a new instance of server setup
    pub fn new<R: CryptoRng + RngCore>(rng: &mut R) -> Self {
        let keypair = KeyPair::generate_random::<CS::OprfCs, _>(rng);
        Self::new_with_key(rng, keypair)
    }
}

/// Length of [`ServerSetup`] in bytes for serialization.
pub type ServerSetupLen<CS: CipherSuite, S: SecretKey<CS::KeGroup>> =
    Sum<Sum<OutputSize<OprfHash<CS>>, S::Len>, <CS::KeGroup as KeGroup>::SkLen>;

impl<CS: CipherSuite, S: SecretKey<CS::KeGroup>> ServerSetup<CS, S>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Create [`ServerSetup`] with the given keypair
    ///
    /// This function should not be used to restore a previously-existing
    /// instance of [`ServerSetup`]. Instead, use [`ServerSetup::serialize`] and
    /// [`ServerSetup::deserialize`] for this purpose.
    pub fn new_with_key<R: CryptoRng + RngCore>(
        rng: &mut R,
        keypair: KeyPair<CS::KeGroup, S>,
    ) -> Self {
        let mut oprf_seed = GenericArray::default();
        rng.fill_bytes(&mut oprf_seed);

        Self {
            oprf_seed,
            keypair,
            fake_keypair: KeyPair::<CS::KeGroup>::generate_random::<CS::OprfCs, _>(rng),
        }
    }

    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, ServerSetupLen<CS, S>>
    where
        // ServerSetup: Hash + KeSk + KeSk
        OutputSize<OprfHash<CS>>: Add<S::Len>,
        Sum<OutputSize<OprfHash<CS>>, S::Len>:
            ArrayLength<u8> + Add<<CS::KeGroup as KeGroup>::SkLen>,
        ServerSetupLen<CS, S>: ArrayLength<u8>,
    {
        self.oprf_seed
            .clone()
            .concat(self.keypair.private().serialize())
            .concat(self.fake_keypair.private().serialize())
    }

    /// Deserialization from bytes
    pub fn deserialize(input: &[u8]) -> Result<Self, ProtocolError<S::Error>> {
        let seed_len = OutputSize::<OprfHash<CS>>::USIZE;
        let key_len = <CS::KeGroup as KeGroup>::SkLen::USIZE;
        let checked_slice = check_slice_size(input, seed_len + key_len + key_len, "server_setup")?;

        Ok(Self {
            oprf_seed: GenericArray::clone_from_slice(&checked_slice[..seed_len]),
            keypair: KeyPair::from_private_key_slice(&checked_slice[seed_len..seed_len + key_len])?,
            fake_keypair: KeyPair::from_private_key_slice(&checked_slice[seed_len + key_len..])
                .map_err(ProtocolError::into_custom)?,
        })
    }

    /// Returns the keypair
    pub fn keypair(&self) -> &KeyPair<CS::KeGroup, S> {
        &self.keypair
    }
}

// Registration
// ============

pub(crate) type ClientRegistrationLen<CS: CipherSuite> =
    Sum<<OprfGroup<CS> as Group>::ScalarLen, <OprfGroup<CS> as Group>::ElemLen>;

impl<CS: CipherSuite> ClientRegistration<CS>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, ClientRegistrationLen<CS>>
    where
        // ClientRegistration: KgSk + KgPk
        <OprfGroup<CS> as Group>::ScalarLen: Add<<OprfGroup<CS> as Group>::ElemLen>,
        ClientRegistrationLen<CS>: ArrayLength<u8>,
    {
        self.oprf_client
            .serialize()
            .concat(self.blinded_element.serialize())
    }

    /// Deserialization from bytes
    pub fn deserialize(input: &[u8]) -> Result<Self, ProtocolError> {
        let client_len = <OprfGroup<CS> as Group>::ScalarLen::USIZE;
        let element_len = <OprfGroup<CS> as Group>::ElemLen::USIZE;
        let checked_slice =
            check_slice_size(input, client_len + element_len, "client_registration")?;

        Ok(Self {
            oprf_client: voprf::OprfClient::deserialize(&checked_slice[..client_len])?,
            blinded_element: voprf::BlindedElement::deserialize(&checked_slice[client_len..])?,
        })
    }

    /// Only used for testing zeroize
    #[cfg(test)]
    pub(crate) fn to_vec(&self) -> std::vec::Vec<u8> {
        [
            self.oprf_client.serialize().to_vec(),
            self.blinded_element.serialize().to_vec(),
        ]
        .concat()
    }

    /// Returns an initial "blinded" request to send to the server, as well as a
    /// [`ClientRegistration`]
    pub fn start<R: RngCore + CryptoRng>(
        blinding_factor_rng: &mut R,
        password: &[u8],
    ) -> Result<ClientRegistrationStartResult<CS>, ProtocolError> {
        let blind_result = blind::<CS, _>(blinding_factor_rng, password)?;

        Ok(ClientRegistrationStartResult {
            message: RegistrationRequest {
                blinded_element: blind_result.message.clone(),
            },
            state: Self {
                oprf_client: blind_result.state,
                blinded_element: blind_result.message,
            },
        })
    }

    /// "Unblinds" the server's answer and returns a final message containing
    /// cryptographic identifiers, to be sent to the server on setup
    /// finalization
    pub fn finish<R: CryptoRng + RngCore>(
        self,
        rng: &mut R,
        password: &[u8],
        registration_response: RegistrationResponse<CS>,
        params: ClientRegistrationFinishParameters<CS>,
    ) -> Result<ClientRegistrationFinishResult<CS>, ProtocolError> {
        // Check for reflected value from server and halt if detected
        if self
            .blinded_element
            .value()
            .ct_eq(&registration_response.evaluation_element.value())
            .into()
        {
            return Err(ProtocolError::ReflectedValueError);
        }

        #[cfg_attr(not(test), allow(unused_variables))]
        let (randomized_pwd, randomized_pwd_hasher) = get_password_derived_key::<CS>(
            password,
            self.oprf_client.clone(),
            registration_response.evaluation_element,
            params.ksf,
        )?;

        let mut masking_key = Output::<OprfHash<CS>>::default();
        randomized_pwd_hasher
            .expand(STR_MASKING_KEY, &mut masking_key)
            .map_err(|_| InternalError::HkdfError)?;

        let result = Envelope::<CS>::seal(
            rng,
            randomized_pwd_hasher,
            &registration_response.server_s_pk,
            params.identifiers,
        )?;

        Ok(ClientRegistrationFinishResult {
            message: RegistrationUpload {
                envelope: result.0,
                masking_key,
                client_s_pk: result.1,
            },
            export_key: result.2,
            server_s_pk: registration_response.server_s_pk,
            #[cfg(test)]
            state: self,
            #[cfg(test)]
            auth_key: result.3,
            #[cfg(test)]
            randomized_pwd,
        })
    }
}

/// Length of [`ServerRegistration`] in bytes for serialization.
pub type ServerRegistrationLen<CS> = RegistrationUploadLen<CS>;

impl<CS: CipherSuite> ServerRegistration<CS>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, ServerRegistrationLen<CS>>
    where
        // Envelope: Nonce + Hash
        NonceLen: Add<OutputSize<OprfHash<CS>>>,
        EnvelopeLen<CS>: ArrayLength<u8>,
        // RegistrationUpload: (KePk + Hash) + Envelope
        <CS::KeGroup as KeGroup>::PkLen: Add<OutputSize<OprfHash<CS>>>,
        Sum<<CS::KeGroup as KeGroup>::PkLen, OutputSize<OprfHash<CS>>>:
            ArrayLength<u8> + Add<EnvelopeLen<CS>>,
        RegistrationUploadLen<CS>: ArrayLength<u8>,
        // ServerRegistration = RegistrationUpload
    {
        self.0.serialize()
    }

    /// Deserialization from bytes
    pub fn deserialize(input: &[u8]) -> Result<Self, ProtocolError> {
        Ok(Self(RegistrationUpload::deserialize(input)?))
    }

    /// From the client's "blinded" password, returns a response to be sent back
    /// to the client, as well as a [`ServerRegistration`]
    pub fn start<S: SecretKey<CS::KeGroup>>(
        server_setup: &ServerSetup<CS, S>,
        message: RegistrationRequest<CS>,
        credential_identifier: &[u8],
    ) -> Result<ServerRegistrationStartResult<CS>, ProtocolError> {
        let oprf_key =
            oprf_key_from_seed::<CS::OprfCs>(&server_setup.oprf_seed, credential_identifier)?;

        let server = voprf::OprfServer::new_with_key(&oprf_key)?;
        let evaluation_element = server.blind_evaluate(&message.blinded_element);

        Ok(ServerRegistrationStartResult {
            message: RegistrationResponse {
                evaluation_element,
                server_s_pk: server_setup.keypair.public().clone(),
            },
            #[cfg(test)]
            oprf_key,
        })
    }

    /// From the client's cryptographic identifiers, fully populates and returns
    /// a [`ServerRegistration`]
    pub fn finish(message: RegistrationUpload<CS>) -> Self {
        Self(message)
    }

    // Creates a dummy instance used for faking a [CredentialResponse]
    pub(crate) fn dummy<R: RngCore + CryptoRng, S: SecretKey<CS::KeGroup>>(
        rng: &mut R,
        server_setup: &ServerSetup<CS, S>,
    ) -> Self {
        Self(RegistrationUpload::dummy(rng, server_setup))
    }
}

// Login
// =====

pub(crate) type ClientLoginLen<CS: CipherSuite> =
    Sum<Sum<<OprfGroup<CS> as Group>::ScalarLen, CredentialRequestLen<CS>>, Ke1StateLen<CS>>;

impl<CS: CipherSuite> ClientLogin<CS>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, ClientLoginLen<CS>>
    where
        // CredentialRequest: KgPk + Ke1Message
        <OprfGroup<CS> as Group>::ElemLen: Add<Ke1MessageLen<CS>>,
        CredentialRequestLen<CS>: ArrayLength<u8>,
        // ClientLogin: KgSk + CredentialRequest + Ke1State
        <OprfGroup<CS> as Group>::ScalarLen: Add<CredentialRequestLen<CS>>,
        Sum<<OprfGroup<CS> as Group>::ScalarLen, CredentialRequestLen<CS>>:
            ArrayLength<u8> + Add<Ke1StateLen<CS>>,
        ClientLoginLen<CS>: ArrayLength<u8>,
    {
        self.oprf_client
            .serialize()
            .concat(self.credential_request.serialize())
            .concat(self.ke1_state.serialize())
    }

    /// Deserialization from bytes
    pub fn deserialize(input: &[u8]) -> Result<Self, ProtocolError> {
        let client_len = <OprfGroup<CS> as Group>::ScalarLen::USIZE;
        let request_len = <OprfGroup<CS> as Group>::ElemLen::USIZE + Ke1MessageLen::<CS>::USIZE;
        let state_len = Ke1StateLen::<CS>::USIZE;
        let checked_slice =
            check_slice_size(input, client_len + request_len + state_len, "client_login")?;

        let ke1_state =
            <CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE1State::deserialize(
                &checked_slice[client_len + request_len..],
            )?;
        Ok(Self {
            oprf_client: voprf::OprfClient::deserialize(&checked_slice[..client_len])?,
            credential_request: CredentialRequest::deserialize(
                &checked_slice[client_len..client_len + request_len],
            )?,
            ke1_state,
        })
    }
}

impl<CS: CipherSuite> ClientLogin<CS>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Returns an initial "blinded" password request to send to the server, as
    /// well as a [`ClientLogin`]
    pub fn start<R: RngCore + CryptoRng>(
        rng: &mut R,
        password: &[u8],
    ) -> Result<ClientLoginStartResult<CS>, ProtocolError> {
        let blind_result = blind::<CS, _>(rng, password)?;
        let (ke1_state, ke1_message) = CS::KeyExchange::generate_ke1::<CS::OprfCs, _>(rng)?;

        let credential_request = CredentialRequest {
            blinded_element: blind_result.message,
            ke1_message,
        };

        Ok(ClientLoginStartResult {
            message: credential_request.clone(),
            state: Self {
                oprf_client: blind_result.state,
                ke1_state,
                credential_request,
            },
        })
    }

    /// "Unblinds" the server's answer and returns the opened assets from the
    /// server
    pub fn finish(
        self,
        password: &[u8],
        credential_response: CredentialResponse<CS>,
        params: ClientLoginFinishParameters<CS>,
    ) -> Result<ClientLoginFinishResult<CS>, ProtocolError>
    where
        // MaskedResponse: (Nonce + Hash) + KePk
        NonceLen: Add<OutputSize<OprfHash<CS>>>,
        Sum<NonceLen, OutputSize<OprfHash<CS>>>:
            ArrayLength<u8> + Add<<CS::KeGroup as KeGroup>::PkLen>,
        MaskedResponseLen<CS>: ArrayLength<u8>,
    {
        // Check if beta value from server is equal to alpha value from client
        if self
            .credential_request
            .blinded_element
            .value()
            .ct_eq(&credential_response.evaluation_element.value())
            .into()
        {
            return Err(ProtocolError::ReflectedValueError);
        }

        let (_, randomized_pwd_hasher) = get_password_derived_key::<CS>(
            password,
            self.oprf_client.clone(),
            credential_response.evaluation_element.clone(),
            params.ksf,
        )?;

        let mut masking_key = Output::<OprfHash<CS>>::default();
        randomized_pwd_hasher
            .expand(STR_MASKING_KEY, &mut masking_key)
            .map_err(|_| InternalError::HkdfError)?;

        let (server_s_pk, envelope) = unmask_response::<CS>(
            &masking_key,
            &credential_response.masking_nonce,
            &credential_response.masked_response,
        )
        .map_err(|e| match e {
            ProtocolError::SerializationError => ProtocolError::InvalidLoginError,
            err => err,
        })?;

        let opened_envelope = envelope
            .open(
                randomized_pwd_hasher,
                server_s_pk.clone(),
                params.identifiers,
            )
            .map_err(|e| match e {
                ProtocolError::LibraryError(InternalError::SealOpenHmacError) => {
                    ProtocolError::InvalidLoginError
                }
                err => err,
            })?;

        let beta = OprfGroup::<CS>::serialize_elem(credential_response.evaluation_element.value());
        let credential_response_component = CredentialResponse::<CS>::serialize_without_ke(
            &beta,
            &credential_response.masking_nonce,
            &credential_response.masked_response,
        );

        let blinded_element =
            OprfGroup::<CS>::serialize_elem(self.credential_request.blinded_element.value());
        let ke1_message = self.credential_request.ke1_message.serialize();
        let serialized_credential_request =
            CredentialRequest::<CS>::serialize_iter(&blinded_element, &ke1_message);

        let result = CS::KeyExchange::generate_ke3(
            credential_response_component,
            credential_response.ke2_message,
            &self.ke1_state,
            serialized_credential_request,
            server_s_pk.clone(),
            opened_envelope.client_static_keypair.private().clone(),
            opened_envelope.id_u.iter(),
            opened_envelope.id_s.iter(),
            params.context.unwrap_or(&[]),
        )?;

        Ok(ClientLoginFinishResult {
            message: CredentialFinalization {
                ke3_message: result.1,
            },
            session_key: result.0,
            export_key: opened_envelope.export_key,
            server_s_pk,
            #[cfg(test)]
            state: self,
            #[cfg(test)]
            handshake_secret: result.2,
            #[cfg(test)]
            client_mac_key: result.3,
        })
    }
}

impl<CS: CipherSuite> ServerLogin<CS>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Serialization into bytes
    pub fn serialize(&self) -> GenericArray<u8, Ke2StateLen<CS>> {
        self.ke2_state.serialize()
    }

    /// Deserialization from bytes
    pub fn deserialize(bytes: &[u8]) -> Result<Self, ProtocolError> {
        Ok(Self {
            ke2_state:
                <CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE2State::deserialize(
                    bytes,
                )?,
        })
    }

    /// From the client's "blinded" password, returns a challenge to be sent
    /// back to the client, as well as a [`ServerLogin`]
    pub fn start<R: RngCore + CryptoRng, S: SecretKey<CS::KeGroup>>(
        rng: &mut R,
        server_setup: &ServerSetup<CS, S>,
        password_file: Option<ServerRegistration<CS>>,
        credential_request: CredentialRequest<CS>,
        credential_identifier: &[u8],
        ServerLoginStartParameters {
            context,
            identifiers,
        }: ServerLoginStartParameters,
    ) -> Result<ServerLoginStartResult<CS>, ProtocolError<S::Error>>
    where
        // MaskedResponse: (Nonce + Hash) + KePk
        NonceLen: Add<OutputSize<OprfHash<CS>>>,
        Sum<NonceLen, OutputSize<OprfHash<CS>>>:
            ArrayLength<u8> + Add<<CS::KeGroup as KeGroup>::PkLen>,
        MaskedResponseLen<CS>: ArrayLength<u8>,
    {
        let record = match password_file {
            Some(x) => x,
            None => ServerRegistration::dummy(rng, server_setup),
        };

        let client_s_pk = record.0.client_s_pk.clone();
        let context = context.unwrap_or(&[]);
        let server_s_sk = server_setup.keypair.private();
        let server_s_pk = server_s_sk.public_key()?;

        let mut masking_nonce = GenericArray::<_, NonceLen>::default();
        rng.fill_bytes(&mut masking_nonce);

        let masked_response = mask_response(
            &record.0.masking_key,
            masking_nonce.as_slice(),
            &server_s_pk,
            &record.0.envelope,
        )
        .map_err(ProtocolError::into_custom)?;

        let (id_u, id_s) = bytestrings_from_identifiers::<CS::KeGroup>(
            identifiers,
            client_s_pk.serialize(),
            server_s_pk.serialize(),
        )
        .map_err(ProtocolError::into_custom)?;

        let blinded_element =
            OprfGroup::<CS>::serialize_elem(credential_request.blinded_element.value());
        let ke1_message = credential_request.ke1_message.serialize();
        let credential_request_bytes =
            CredentialRequest::<CS>::serialize_iter(&blinded_element, &ke1_message);

        let oprf_key =
            oprf_key_from_seed::<CS::OprfCs>(&server_setup.oprf_seed, credential_identifier)
                .map_err(ProtocolError::into_custom)?;
        let server = voprf::OprfServer::new_with_key(&oprf_key)
            .map_err(|e| ProtocolError::into_custom(e.into()))?;
        let evaluation_element = server.blind_evaluate(&credential_request.blinded_element);

        let beta = OprfGroup::<CS>::serialize_elem(evaluation_element.value());
        let credential_response_component =
            CredentialResponse::<CS>::serialize_without_ke(&beta, &masking_nonce, &masked_response);

        let result = CS::KeyExchange::generate_ke2::<CS::OprfCs, _, _>(
            rng,
            credential_request_bytes,
            credential_response_component,
            credential_request.ke1_message.clone(),
            client_s_pk,
            server_s_sk.clone(),
            id_u.iter(),
            id_s.iter(),
            context,
        )?;

        let credential_response = CredentialResponse {
            evaluation_element,
            masking_nonce,
            masked_response,
            ke2_message: result.1,
        };

        Ok(ServerLoginStartResult {
            message: credential_response,
            state: Self {
                ke2_state: result.0,
            },
            #[cfg(test)]
            handshake_secret: result.2,
            #[cfg(test)]
            server_mac_key: result.3,
            #[cfg(test)]
            oprf_key: GenericArray::clone_from_slice(&oprf_key),
        })
    }

    /// From the client's second and final message, check the client's
    /// authentication and produce a message transport
    pub fn finish(
        self,
        message: CredentialFinalization<CS>,
    ) -> Result<ServerLoginFinishResult<CS>, ProtocolError> {
        let session_key = <CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::finish_ke(
            message.ke3_message,
            &self.ke2_state,
        )?;

        Ok(ServerLoginFinishResult {
            session_key,
            #[cfg(test)]
            state: self,
        })
    }
}

/////////////////////////
// Convenience Structs //
//==================== //
/////////////////////////

/// Options for specifying custom identifiers
#[derive(Clone, Copy, Debug, Default)]
pub struct Identifiers<'a> {
    /// Client identifier
    pub client: Option<&'a [u8]>,
    /// Server identifier
    pub server: Option<&'a [u8]>,
}

/// Optional parameters for client registration finish
#[derive_where(Clone, Default)]
pub struct ClientRegistrationFinishParameters<'i, 'h, CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Specifying the identifiers idU and idS
    pub identifiers: Identifiers<'i>,
    /// Specifying a configuration for the key stretching function
    pub ksf: Option<&'h CS::Ksf>,
}

impl<'i, 'h, CS: CipherSuite> ClientRegistrationFinishParameters<'i, 'h, CS>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Create a new [`ClientRegistrationFinishParameters`]
    pub fn new(identifiers: Identifiers<'i>, ksf: Option<&'h CS::Ksf>) -> Self {
        Self { identifiers, ksf }
    }
}

/// Contains the fields that are returned by a client registration start
#[derive_where(Clone)]
pub struct ClientRegistrationStartResult<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// The registration request message to be sent to the server
    pub message: RegistrationRequest<CS>,
    /// The client state that must be persisted in order to complete
    /// registration
    pub state: ClientRegistration<CS>,
}

/// Contains the fields that are returned by a client registration finish
#[derive_where(Clone)]
pub struct ClientRegistrationFinishResult<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// The registration upload message to be sent to the server
    pub message: RegistrationUpload<CS>,
    /// The export key output by client registration
    pub export_key: Output<OprfHash<CS>>,
    /// The server's static public key
    pub server_s_pk: PublicKey<CS::KeGroup>,
    /// Instance of the ClientRegistration, only used in tests for checking
    /// zeroize
    #[cfg(test)]
    pub state: ClientRegistration<CS>,
    /// AuthKey, only used in tests
    #[cfg(test)]
    pub auth_key: Output<OprfHash<CS>>,
    /// Password derived key, only used in tests
    #[cfg(test)]
    pub randomized_pwd: Output<OprfHash<CS>>,
}

/// Contains the fields that are returned by a server registration start. Note
/// that there is no state output in this step
#[derive_where(Clone)]
pub struct ServerRegistrationStartResult<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// The registration resposne message to send to the client
    pub message: RegistrationResponse<CS>,
    /// OPRF key, only used in tests
    #[cfg(test)]
    pub oprf_key: GenericArray<u8, <OprfGroup<CS> as Group>::ScalarLen>,
}

/// Contains the fields that are returned by a client login start
#[derive_where(Clone)]
pub struct ClientLoginStartResult<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// The message to send to the server to begin the login protocol
    pub message: CredentialRequest<CS>,
    /// The state that the client must keep in order to complete the protocol
    pub state: ClientLogin<CS>,
}

/// Optional parameters for client login finish
#[derive_where(Clone, Default)]
pub struct ClientLoginFinishParameters<'c, 'i, 'h, CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Specifying a context field that the server must agree on
    pub context: Option<&'c [u8]>,
    /// Specifying a user identifier and server identifier that will be matched
    /// against the server
    pub identifiers: Identifiers<'i>,
    /// Specifying a configuration for the key stretching hash
    pub ksf: Option<&'h CS::Ksf>,
}

impl<'c, 'i, 'h, CS: CipherSuite> ClientLoginFinishParameters<'c, 'i, 'h, CS>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// Create a new [`ClientLoginFinishParameters`]
    pub fn new(
        context: Option<&'c [u8]>,
        identifiers: Identifiers<'i>,
        ksf: Option<&'h CS::Ksf>,
    ) -> Self {
        Self {
            context,
            identifiers,
            ksf,
        }
    }
}

/// Contains the fields that are returned by a client login finish
#[derive_where(Clone)]
pub struct ClientLoginFinishResult<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// The message to send to the server to complete the protocol
    pub message: CredentialFinalization<CS>,
    /// The session key
    pub session_key: Output<OprfHash<CS>>,
    /// The client-side export key
    pub export_key: Output<OprfHash<CS>>,
    /// The server's static public key
    pub server_s_pk: PublicKey<CS::KeGroup>,
    /// Instance of the ClientLogin, only used in tests for checking zeroize
    #[cfg(test)]
    pub state: ClientLogin<CS>,
    /// Handshake secret, only used in tests
    #[cfg(test)]
    pub handshake_secret: Output<OprfHash<CS>>,
    /// Client MAC key, only used in tests
    #[cfg(test)]
    pub client_mac_key: Output<OprfHash<CS>>,
}

/// Contains the fields that are returned by a server login finish
#[derive_where(Clone)]
#[cfg_attr(not(test), derive_where(Debug))]
#[cfg_attr(test, derive_where(Debug; ServerLogin<CS>))]
pub struct ServerLoginFinishResult<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// The session key between client and server
    pub session_key: Output<OprfHash<CS>>,
    /// Instance of the ClientRegistration, only used in tests for checking
    /// zeroize
    #[cfg(test)]
    pub state: ServerLogin<CS>,
}

/// Optional parameters for server login start
#[derive(Clone, Debug, Default)]
pub struct ServerLoginStartParameters<'c, 'i> {
    /// Specifying a context field that the client must agree on
    pub context: Option<&'c [u8]>,
    /// Specifying a user identifier and server identifier that will be matched
    /// against the client
    pub identifiers: Identifiers<'i>,
}

/// Contains the fields that are returned by a server login start
#[derive_where(Clone)]
#[derive_where(
    Debug;
    voprf::EvaluationElement<CS::OprfCs>,
    <CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE2Message,
    <CS::KeyExchange as KeyExchange<OprfHash<CS>, CS::KeGroup>>::KE2State,
)]
pub struct ServerLoginStartResult<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    /// The message to send back to the client
    pub message: CredentialResponse<CS>,
    /// The state that the server must keep in order to finish the protocl
    pub state: ServerLogin<CS>,
    /// Handshake secret, only used in tests
    #[cfg(test)]
    pub handshake_secret: Output<OprfHash<CS>>,
    /// Server MAC key, only used in tests
    #[cfg(test)]
    pub server_mac_key: Output<OprfHash<CS>>,
    /// OPRF key, only used in tests
    #[cfg(test)]
    pub oprf_key: GenericArray<u8, <OprfGroup<CS> as Group>::ScalarLen>,
}

////////////////////////////////////////////////
// Helper functions and Trait Implementations //
// ========================================== //
////////////////////////////////////////////////

// Helper functions
#[allow(clippy::type_complexity)]
fn get_password_derived_key<CS: CipherSuite>(
    input: &[u8],
    oprf_client: voprf::OprfClient<CS::OprfCs>,
    evaluation_element: voprf::EvaluationElement<CS::OprfCs>,
    ksf: Option<&CS::Ksf>,
) -> Result<(Output<OprfHash<CS>>, Hkdf<OprfHash<CS>>), ProtocolError>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    let oprf_output = oprf_client.finalize(input, &evaluation_element)?;

    let hardened_output = if let Some(ksf) = ksf {
        ksf.hash(oprf_output.clone())
    } else {
        CS::Ksf::default().hash(oprf_output.clone())
    }
    .map_err(ProtocolError::from)?;

    let mut hkdf = HkdfExtract::<OprfHash<CS>>::new(None);
    hkdf.input_ikm(&oprf_output);
    hkdf.input_ikm(&hardened_output);
    Ok(hkdf.finalize())
}

fn oprf_key_from_seed<CS: voprf::CipherSuite>(
    oprf_seed: &Output<CS::Hash>,
    credential_identifier: &[u8],
) -> Result<GenericArray<u8, <CS::Group as Group>::ScalarLen>, ProtocolError>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    CS::Hash: Hash,
    <CS::Hash as CoreProxy>::Core: ProxyHash,
    <<CS::Hash as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<CS::Hash as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    let mut ikm = GenericArray::<_, <CS::Group as Group>::ScalarLen>::default();
    Hkdf::<CS::Hash>::from_prk(oprf_seed)
        .ok()
        .and_then(|hkdf| {
            hkdf.expand_multi_info(&[credential_identifier, STR_OPRF_KEY], &mut ikm)
                .ok()
        })
        .ok_or(InternalError::HkdfError)?;

    Ok(CS::Group::serialize_scalar(voprf::derive_key::<CS>(
        ikm.as_slice(),
        &GenericArray::from(*STR_OPAQUE_DERIVE_KEY_PAIR),
        voprf::Mode::Oprf,
    )?))
}

#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
#[derive_where(Clone)]
#[derive_where(Debug, Eq, Hash, PartialEq)]
pub(crate) struct MaskedResponse<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    pub(crate) nonce: GenericArray<u8, NonceLen>,
    pub(crate) hash: Output<OprfHash<CS>>,
    pub(crate) pk: GenericArray<u8, <CS::KeGroup as KeGroup>::PkLen>,
}

pub(crate) type MaskedResponseLen<CS: CipherSuite> =
    Sum<Sum<NonceLen, OutputSize<OprfHash<CS>>>, <CS::KeGroup as KeGroup>::PkLen>;

impl<CS: CipherSuite> MaskedResponse<CS>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    pub(crate) fn serialize(&self) -> GenericArray<u8, MaskedResponseLen<CS>>
    where
        // MaskedResponse: (Nonce + Hash) + KePk
        NonceLen: Add<OutputSize<OprfHash<CS>>>,
        Sum<NonceLen, OutputSize<OprfHash<CS>>>:
            ArrayLength<u8> + Add<<CS::KeGroup as KeGroup>::PkLen>,
        MaskedResponseLen<CS>: ArrayLength<u8>,
    {
        self.nonce.concat(self.hash.clone()).concat(self.pk.clone())
    }

    pub(crate) fn deserialize(bytes: &[u8]) -> Self {
        let nonce = NonceLen::USIZE;
        let hash = nonce + OutputSize::<OprfHash<CS>>::USIZE;
        let pk = hash + <CS::KeGroup as KeGroup>::PkLen::USIZE;

        Self {
            nonce: GenericArray::clone_from_slice(&bytes[..nonce]),
            hash: GenericArray::clone_from_slice(&bytes[nonce..hash]),
            pk: GenericArray::clone_from_slice(&bytes[hash..pk]),
        }
    }

    pub(crate) fn iter(&self) -> impl Iterator<Item = &[u8]> {
        [self.nonce.as_slice(), &self.hash, &self.pk].into_iter()
    }
}

fn mask_response<CS: CipherSuite>(
    masking_key: &[u8],
    masking_nonce: &[u8],
    server_s_pk: &PublicKey<CS::KeGroup>,
    envelope: &Envelope<CS>,
) -> Result<MaskedResponse<CS>, ProtocolError>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
    // MaskedResponse: (Nonce + Hash) + KePk
    NonceLen: Add<OutputSize<OprfHash<CS>>>,
    Sum<NonceLen, OutputSize<OprfHash<CS>>>: ArrayLength<u8> + Add<<CS::KeGroup as KeGroup>::PkLen>,
    MaskedResponseLen<CS>: ArrayLength<u8>,
{
    let mut xor_pad = GenericArray::<_, MaskedResponseLen<CS>>::default();

    Hkdf::<OprfHash<CS>>::from_prk(masking_key)
        .map_err(|_| InternalError::HkdfError)?
        .expand_multi_info(&[masking_nonce, STR_CREDENTIAL_RESPONSE_PAD], &mut xor_pad)
        .map_err(|_| InternalError::HkdfError)?;

    for (x1, x2) in xor_pad.iter_mut().zip(
        server_s_pk
            .serialize()
            .as_slice()
            .iter()
            .chain(envelope.serialize().iter()),
    ) {
        *x1 ^= x2
    }

    Ok(MaskedResponse::deserialize(&xor_pad))
}

fn unmask_response<CS: CipherSuite>(
    masking_key: &[u8],
    masking_nonce: &[u8],
    masked_response: &MaskedResponse<CS>,
) -> Result<(PublicKey<CS::KeGroup>, Envelope<CS>), ProtocolError>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
    // MaskedResponse: (Nonce + Hash) + KePk
    NonceLen: Add<OutputSize<OprfHash<CS>>>,
    Sum<NonceLen, OutputSize<OprfHash<CS>>>: ArrayLength<u8> + Add<<CS::KeGroup as KeGroup>::PkLen>,
    MaskedResponseLen<CS>: ArrayLength<u8>,
{
    let mut xor_pad = GenericArray::<_, MaskedResponseLen<CS>>::default();

    Hkdf::<OprfHash<CS>>::from_prk(masking_key)
        .map_err(|_| InternalError::HkdfError)?
        .expand_multi_info(&[masking_nonce, STR_CREDENTIAL_RESPONSE_PAD], &mut xor_pad)
        .map_err(|_| InternalError::HkdfError)?;

    for (x1, x2) in xor_pad.iter_mut().zip(masked_response.iter().flatten()) {
        *x1 ^= x2
    }

    let key_len = <CS::KeGroup as KeGroup>::PkLen::USIZE;
    let server_s_pk = PublicKey::deserialize(&xor_pad[..key_len])
        .map_err(|_| ProtocolError::SerializationError)?;
    let envelope = Envelope::deserialize(&xor_pad[key_len..])?;

    Ok((server_s_pk, envelope))
}

#[allow(clippy::type_complexity)]
pub(crate) fn bytestrings_from_identifiers<KG: KeGroup>(
    ids: Identifiers,
    client_s_pk: GenericArray<u8, KG::PkLen>,
    server_s_pk: GenericArray<u8, KG::PkLen>,
) -> Result<(Input<U2, KG::PkLen>, Input<U2, KG::PkLen>), ProtocolError> {
    let client_identity = if let Some(client) = ids.client {
        Input::<U2, _>::from(client)?
    } else {
        Input::<U2, _>::from_owned(client_s_pk)?
    };
    let server_identity = if let Some(server) = ids.server {
        Input::<U2, _>::from(server)?
    } else {
        Input::<U2, _>::from_owned(server_s_pk)?
    };

    Ok((client_identity, server_identity))
}

/// Internal function for computing the blind result by calling the voprf
/// library. Note that for tests, we use the deterministic blinding in order to
/// be able to set the blinding factor directly from the passed-in rng.
fn blind<CS: CipherSuite, R: RngCore + CryptoRng>(
    rng: &mut R,
    password: &[u8],
) -> Result<voprf::OprfClientBlindResult<CS::OprfCs>, voprf::Error>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    #[cfg(not(test))]
    let result = voprf::OprfClient::blind(password, rng)?;

    #[cfg(test)]
    let result = {
        let mut blind_bytes = GenericArray::<_, <OprfGroup<CS> as Group>::ScalarLen>::default();
        let blind = loop {
            rng.fill_bytes(&mut blind_bytes);
            if let Ok(scalar) = <OprfGroup<CS> as Group>::deserialize_scalar(&blind_bytes) {
                break scalar;
            }
        };
        voprf::OprfClient::deterministic_blind_unchecked(password, blind)?
    };

    Ok(result)
}
