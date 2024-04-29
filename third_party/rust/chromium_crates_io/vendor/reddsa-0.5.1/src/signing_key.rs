// -*- mode: rust; -*-
//
// This file is part of reddsa.
// Copyright (c) 2019-2021 Zcash Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Deirdre Connolly <deirdre@zfnd.org>
// - Henry de Valence <hdevalence@hdevalence.ca>

use core::{
    convert::{TryFrom, TryInto},
    marker::PhantomData,
};

use crate::{
    private::SealedScalar, Error, Randomizer, SigType, Signature, SpendAuth, VerificationKey,
};

use group::{ff::PrimeField, GroupEncoding};
use rand_core::{CryptoRng, RngCore};

/// A RedDSA signing key.
#[derive(Copy, Clone, Debug)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[cfg_attr(feature = "serde", serde(try_from = "SerdeHelper"))]
#[cfg_attr(feature = "serde", serde(into = "SerdeHelper"))]
#[cfg_attr(feature = "serde", serde(bound = "T: SigType"))]
pub struct SigningKey<T: SigType> {
    sk: T::Scalar,
    pk: VerificationKey<T>,
}

impl<T: SigType> From<&SigningKey<T>> for VerificationKey<T> {
    fn from(sk: &SigningKey<T>) -> VerificationKey<T> {
        sk.pk
    }
}

impl<T: SigType> From<SigningKey<T>> for [u8; 32] {
    fn from(sk: SigningKey<T>) -> [u8; 32] {
        sk.sk.to_repr().as_ref().try_into().unwrap()
    }
}

impl<T: SigType> TryFrom<[u8; 32]> for SigningKey<T> {
    type Error = Error;

    fn try_from(bytes: [u8; 32]) -> Result<Self, Self::Error> {
        // XXX-jubjub: this should not use CtOption
        let mut repr = <T::Scalar as PrimeField>::Repr::default();
        repr.as_mut().copy_from_slice(&bytes);
        let maybe_sk = T::Scalar::from_repr(repr);
        if maybe_sk.is_some().into() {
            let sk = maybe_sk.unwrap();
            let pk = VerificationKey::from(&sk);
            Ok(SigningKey { sk, pk })
        } else {
            Err(Error::MalformedSigningKey)
        }
    }
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
struct SerdeHelper([u8; 32]);

impl<T: SigType> TryFrom<SerdeHelper> for SigningKey<T> {
    type Error = Error;

    fn try_from(helper: SerdeHelper) -> Result<Self, Self::Error> {
        helper.0.try_into()
    }
}

impl<T: SigType> From<SigningKey<T>> for SerdeHelper {
    fn from(sk: SigningKey<T>) -> Self {
        Self(sk.into())
    }
}

impl<T: SpendAuth> SigningKey<T> {
    /// Randomize this public key with the given `randomizer`.
    pub fn randomize(&self, randomizer: &Randomizer<T>) -> SigningKey<T> {
        let sk = self.sk + randomizer;
        let pk = VerificationKey::from(&sk);
        SigningKey { sk, pk }
    }
}

impl<T: SigType> SigningKey<T> {
    /// Generate a new signing key.
    pub fn new<R: RngCore + CryptoRng>(mut rng: R) -> SigningKey<T> {
        let sk = {
            let mut bytes = [0; 64];
            rng.fill_bytes(&mut bytes);
            T::Scalar::from_bytes_wide(&bytes)
        };
        let pk = VerificationKey::from(&sk);
        SigningKey { sk, pk }
    }

    /// Create a signature of type `T` on `msg` using this `SigningKey`.
    // Similar to signature::Signer but without boxed errors.
    pub fn sign<R: RngCore + CryptoRng>(&self, mut rng: R, msg: &[u8]) -> Signature<T> {
        use crate::HStar;

        // Choose a byte sequence uniformly at random of length
        // (\ell_H + 128)/8 bytes.  For RedJubjub and RedPallas this is
        // (512 + 128)/8 = 80.
        let random_bytes = {
            let mut bytes = [0; 80];
            rng.fill_bytes(&mut bytes);
            bytes
        };

        let nonce = HStar::<T>::default()
            .update(&random_bytes[..])
            .update(&self.pk.bytes.bytes[..]) // XXX ugly
            .update(msg)
            .finalize();

        let r: T::Point = T::basepoint() * nonce;
        let r_bytes: [u8; 32] = r.to_bytes().as_ref().try_into().unwrap();

        let c = HStar::<T>::default()
            .update(&r_bytes[..])
            .update(&self.pk.bytes.bytes[..]) // XXX ugly
            .update(msg)
            .finalize();

        let s = nonce + (c * self.sk);
        let s_bytes = s.to_repr().as_ref().try_into().unwrap();

        Signature {
            r_bytes,
            s_bytes,
            _marker: PhantomData,
        }
    }
}
