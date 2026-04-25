// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

use core::convert::TryFrom;
use core::ops::Add;

use derive_where::derive_where;
use digest::core_api::{BlockSizeUser, CoreProxy};
use digest::{Output, OutputSizeUser};
use generic_array::sequence::Concat;
use generic_array::typenum::{IsLess, IsLessOrEqual, Le, NonZero, Sum, Unsigned, U2, U256, U32};
use generic_array::{ArrayLength, GenericArray};
use hkdf::Hkdf;
use hmac::{Hmac, Mac};
use rand::{CryptoRng, RngCore};
use zeroize::{Zeroize, ZeroizeOnDrop};

use crate::ciphersuite::{CipherSuite, OprfHash};
use crate::errors::utils::check_slice_size;
use crate::errors::{InternalError, ProtocolError};
use crate::hash::{Hash, OutputSize, ProxyHash};
use crate::key_exchange::group::KeGroup;
use crate::keypair::{KeyPair, PublicKey};
use crate::opaque::{bytestrings_from_identifiers, Identifiers};
use crate::serialization::{Input, MacExt};

// Constant string used as salt for HKDF computation
const STR_AUTH_KEY: [u8; 7] = *b"AuthKey";
const STR_EXPORT_KEY: [u8; 9] = *b"ExportKey";
const STR_PRIVATE_KEY: [u8; 10] = *b"PrivateKey";
type NonceLen = U32;

#[cfg_attr(feature = "serde", derive(serde::Deserialize, serde::Serialize))]
#[derive(Clone, Debug, Eq, Hash, Ord, PartialEq, PartialOrd, ZeroizeOnDrop)]
pub(crate) enum InnerEnvelopeMode {
    Zero = 0,
    Internal = 1,
}

impl Zeroize for InnerEnvelopeMode {
    fn zeroize(&mut self) {
        *self = Self::Zero
    }
}

impl TryFrom<u8> for InnerEnvelopeMode {
    type Error = ProtocolError;
    fn try_from(x: u8) -> Result<Self, Self::Error> {
        match x {
            1 => Ok(InnerEnvelopeMode::Internal),
            _ => Err(ProtocolError::SerializationError),
        }
    }
}

/// This struct is an instantiation of the envelope.
///
/// Note that earlier versions of this specification described an implementation
/// of this envelope using an encryption scheme that satisfied random-key
/// robustness.
/// The specification update has simplified this assumption by taking an
/// XOR-based approach without compromising on security, and to avoid the
/// confusion around the implementation of an RKR-secure encryption.
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
#[derive_where(Clone, Debug, Eq, Hash, Ord, PartialEq, PartialOrd, ZeroizeOnDrop)]
pub(crate) struct Envelope<CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    pub(crate) mode: InnerEnvelopeMode,
    nonce: GenericArray<u8, NonceLen>,
    hmac: Output<OprfHash<CS>>,
}

// Note that this struct represents an envelope that has been "opened" with the
// asssociated key. This key is also used to derive the export_key parameter,
// which is technically unrelated to the envelope's encrypted and authenticated
// contents.
pub(crate) struct OpenedEnvelope<'a, CS: CipherSuite>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    pub(crate) client_static_keypair: KeyPair<CS::KeGroup>,
    pub(crate) export_key: Output<OprfHash<CS>>,
    pub(crate) id_u: Input<'a, U2, <CS::KeGroup as KeGroup>::PkLen>,
    pub(crate) id_s: Input<'a, U2, <CS::KeGroup as KeGroup>::PkLen>,
}

pub(crate) struct OpenedInnerEnvelope<D: Hash>
where
    D::Core: ProxyHash,
    <D::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<D::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    pub(crate) export_key: Output<D>,
}

#[cfg(not(test))]
type SealRawResult<CS: CipherSuite> = (Envelope<CS>, Output<OprfHash<CS>>);
#[cfg(test)]
type SealRawResult<CS: CipherSuite> = (Envelope<CS>, Output<OprfHash<CS>>, Output<OprfHash<CS>>);
#[cfg(not(test))]
type SealResult<CS: CipherSuite> = (Envelope<CS>, PublicKey<CS::KeGroup>, Output<OprfHash<CS>>);
#[cfg(test)]
type SealResult<CS: CipherSuite> = (
    Envelope<CS>,
    PublicKey<CS::KeGroup>,
    Output<OprfHash<CS>>,
    Output<OprfHash<CS>>,
);

pub(crate) type EnvelopeLen<CS: CipherSuite> = Sum<NonceLen, OutputSize<OprfHash<CS>>>;

