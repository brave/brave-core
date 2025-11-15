// -*- mode: rust; -*-
//
// This file is part of schnorrkel.
// Copyright (c) 2019 Web 3 Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Jeff Burdges <jeff@web3.foundation>

//! Encryption using schnorrkel keys

/*
Riad Wahby writes:

As luck would have it, Degabriele, Lehmann, Paterson, Smart, and
Strefler (CT-RSA '12, https://eprint.iacr.org/2011/615) show that
ECIES and EC-Schnorr signatures are secure when instantiated with
the same secret key, in the random oracle model and assuming that
gap-DH and gap-DLP are hard (see Theorem 2).

It seems like this proof could be extended to EdDSA and HPKE with
DHKEM---but I haven't done more than glance! so this should be
regarded as a pointer, not a recommendation.
*/

// use rand_core::{RngCore,CryptoRng};

use aead::{KeyInit, KeySizeUser, generic_array::{GenericArray}};

use curve25519_dalek::digest::generic_array::typenum::{U32};

use curve25519_dalek::{
    ristretto::{CompressedRistretto}, // RistrettoPoint
};

use super::{SecretKey,PublicKey,Keypair,SignatureResult};
use crate::context::SigningTranscript;

use crate::cert::AdaptorCertPublic;


fn make_aead<T,AEAD>(mut t: T) -> AEAD
where T: SigningTranscript,AEAD: KeyInit
{
    let mut key: GenericArray<u8, <AEAD as KeySizeUser>::KeySize> = Default::default();
    t.challenge_bytes(b"",key.as_mut_slice());
    AEAD::new(&key)
}

impl SecretKey {
    /// Commit the results of a key exchange into a transcript
    #[inline(always)]
    pub(crate) fn raw_key_exchange(&self, public: &PublicKey) -> CompressedRistretto {
        (self.key * public.as_point()).compress()
    }

    /// Commit the results of a raw key exchange into a transcript
    pub fn commit_raw_key_exchange<T>(&self, t: &mut T, ctx: &'static [u8], public: &PublicKey)
    where T: SigningTranscript
    {
        let p = self.raw_key_exchange(public);
        t.commit_point(ctx, &p);
    }

    /// An AEAD from a key exchange with the specified public key.
    ///
    /// Requires the AEAD have a 32 byte public key and does not support a context.
    pub fn aead32_unauthenticated<AEAD>(&self, public: &PublicKey) -> AEAD
    where AEAD: KeyInit<KeySize=U32>
    {
        let mut key: GenericArray<u8, <AEAD as KeySizeUser>::KeySize> = Default::default();
        key.clone_from_slice( self.raw_key_exchange(public).as_bytes() );
        AEAD::new(&key)
    }
}

impl PublicKey {
    /// Initialize an AEAD to the public key `self` using an ephemeral key exchange.
    ///
    /// Returns the ephemeral public key and AEAD.
    pub fn init_aead_unauthenticated<AEAD: KeyInit>(&self, ctx: &[u8]) -> (CompressedRistretto,AEAD)
    {
        let ephemeral = Keypair::generate();
        let aead = ephemeral.aead_unauthenticated(ctx,self);
        (ephemeral.public.into_compressed(), aead)
    }

    /// Initialize an AEAD to the public key `self` using an ephemeral key exchange.
    ///
    /// Returns the ephemeral public key and AEAD.
    /// Requires the AEAD have a 32 byte public key and does not support a context.
    pub fn init_aead32_unauthenticated<AEAD>(&self) -> (CompressedRistretto,AEAD)
    where AEAD: KeyInit<KeySize=U32>
    {
        let secret = SecretKey::generate();
        let aead = secret.aead32_unauthenticated(self);
        (secret.to_public().into_compressed(), aead)
    }
}

impl Keypair {
    /// Commit the results of a key exchange into a transcript
    /// including the public keys in sorted order.
    pub fn commit_key_exchange<T>(&self, t: &mut T, ctx: &'static [u8], public: &PublicKey)
    where T: SigningTranscript
    {
        let mut pks = [self.public.as_compressed(), public.as_compressed()];
        pks.sort_unstable_by_key( |pk| pk.as_bytes() );
        for pk in &pks { t.commit_point(b"pk",pk); }
        self.secret.commit_raw_key_exchange(t,ctx,public);
    }

    /// An AEAD from a key exchange with the specified public key.
    pub fn aead_unauthenticated<AEAD: KeyInit>(&self, ctx: &[u8], public: &PublicKey) -> AEAD {
        let mut t = merlin::Transcript::new(b"KEX");
        t.append_message(b"ctx",ctx);
        self.commit_key_exchange(&mut t,b"kex",public);
        make_aead(t)
    }

    /// Reciever's 2DH AEAD
    pub fn reciever_aead<T,AEAD>(
        &self,
        mut t: T,
        ephemeral_pk: &PublicKey,
        static_pk: &PublicKey,
    ) -> AEAD
    where T: SigningTranscript, AEAD: KeyInit
    {
        self.commit_key_exchange(&mut t,b"epk",ephemeral_pk);
        self.commit_key_exchange(&mut t,b"epk",static_pk);
        make_aead(t)
    }

    /// Sender's 2DH AEAD
    pub fn sender_aead<T,AEAD>(
        &self,
        mut t: T,
        public: &PublicKey,
    ) -> (CompressedRistretto,AEAD)
    where T: SigningTranscript, AEAD: KeyInit
    {
        let key = t.witness_scalar(b"make_esk", &[&self.secret.nonce]);
        let ekey = SecretKey { key, nonce: self.secret.nonce }.to_keypair();
        ekey.commit_key_exchange(&mut t,b"epk",public);
        self.commit_key_exchange(&mut t,b"epk",public);
        (ekey.public.into_compressed(), make_aead(t))
    }

    /// Reciever's AEAD with Adaptor certificate.
    ///
    /// Returns the AEAD constructed from an ephemeral key exchange
    /// with the public key computed form the sender's public key
    /// and their implicit Adaptor certificate.
    pub fn reciever_aead_with_adaptor_cert<T,AEAD>(
        &self,
        t: T,
        cert_public: &AdaptorCertPublic,
        public: &PublicKey,
    ) -> SignatureResult<AEAD>
    where T: SigningTranscript, AEAD: KeyInit
    {
        let epk = public.open_adaptor_cert(t,cert_public) ?;
        Ok(self.aead_unauthenticated(b"",&epk))
    }

    /// Sender's AEAD with Adaptor certificate.
    ///
    /// Along with the AEAD, we return the implicit Adaptor certificate
    /// from which the receiver recreates the ephemeral public key.
    pub fn sender_aead_with_adaptor_cert<T,AEAD>(&self, t: T, public: &PublicKey) -> (AdaptorCertPublic,AEAD)
    where T: SigningTranscript+Clone, AEAD: KeyInit
    {
        let (cert,secret) = self.issue_self_adaptor_cert(t);
        let aead = secret.to_keypair().aead_unauthenticated(b"", public);
        (cert, aead)
    }
}
