// -*- mode: rust; -*-
//
// This file is part of schnorrkel.
// Copyright (c) 2019 Web 3 Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Jeffrey Burdges <jeff@web3.foundation>


//! ### Adaptor signature-based implicit certificate scheme for Ristretto
//!
//! [Implicit certificates](https://en.wikipedia.org/wiki/Implicit_certificate)
//! provide an extremely space efficient public key certificate scheme.
//!
//! As a rule, implicit certificates do not prove possession of the
//! private key.  We thus worry more about fear rogue key attack when
//! using them, but all protocols here should provide strong defenses
//! against then.
//!
//! We implement an implicit certificate scheme based on adaptor
//! signatures as recommended by [1] and [2], which appears useful for
//!  "scriptless script" applications like [3] and [4].
//!
//! We should eventually place this into some more generally usable adaptor
//! signature framework, but doing this safely this requires more work.
//! We have not actually done security arguments for this code yet either,
//! but we expect to find such results in the paymet channel literature [3].
//! We might find arguments around Elliptic curve Qu-Vanstone (ECQV)
//! helpful too [5,6].
//!
//! [1] "Schnorr Signatures for secp256k1"
//!     by Pieter Wuille, Jonas Nick, and Tim Ruffing
//!     https://github.com/sipa/bips/blob/bip-schnorr/bip-schnorr.mediawiki#Adaptor_Signatures
//! [2] Ruben Somsen. "Schnorr signatures, adaptor signatures and statechains"
//!     https://bitcoinedge.org/transcript/telaviv2019/statechains
//! [3] Giulio Malavolta and Pedro Moreno-Sanchez and Clara Schneidewind and Aniket Kate and Matteo Maffei
//!     "Anonymous Multi-Hop Locks for Blockchain Scalability and Interoperability"
//!     https://eprint.iacr.org/2018/472
//! [4] Jonas Nick.  "Scriptless Scripts [Using Adaptor Signatures]"
//!     https://github.com/ElementsProject/scriptless-scripts
//! [5] "Standards for efficient cryptography, SEC 4: Elliptic Curve
//!     Qu-Vanstone Implicit Certificate Scheme (ECQV)".
//!     http://www.secg.org/sec4-1.0.pdf
//! [6] Daniel R. L. Brown, Robert P. Gallant, and Scott A. Vanstone.
//!     "Provably Secure Implicit Certificate Schemes". Financial
//!     Cryptography 2001. Lecture Notes in Computer Science.
//!     Springer Berlin Heidelberg. 2339 (1): 156–165. doi:10.1007/3-540-46088-8_15.
//!     http://www.cacr.math.uwaterloo.ca/techreports/2000/corr2000-55.ps
//
// Found [4] via https://download.wpsoftware.net/bitcoin/wizardry/mw-slides/2018-05-18-l2/slides.pdf via [1]

use curve25519_dalek::constants;
use curve25519_dalek::ristretto::{CompressedRistretto};
use curve25519_dalek::scalar::Scalar;

use super::*;
use crate::context::SigningTranscript;


/// Adaptor Implicit Certificate Secret
///
/// Issuing an Adaptor implicit certificate requires producing
/// this and securely sending it to the certificate holder.
#[derive(Clone, Copy)] // Debug, Eq, PartialEq
pub struct AdaptorCertSecret(pub [u8; 64]);
/// TODO: Serde serialization/deserialization

impl From<AdaptorCertSecret> for AdaptorCertPublic {
    fn from(secret: AdaptorCertSecret) -> AdaptorCertPublic {
        let mut public = AdaptorCertPublic([0u8; 32]);
        public.0.copy_from_slice(&secret.0[0..32]);
        public
    }
}

/// Adaptor Implicit Certificate Public Key Reconstruction Data
///
/// Identifying the public key of, and implicitly verifying, an Adaptor
/// implicit certificate requires this data, which is produced
/// when the certificate holder accepts the implicit certificate.
#[derive(Debug, Clone, Copy, Eq, PartialEq, Hash)]
pub struct AdaptorCertPublic(pub [u8; 32]);
/// TODO: Serde serialization/deserialization

impl AdaptorCertPublic {
    fn derive_e<T: SigningTranscript>(&self, mut t: T) -> Scalar {
        t.challenge_scalar(b"adaptor-e")
    }
}

impl Keypair {
    /// Issue an Adaptor implicit certificate
    ///
    /// Aside from the issuing `Keypair` supplied as `self`, you provide both
    /// (1) a `SigningTranscript` called `t` that incorporates both the context 
    ///     and the certificate requester's identity, and
    /// (2) the `seed_public_key` supplied by the certificate recipient
    ///     in their certificate request.
    /// We return an `AdaptorCertSecret` which the issuer sent to the
    /// certificate requester, ans from which the certificate requester
    /// derives their certified key pair.
    pub fn issue_adaptor_cert<T>(&self, mut t: T, seed_public_key: &PublicKey) -> AdaptorCertSecret
    where T: SigningTranscript
    {
        t.proto_name(b"Adaptor");
        t.commit_point(b"issuer-pk",self.public.as_compressed());

        // We cannot commit the `seed_public_key` to the transcript
        // because the whole point is to keep the transcript minimal.
        // Instead we consume it as witness datathat influences only k.
        let k = t.witness_scalar(b"issuing",&[ &self.secret.nonce, seed_public_key.as_compressed().as_bytes() ]);

        // Compute the public key reconstruction data
        let gamma = seed_public_key.as_point() + &k * constants::RISTRETTO_BASEPOINT_TABLE;
        let gamma = gamma.compress();
        t.commit_point(b"gamma",&gamma);
        let cert_public = AdaptorCertPublic(gamma.0);

        // Compute the secret key reconstruction data
        let s = k + cert_public.derive_e(t) * self.secret.key;

        let mut cert_secret = AdaptorCertSecret([0u8; 64]);
        cert_secret.0[0..32].copy_from_slice(&cert_public.0[..]);
        cert_secret.0[32..64].copy_from_slice(s.as_bytes());
        cert_secret
    }
}