impl<CS: CipherSuite> Envelope<CS>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    #[allow(clippy::type_complexity)]
    pub(crate) fn seal<R: RngCore + CryptoRng>(
        rng: &mut R,
        randomized_pwd_hasher: Hkdf<OprfHash<CS>>,
        server_s_pk: &PublicKey<CS::KeGroup>,
        ids: Identifiers,
    ) -> Result<SealResult<CS>, ProtocolError> {
        let mut nonce = GenericArray::default();
        rng.fill_bytes(&mut nonce);

        let (mode, client_s_pk) = (
            InnerEnvelopeMode::Internal,
            build_inner_envelope_internal::<CS>(randomized_pwd_hasher.clone(), nonce)?,
        );

        let server_s_pk_bytes = server_s_pk.serialize();
        let (id_u, id_s) = bytestrings_from_identifiers::<CS::KeGroup>(
            ids,
            client_s_pk.serialize(),
            server_s_pk_bytes.clone(),
        )?;
        let aad = construct_aad(id_u.iter(), id_s.iter(), &server_s_pk_bytes);

        let result = Self::seal_raw(randomized_pwd_hasher, nonce, aad, mode)?;
        Ok((
            result.0,
            client_s_pk,
            result.1,
            #[cfg(test)]
            result.2,
        ))
    }

    /// Uses a key to convert the plaintext into an envelope, authenticated by
    /// the aad field. Note that a new nonce is sampled for each call to seal.
    #[allow(clippy::type_complexity)]
    pub(crate) fn seal_raw<'a>(
        randomized_pwd_hasher: Hkdf<OprfHash<CS>>,
        nonce: GenericArray<u8, NonceLen>,
        aad: impl Iterator<Item = &'a [u8]>,
        mode: InnerEnvelopeMode,
    ) -> Result<SealRawResult<CS>, InternalError> {
        let mut hmac_key = Output::<OprfHash<CS>>::default();
        let mut export_key = Output::<OprfHash<CS>>::default();

        randomized_pwd_hasher
            .expand_multi_info(&[&nonce, &STR_AUTH_KEY], &mut hmac_key)
            .map_err(|_| InternalError::HkdfError)?;
        randomized_pwd_hasher
            .expand_multi_info(&[&nonce, &STR_EXPORT_KEY], &mut export_key)
            .map_err(|_| InternalError::HkdfError)?;

        let mut hmac = Hmac::<OprfHash<CS>>::new_from_slice(&hmac_key)
            .map_err(|_| InternalError::HmacError)?;
        hmac.update(&nonce);
        hmac.update_iter(aad);

        let hmac_bytes = hmac.finalize().into_bytes();

        Ok((
            Self {
                mode,
                nonce,
                hmac: hmac_bytes,
            },
            export_key,
            #[cfg(test)]
            hmac_key,
        ))
    }

    pub(crate) fn open<'a>(
        &self,
        randomized_pwd_hasher: Hkdf<OprfHash<CS>>,
        server_s_pk: PublicKey<CS::KeGroup>,
        optional_ids: Identifiers<'a>,
    ) -> Result<OpenedEnvelope<'a, CS>, ProtocolError> {
        let client_static_keypair = match self.mode {
            InnerEnvelopeMode::Zero => {
                return Err(InternalError::IncompatibleEnvelopeModeError.into())
            }
            InnerEnvelopeMode::Internal => {
                recover_keys_internal::<CS>(randomized_pwd_hasher.clone(), self.nonce)?
            }
        };

        let server_s_pk_bytes = server_s_pk.serialize();
        let (id_u, id_s) = bytestrings_from_identifiers::<CS::KeGroup>(
            optional_ids,
            client_static_keypair.public().serialize(),
            server_s_pk_bytes.clone(),
        )?;
        let aad = construct_aad(id_u.iter(), id_s.iter(), &server_s_pk_bytes);

        let opened = self.open_raw(randomized_pwd_hasher, aad)?;

        Ok(OpenedEnvelope {
            client_static_keypair,
            export_key: opened.export_key,
            id_u,
            id_s,
        })
    }

    /// Attempts to decrypt the envelope using a key, which is successful only
    /// if the key and aad used to construct the envelope are the same.
    pub(crate) fn open_raw<'a>(
        &self,
        randomized_pwd_hasher: Hkdf<OprfHash<CS>>,
        aad: impl Iterator<Item = &'a [u8]>,
    ) -> Result<OpenedInnerEnvelope<OprfHash<CS>>, InternalError> {
        let mut hmac_key = Output::<OprfHash<CS>>::default();
        let mut export_key = Output::<OprfHash<CS>>::default();

        randomized_pwd_hasher
            .expand(&self.nonce.concat(STR_AUTH_KEY.into()), &mut hmac_key)
            .map_err(|_| InternalError::HkdfError)?;
        randomized_pwd_hasher
            .expand(&self.nonce.concat(STR_EXPORT_KEY.into()), &mut export_key)
            .map_err(|_| InternalError::HkdfError)?;

        let mut hmac = Hmac::<OprfHash<CS>>::new_from_slice(&hmac_key)
            .map_err(|_| InternalError::HmacError)?;
        hmac.update(&self.nonce);
        hmac.update_iter(aad);
        hmac.verify(&self.hmac)
            .map_err(|_| InternalError::SealOpenHmacError)?;

        Ok(OpenedInnerEnvelope { export_key })
    }

    // Creates a dummy envelope object that serializes to the all-zeros byte string
    pub(crate) fn dummy() -> Self {
        Self {
            mode: InnerEnvelopeMode::Zero,
            nonce: GenericArray::default(),
            hmac: GenericArray::default(),
        }
    }

    fn hmac_key_size() -> usize {
        OutputSize::<OprfHash<CS>>::USIZE
    }

    pub(crate) fn len() -> usize {
        OutputSize::<OprfHash<CS>>::USIZE + NonceLen::USIZE
    }

    pub(crate) fn serialize(&self) -> GenericArray<u8, EnvelopeLen<CS>>
    where
        // Envelope: Nonce + Hash
        NonceLen: Add<OutputSize<OprfHash<CS>>>,
        EnvelopeLen<CS>: ArrayLength<u8>,
    {
        self.nonce.concat(self.hmac.clone())
    }

    pub(crate) fn deserialize(bytes: &[u8]) -> Result<Self, ProtocolError> {
        let mode = InnerEnvelopeMode::Internal; // Better way to hard-code this?

        if bytes.len() < NonceLen::USIZE {
            return Err(ProtocolError::SerializationError);
        }
        let nonce = GenericArray::clone_from_slice(&bytes[..NonceLen::USIZE]);

        let remainder = match mode {
            InnerEnvelopeMode::Zero => {
                return Err(InternalError::IncompatibleEnvelopeModeError.into())
            }
            InnerEnvelopeMode::Internal => &bytes[NonceLen::USIZE..],
        };

        let hmac_key_size = Self::hmac_key_size();
        let hmac = check_slice_size(remainder, hmac_key_size, "hmac_key_size")?;

        Ok(Self {
            mode,
            nonce,
            hmac: GenericArray::clone_from_slice(hmac),
        })
    }
}

