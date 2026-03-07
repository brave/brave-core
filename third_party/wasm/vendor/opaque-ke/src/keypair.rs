// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Contains the keypair types that must be supplied for the OPAQUE API

#![allow(unsafe_code)]

use derive_where::derive_where;
use digest::core_api::BlockSizeUser;
use digest::OutputSizeUser;
use generic_array::typenum::{IsLess, IsLessOrEqual, U256};
use generic_array::{ArrayLength, GenericArray};
use rand::{CryptoRng, RngCore};

use crate::errors::{InternalError, ProtocolError};
use crate::key_exchange::group::KeGroup;

/// A Keypair trait with public-private verification
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound(
        deserialize = "S: serde::Deserialize<'de>",
        serialize = "S: serde::Serialize"
    ))
)]
#[derive_where(Clone)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; KG::Pk, S)]
pub struct KeyPair<KG: KeGroup, S: SecretKey<KG> = PrivateKey<KG>> {
    pk: PublicKey<KG>,
    sk: S,
}

impl<KG: KeGroup, S: SecretKey<KG>> KeyPair<KG, S> {
    /// The public key component
    pub fn public(&self) -> &PublicKey<KG> {
        &self.pk
    }

    /// The private key component
    pub fn private(&self) -> &S {
        &self.sk
    }

    /// Obtains a [`KeyPair`] from a slice representing the private key
    pub fn from_private_key_slice(input: &[u8]) -> Result<Self, ProtocolError<S::Error>> {
        Self::from_private_key(S::deserialize(input)?)
    }

    /// Obtains a [`KeyPair`] from a private key
    pub fn from_private_key(private_key: S) -> Result<Self, ProtocolError<S::Error>> {
        let pk = private_key.public_key()?;
        Ok(Self {
            pk,
            sk: private_key,
        })
    }
}

impl<KG: KeGroup> KeyPair<KG> {
    /// Generating a random key pair given a cryptographic rng
    pub(crate) fn generate_random<CS: voprf::CipherSuite, R: RngCore + CryptoRng>(
        rng: &mut R,
    ) -> Self
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let mut scalar_bytes = GenericArray::<_, <KG as KeGroup>::SkLen>::default();
        rng.fill_bytes(&mut scalar_bytes);
        let sk =
            KG::derive_auth_keypair::<CS>(GenericArray::clone_from_slice(&scalar_bytes)).unwrap();
        let pk = KG::public_key(sk);
        Self {
            pk: PublicKey(pk),
            sk: PrivateKey(sk),
        }
    }
}

#[cfg(test)]
impl<KG: KeGroup> KeyPair<KG>
where
    KG::Pk: std::fmt::Debug,
    KG::Sk: std::fmt::Debug,
{
    /// Test-only strategy returning a proptest Strategy based on
    /// [`Self::generate_random`]
    fn uniform_keypair_strategy<CS: voprf::CipherSuite>() -> proptest::prelude::BoxedStrategy<Self>
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        use proptest::prelude::*;
        use rand::rngs::StdRng;
        use rand::SeedableRng;

        // The no_shrink is because keypairs should be fixed -- shrinking would cause a
        // different keypair to be generated, which appears to not be very useful.
        any::<[u8; 32]>()
            .prop_filter_map("valid random keypair", |seed| {
                let mut rng = StdRng::from_seed(seed);
                Some(Self::generate_random::<CS, _>(&mut rng))
            })
            .no_shrink()
            .boxed()
    }
}

/// Wrapper around a Key to enforce that it's a private one.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; KG::Sk)]
pub struct PrivateKey<KG: KeGroup>(KG::Sk);

/// A trait specifying the requirements for a private key container
pub trait SecretKey<KG: KeGroup>: Clone + Sized {
    /// Custom error type that can be passed down to `InternalError::Custom`
    type Error;
    /// Serialization size in bytes.
    type Len: ArrayLength<u8>;

    /// Diffie-Hellman key exchange implementation
    fn diffie_hellman(
        &self,
        pk: PublicKey<KG>,
    ) -> Result<GenericArray<u8, KG::PkLen>, InternalError<Self::Error>>;

    /// Returns public key from private key
    fn public_key(&self) -> Result<PublicKey<KG>, InternalError<Self::Error>>;