impl PublicKey {
    /// Accept an Adaptor implicit certificate
    ///
    /// We request an Adaptor implicit certificate by first creating an
    /// ephemeral `Keypair` and sending the public portion to the issuer
    /// as `seed_public_key`.  An issuer issues the certificate by replying
    /// with the `AdaptorCertSecret` created by `issue_adaptor_cert`.
    ///
    /// Aside from the issuer `PublicKey` supplied as `self`, you provide
    /// (1) a `SigningTranscript` called `t` that incorporates both the context
    ///     and the certificate requester's identity,
    /// (2) the `seed_secret_key` corresponding to the `seed_public_key`
    ///     they sent to the issuer by the certificate recipient in their
    ///     certificate request, and
    /// (3) the `AdaptorCertSecret` send by the issuer to the certificate
    ///     requester.
    /// We return both your certificate's new `SecretKey` as well as
    /// an `AdaptorCertPublic` from which third parties may derive
    /// corresponding public key from `h` and the issuer's public key.
    pub fn accept_adaptor_cert<T>(
        &self,
        mut t: T,
        seed_secret_key: &SecretKey,
        cert_secret: AdaptorCertSecret
    ) -> SignatureResult<(AdaptorCertPublic, SecretKey)>
    where T: SigningTranscript
    {
        t.proto_name(b"Adaptor");
        t.commit_point(b"issuer-pk",self.as_compressed());

        // Again we cannot commit much to the transcript, but we again
        // treat anything relevant as a witness when defining the
        let mut nonce = [0u8; 32];
        t.witness_bytes(b"accepting",&mut nonce, &[&cert_secret.0[..],&seed_secret_key.nonce]);

        let mut s = [0u8; 32];
        s.copy_from_slice(&cert_secret.0[32..64]);
        let s = crate::scalar_from_canonical_bytes(s).ok_or(SignatureError::ScalarFormatError) ?;
        let cert_public : AdaptorCertPublic = cert_secret.into();
        let gamma = CompressedRistretto(cert_public.0);
        t.commit_point(b"gamma",&gamma);

        let key = s + seed_secret_key.key;
        Ok(( cert_public, SecretKey { key, nonce } ))
    }
}

impl Keypair {
    /// Issue an Adaptor Implicit Certificate for yourself
    ///
    /// We can issue an implicit certificate to ourselves if we merely
    /// want to certify an associated public key.  We should prefer
    /// this option over "hierarchical deterministic" key derivation
    /// because compromising the resulting secret key does not
    /// compromise the issuer's secret key.
    ///
    /// In this case, we avoid the entire interactive protocol described
    /// by `issue_adaptor_cert` and `accept_adaptor_cert` by hiding it an all
    /// management of the ephemeral `Keypair` inside this function.
    ///
    /// Aside from the issuing secret key supplied as `self`, you provide
    /// only a digest `h` that incorporates any context and metadata
    /// pertaining to the issued key.
    pub fn issue_self_adaptor_cert<T>(&self, t: T) -> (AdaptorCertPublic, SecretKey)
    where T: SigningTranscript+Clone
    {
        let mut bytes = [0u8; 96];
        t.witness_bytes(b"issue_self_adaptor_cert", &mut bytes, &[&self.secret.nonce, &self.secret.to_bytes() as &[u8]]);

        let mut nonce = [0u8; 32];
        nonce.copy_from_slice(&bytes[64..96]);

        let mut key = [0u8; 64];
        key.copy_from_slice(&bytes[0..64]);
        let key = Scalar::from_bytes_mod_order_wide(&key);

        let seed = SecretKey { key, nonce }.to_keypair();
        let cert_secret = self.issue_adaptor_cert(t.clone(), &seed.public);
        self.public.accept_adaptor_cert(t, &seed.secret, cert_secret).expect("Cert issued above and known to produce signature errors; qed")
    }
}

impl PublicKey {
    /// Extract the certified pulic key from an adaptor certificate
    /// 
    /// We've no confirmation that this public key was certified
    /// until we use it in some authenticated setting, like an AEAD
    /// or another signature.
    pub fn open_adaptor_cert<T>(&self, mut t: T, cert_public: &AdaptorCertPublic) -> SignatureResult<PublicKey>
    where T: SigningTranscript
    {
        t.proto_name(b"Adaptor");
        t.commit_point(b"issuer-pk",self.as_compressed());

        let gamma = CompressedRistretto(cert_public.0);
        t.commit_point(b"gamma",&gamma);
        let gamma = gamma.decompress().ok_or(SignatureError::PointDecompressionError) ?;

        let point = cert_public.derive_e(t) * self.as_point() + gamma;
        Ok(PublicKey::from_point(point))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[cfg(feature = "getrandom")]
    #[test]
    fn adaptor_cert_public_vs_private_paths() {
        let t = signing_context(b"").bytes(b"MrMeow!");

        let mut csprng = rand_core::OsRng;
        let issuer = Keypair::generate_with(&mut csprng);

        let (cert_public,secret_key) = issuer.issue_self_adaptor_cert(t.clone());
        let public_key = issuer.public.open_adaptor_cert(t,&cert_public).unwrap();
        assert_eq!(secret_key.to_public(), public_key);
    }
}