// Helper functions

fn build_inner_envelope_internal<CS: CipherSuite>(
    randomized_pwd_hasher: Hkdf<OprfHash<CS>>,
    nonce: GenericArray<u8, NonceLen>,
) -> Result<PublicKey<CS::KeGroup>, ProtocolError>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    let mut keypair_seed = GenericArray::<_, <CS::KeGroup as KeGroup>::SkLen>::default();
    randomized_pwd_hasher
        .expand(&nonce.concat(STR_PRIVATE_KEY.into()), &mut keypair_seed)
        .map_err(|_| InternalError::HkdfError)?;
    let client_static_keypair =
        KeyPair::<CS::KeGroup>::from_private_key_slice(&CS::KeGroup::serialize_sk(
            CS::KeGroup::derive_auth_keypair::<CS::OprfCs>(keypair_seed)?,
        ))?;

    Ok(client_static_keypair.public().clone())
}

fn recover_keys_internal<CS: CipherSuite>(
    randomized_pwd_hasher: Hkdf<OprfHash<CS>>,
    nonce: GenericArray<u8, NonceLen>,
) -> Result<KeyPair<CS::KeGroup>, ProtocolError>
where
    <OprfHash<CS> as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<OprfHash<CS> as BlockSizeUser>::BlockSize>,
    OprfHash<CS>: Hash,
    <OprfHash<CS> as CoreProxy>::Core: ProxyHash,
    <<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<<OprfHash<CS> as CoreProxy>::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    let mut keypair_seed = GenericArray::<_, <CS::KeGroup as KeGroup>::SkLen>::default();
    randomized_pwd_hasher
        .expand(&nonce.concat(STR_PRIVATE_KEY.into()), &mut keypair_seed)
        .map_err(|_| InternalError::HkdfError)?;
    let client_static_keypair =
        KeyPair::<CS::KeGroup>::from_private_key_slice(&CS::KeGroup::serialize_sk(
            CS::KeGroup::derive_auth_keypair::<CS::OprfCs>(keypair_seed)?,
        ))?;

    Ok(client_static_keypair)
}

fn construct_aad<'a>(
    id_u: impl Iterator<Item = &'a [u8]>,
    id_s: impl Iterator<Item = &'a [u8]>,
    server_s_pk: &'a [u8],
) -> impl Iterator<Item = &'a [u8]> {
    [server_s_pk].into_iter().chain(id_s).chain(id_u)
}