    /// Serialization into bytes
    fn serialize(&self) -> GenericArray<u8, Self::Len>;

    /// Deserialization from bytes
    fn deserialize(input: &[u8]) -> Result<Self, InternalError<Self::Error>>;
}

impl<KG: KeGroup> SecretKey<KG> for PrivateKey<KG> {
    type Error = core::convert::Infallible;
    type Len = KG::SkLen;

    fn diffie_hellman(
        &self,
        pk: PublicKey<KG>,
    ) -> Result<GenericArray<u8, KG::PkLen>, InternalError> {
        Ok(KG::diffie_hellman(pk.0, self.0))
    }

    fn public_key(&self) -> Result<PublicKey<KG>, InternalError> {
        Ok(PublicKey(KG::public_key(self.0)))
    }

    fn serialize(&self) -> GenericArray<u8, Self::Len> {
        KG::serialize_sk(self.0)
    }

    fn deserialize(input: &[u8]) -> Result<Self, InternalError> {
        KG::deserialize_sk(input).map(Self)
    }
}

#[cfg(feature = "serde")]
impl<'de, KG: KeGroup> serde::Deserialize<'de> for PrivateKey<KG> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        use serde::de::Error;

        KG::deserialize_sk(&GenericArray::<_, KG::SkLen>::deserialize(deserializer)?)
            .map(Self)
            .map_err(D::Error::custom)
    }
}

#[cfg(feature = "serde")]
impl<KG: KeGroup> serde::Serialize for PrivateKey<KG> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        KG::serialize_sk(self.0).serialize(serializer)
    }
}

/// Wrapper around a Key to enforce that it's a public one.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; KG::Pk)]
pub struct PublicKey<KG: KeGroup>(KG::Pk);

impl<KG: KeGroup> PublicKey<KG> {
    /// Convert from bytes
    pub fn deserialize(key_bytes: &[u8]) -> Result<Self, InternalError> {
        KG::deserialize_pk(key_bytes).map(Self)
    }

    /// Convert to bytes
    pub fn serialize(&self) -> GenericArray<u8, KG::PkLen> {
        KG::serialize_pk(self.0)
    }
}

#[cfg(feature = "serde")]
impl<'de, KG: KeGroup> serde::Deserialize<'de> for PublicKey<KG> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        use serde::de::Error;

        KG::deserialize_pk(&GenericArray::<_, KG::PkLen>::deserialize(deserializer)?)
            .map(Self)
            .map_err(D::Error::custom)
    }
}

#[cfg(feature = "serde")]
impl<KG: KeGroup> serde::Serialize for PublicKey<KG> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        KG::serialize_pk(self.0).serialize(serializer)
    }
}

#[cfg(test)]
mod tests {
    use rand::rngs::OsRng;

    use super::*;
    use crate::errors::*;
    use crate::util;

    #[test]
    fn test_zeroize_key() {
        fn inner<G: KeGroup>() {
            let mut rng = OsRng;
            let mut key = PrivateKey::<G>(G::random_sk(&mut rng));
            util::test_zeroize_on_drop(&mut key);
        }

        #[cfg(feature = "ristretto255")]
        inner::<crate::Ristretto255>();
        inner::<::p256::NistP256>();
        inner::<::p384::NistP384>();
        inner::<::p521::NistP521>();
    }

    macro_rules! test {
        ($mod:ident, $point:ty) => {
            mod $mod {
                use std::format;

                use proptest::prelude::*;

                use super::*;

                proptest! {
                    #[test]
                    fn pub_from_priv(kp in KeyPair::<$point>::uniform_keypair_strategy::<$point>()) {
                        let pk = kp.public();
                        let sk = kp.private();
                        prop_assert_eq!(&sk.public_key()?, pk);
                    }

                    #[test]
                    fn dh(kp1 in KeyPair::<$point>::uniform_keypair_strategy::<$point>(),
                                      kp2 in KeyPair::<$point>::uniform_keypair_strategy::<$point>()) {

                        let dh1 = kp2.private().diffie_hellman(kp1.public().clone())?;
                        let dh2 = kp1.private().diffie_hellman(kp2.public().clone())?;

                        prop_assert_eq!(dh1, dh2);
                    }

                    #[test]
                    fn private_key_slice(kp in KeyPair::<$point>::uniform_keypair_strategy::<$point>()) {
                        let sk_bytes = kp.private().serialize().to_vec();

                        let kp2 = KeyPair::<$point>::from_private_key_slice(&sk_bytes)?;
                        let kp2_private_bytes = kp2.private().serialize().to_vec();

                        prop_assert_eq!(sk_bytes, kp2_private_bytes);
                    }
                }
            }
        };
    }

    #[cfg(feature = "ristretto255")]
    test!(ristretto, crate::Ristretto255);
    test!(p256, ::p256::NistP256);
    test!(p384, ::p384::NistP384);
    test!(p521, ::p521::NistP521);

    #[test]
    fn remote_key() {
        use rand::rngs::OsRng;

        use crate::{
            CipherSuite, ClientLogin, ClientLoginFinishParameters, ClientLoginFinishResult,
            ClientLoginStartResult, ClientRegistration, ClientRegistrationFinishParameters,
            ClientRegistrationFinishResult, ClientRegistrationStartResult, ServerLogin,
            ServerLoginStartParameters, ServerLoginStartResult, ServerRegistration,
            ServerRegistrationStartResult, ServerSetup,
        };

        struct Default;

        impl CipherSuite for Default {
            #[cfg(feature = "ristretto255")]
            type OprfCs = crate::Ristretto255;
            #[cfg(not(feature = "ristretto255"))]
            type OprfCs = ::p256::NistP256;
            #[cfg(feature = "ristretto255")]
            type KeGroup = crate::Ristretto255;
            #[cfg(not(feature = "ristretto255"))]
            type KeGroup = ::p256::NistP256;
            type KeyExchange = crate::key_exchange::tripledh::TripleDh;
            type Ksf = crate::ksf::Identity;
        }

        type KeCurve = <Default as CipherSuite>::KeGroup;

        #[derive(Clone)]
        struct RemoteKey(PrivateKey<KeCurve>);

        impl SecretKey<KeCurve> for RemoteKey {
            type Error = core::convert::Infallible;
            type Len = <KeCurve as KeGroup>::SkLen;

            fn diffie_hellman(
                &self,
                pk: PublicKey<KeCurve>,
            ) -> Result<GenericArray<u8, <KeCurve as KeGroup>::PkLen>, InternalError<Self::Error>>
            {
                self.0.diffie_hellman(pk)
            }

            fn public_key(&self) -> Result<PublicKey<KeCurve>, InternalError<Self::Error>> {
                self.0.public_key()
            }

            fn serialize(&self) -> GenericArray<u8, Self::Len> {
                self.0.serialize()
            }

            fn deserialize(input: &[u8]) -> Result<Self, InternalError<Self::Error>> {
                PrivateKey::deserialize(input).map(Self)
            }
        }

        const PASSWORD: &str = "password";

        let sk = KeCurve::random_sk(&mut OsRng);
        let sk = RemoteKey(PrivateKey(sk));
        let keypair = KeyPair::from_private_key(sk).unwrap();

        let server_setup = ServerSetup::<Default, RemoteKey>::new_with_key(&mut OsRng, keypair);

        let ClientRegistrationStartResult {
            message,
            state: client,
        } = ClientRegistration::<Default>::start(&mut OsRng, PASSWORD.as_bytes()).unwrap();
        let ServerRegistrationStartResult { message, .. } =
            ServerRegistration::start(&server_setup, message, &[]).unwrap();
        let ClientRegistrationFinishResult { message, .. } = client
            .finish(
                &mut OsRng,
                PASSWORD.as_bytes(),
                message,
                ClientRegistrationFinishParameters::default(),
            )
            .unwrap();
        let file = ServerRegistration::finish(message);

        let ClientLoginStartResult {
            message,
            state: client,
        } = ClientLogin::<Default>::start(&mut OsRng, PASSWORD.as_bytes()).unwrap();
        let ServerLoginStartResult {
            message,
            state: server,
            ..
        } = ServerLogin::start(
            &mut OsRng,
            &server_setup,
            Some(file),
            message,
            &[],
            ServerLoginStartParameters::default(),
        )
        .unwrap();
        let ClientLoginFinishResult { message, .. } = client
            .finish(
                PASSWORD.as_bytes(),
                message,
                ClientLoginFinishParameters::default(),
            )
            .unwrap();
        server.finish(message).unwrap();
    }
}
