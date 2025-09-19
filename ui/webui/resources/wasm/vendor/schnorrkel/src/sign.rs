// -*- mode: rust; -*-
//
// This file is part of schnorrkel.
// Copyright (c) 2017-2019 isis lovecruft
// Copyright (c) 2019 Web 3 Foundation
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>
// - Jeffrey Burdges <jeff@web3.foundation>

//! ### Schnorr signature creation and verification, including batch verification.


use core::fmt::{Debug};

use curve25519_dalek::constants;
use curve25519_dalek::ristretto::{CompressedRistretto,RistrettoPoint};
use curve25519_dalek::scalar::Scalar;

use super::*;
use crate::context::{SigningTranscript,SigningContext};


// === Actual signature type === //

/// The length of a curve25519 EdDSA `Signature`, in bytes.
pub const SIGNATURE_LENGTH: usize = 64;

/// A Ristretto Schnorr signature "detached" from the signed message.
///
/// These cannot be converted to any Ed25519 signature because they hash
/// curve points in the Ristretto encoding.
#[allow(non_snake_case)]
#[derive(Clone, Copy, Eq, PartialEq)]
pub struct Signature {
    /// `R` is a `RistrettoPoint`, formed by using an hash function with
    /// 512-bits output to produce the digest of:
    ///
    /// - the nonce half of the `SecretKey`, and
    /// - the message to be signed.
    ///
    /// This digest is then interpreted as a `Scalar` and reduced into an
    /// element in ℤ/lℤ.  The scalar is then multiplied by the distinguished
    /// basepoint to produce `R`, a `RistrettoPoint`.
    pub (crate) R: CompressedRistretto,

    /// `s` is a `Scalar`, formed by using an hash function with 512-bits output
    /// to produce the digest of:
    ///
    /// - the `r` portion of this `Signature`,
    /// - the `PublicKey` which should be used to verify this `Signature`, and
    /// - the message to be signed.
    ///
    /// This digest is then interpreted as a `Scalar` and reduced into an
    /// element in ℤ/lℤ.
    pub (crate) s: Scalar,
}

impl Debug for Signature {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "Signature( R: {:?}, s: {:?} )", &self.R, &self.s)
    }
}

pub(crate) fn check_scalar(bytes: [u8; 32]) -> SignatureResult<Scalar> {
    // Since this is only used in signature deserialisation (i.e. upon
    // verification), we can do a "succeed fast" trick by checking that the most
    // significant 4 bits are unset.  If they are unset, we can succeed fast
    // because we are guaranteed that the scalar is fully reduced.  However, if
    // the 4th most significant bit is set, we must do the full reduction check,
    // as the order of the basepoint is roughly a 2^(252.5) bit number.
    //
    // This succeed-fast trick should succeed for roughly half of all scalars.
    if bytes[31] & 0b11110000 == 0 {
        #[allow(deprecated)] // Scalar's always reduced here, so this is OK.
        return Ok(Scalar::from_bits(bytes))
    }

    crate::scalar_from_canonical_bytes(bytes).ok_or(SignatureError::ScalarFormatError)
}

impl Signature {
    const DESCRIPTION : &'static str = "A 64 byte Ristretto Schnorr signature";
    /*
    const DESCRIPTION_LONG : &'static str =
        "A 64 byte Ristretto Schnorr signature, similar to an ed25519 \
         signature as specified in RFC8032, except the Ristretto point \
         compression is used for the curve point in the first 32 bytes";
    */

    /// Convert this `Signature` to a byte array.
    #[inline]
    pub fn to_bytes(&self) -> [u8; SIGNATURE_LENGTH] {
        let mut bytes: [u8; SIGNATURE_LENGTH] = [0u8; SIGNATURE_LENGTH];
        bytes[..32].copy_from_slice(&self.R.as_bytes()[..]);
        bytes[32..].copy_from_slice(&self.s.as_bytes()[..]);
        bytes[63] |= 128;
        bytes
    }

    /// Construct a `Signature` from a slice of bytes.
    ///
    /// We distinguish schnorrkell signatures from ed25519 signatures
    /// by setting the high bit of byte 31.  We return an error if
    /// this marker remains unset because otherwise schnorrkel
    /// signatures would be indistinguishable from ed25519 signatures.
    /// We cannot always distinguish between schnorrkel and ed25519
    /// public keys either, so without this marker bit we could not
    /// do batch verification in systems that support precisely
    /// ed25519 and schnorrkel.
    ///
    /// We cannot distinguish amongst different `SigningTranscript`
    /// types using these marker bits, but protocol should not need
    /// two different transcript types.
    #[inline]
    pub fn from_bytes(bytes: &[u8]) -> SignatureResult<Signature> {
        if bytes.len() != SIGNATURE_LENGTH {
            return Err(SignatureError::BytesLengthError {
                name: "Signature",
                description: Signature::DESCRIPTION,
                length: SIGNATURE_LENGTH
            });
        }

        let mut lower: [u8; 32] = [0u8; 32];
        let mut upper: [u8; 32] = [0u8; 32];
        lower.copy_from_slice(&bytes[..32]);
        upper.copy_from_slice(&bytes[32..]);
        if upper[31] & 128 == 0 {
            return Err(SignatureError::NotMarkedSchnorrkel);
        }
        upper[31] &= 127;

        Ok(Signature{ R: CompressedRistretto(lower), s: check_scalar(upper) ? })
    }

    /// Deprecated construction of a `Signature` from a slice of bytes
    /// without checking the bit distinguishing from ed25519.  Deprecated.
    #[cfg(feature = "preaudit_deprecated")]
    #[inline]
    pub fn from_bytes_not_distinguished_from_ed25519(bytes: &[u8]) -> SignatureResult<Signature> {
        if bytes.len() != SIGNATURE_LENGTH {
            return Err(SignatureError::BytesLengthError {
                name: "Signature",
                description: Signature::DESCRIPTION,
                length: SIGNATURE_LENGTH
            });
        }
        let mut bytes0: [u8; SIGNATURE_LENGTH] = [0u8; SIGNATURE_LENGTH];
        bytes0.copy_from_slice(bytes);
        bytes0[63] |= 128;
        Signature::from_bytes(&bytes0[..])
    }
}

serde_boilerplate!(Signature);


// === Implement signing and verification operations on key types === //

impl SecretKey {
    /// Sign a transcript with this `SecretKey`.
    ///
    /// Requires a `SigningTranscript`, normally created from a
    /// `SigningContext` and a message, as well as the public key
    /// corresponding to `self`.  Returns a Schnorr signature.
    ///
    /// We employ a randomized nonce here, but also incorporate the
    /// transcript like in a derandomized scheme, but only after first
    /// extending the transcript by the public key.  As a result, there
    /// should be no attacks even if both the random number generator
    /// fails and the function gets called with the wrong public key.
    #[allow(non_snake_case)]
    pub fn sign<T: SigningTranscript>(&self, mut t: T, public_key: &PublicKey) -> Signature
    {
        t.proto_name(b"Schnorr-sig");
        t.commit_point(b"sign:pk",public_key.as_compressed());

        let mut r = t.witness_scalar(b"signing",&[&self.nonce]);  // context, message, A/public_key
        let R = (&r * constants::RISTRETTO_BASEPOINT_TABLE).compress();

        t.commit_point(b"sign:R",&R);

        let k: Scalar = t.challenge_scalar(b"sign:c");  // context, message, A/public_key, R=rG
        let s: Scalar = k * self.key + r;

        zeroize::Zeroize::zeroize(&mut r);

        Signature{ R, s }
    }

    /// Sign a message with this `SecretKey`, but doublecheck the result.
    pub fn sign_doublecheck<T>(&self, t: T, public_key: &PublicKey) -> SignatureResult<Signature>
    where T: SigningTranscript+Clone
    {
        let sig = self.sign(t.clone(),public_key);
        let sig = Signature::from_bytes(& sig.to_bytes()) ?;
        PublicKey::from_bytes(& public_key.to_bytes()) ?
        .verify(t,&sig).map(|()| sig)
    }

    /// Sign a message with this `SecretKey`.
    pub fn sign_simple(&self, ctx: &[u8], msg: &[u8], public_key: &PublicKey) -> Signature
    {
        let t = SigningContext::new(ctx).bytes(msg);
        self.sign(t,public_key)
    }

    /// Sign a message with this `SecretKey`, but doublecheck the result.
    pub fn sign_simple_doublecheck(&self, ctx: &[u8], msg: &[u8], public_key: &PublicKey)
     -> SignatureResult<Signature>
    {
        let t = SigningContext::new(ctx).bytes(msg);
        let sig = self.sign(t,public_key);
        let sig = Signature::from_bytes(& sig.to_bytes()) ?;
        PublicKey::from_bytes(& public_key.to_bytes()) ?
        .verify_simple(ctx,msg,&sig).map(|()| sig)
    }
}


impl PublicKey {
    /// Verify a signature by this public key on a transcript.
    ///
    /// Requires a `SigningTranscript`, normally created from a
    /// `SigningContext` and a message, as well as the signature
    /// to be verified.
    #[allow(non_snake_case)]
    pub fn verify<T: SigningTranscript>(&self, mut t: T, signature: &Signature)
     -> SignatureResult<()>
    {
        let A: &RistrettoPoint = self.as_point();

        t.proto_name(b"Schnorr-sig");
        t.commit_point(b"sign:pk",self.as_compressed());
        t.commit_point(b"sign:R",&signature.R);

        let k: Scalar = t.challenge_scalar(b"sign:c");  // context, message, A/public_key, R=rG
        let R = RistrettoPoint::vartime_double_scalar_mul_basepoint(&k, &(-A), &signature.s);

        if R.compress() == signature.R { Ok(()) } else { Err(SignatureError::EquationFalse) }
    }

    /// Verify a signature by this public key on a message.
    pub fn verify_simple(&self, ctx: &[u8], msg: &[u8], signature: &Signature)
     -> SignatureResult<()>
    {
        let t = SigningContext::new(ctx).bytes(msg);
        self.verify(t,signature)
    }

    /// A temporary verification routine for use in transitioning substrate testnets only.
    #[cfg(feature = "preaudit_deprecated")]
    #[allow(non_snake_case)]
    pub fn verify_simple_preaudit_deprecated(&self, ctx: &'static [u8], msg: &[u8], sig: &[u8])
     -> SignatureResult<()>
    {
        let t = SigningContext::new(ctx).bytes(msg);

        if let Ok(signature) = Signature::from_bytes(sig) {
            return self.verify(t,&signature);
        }

        let signature = Signature::from_bytes_not_distinguished_from_ed25519(sig) ?;

        let mut t = merlin::Transcript::new(ctx);
        t.append_message(b"sign-bytes", msg);

        let A: &RistrettoPoint = self.as_point();

        t.proto_name(b"Schnorr-sig");
        t.commit_point(b"pk",self.as_compressed());
        t.commit_point(b"no",&signature.R);

        let k: Scalar = t.challenge_scalar(b"");  // context, message, A/public_key, R=rG
        let R = RistrettoPoint::vartime_double_scalar_mul_basepoint(&k, &(-A), &signature.s);

        if R.compress() == signature.R { Ok(()) } else { Err(SignatureError::EquationFalse) }
    }

}


impl Keypair {
    /// Sign a transcript with this keypair's secret key.
    ///
    /// Requires a `SigningTranscript`, normally created from a
    /// `SigningContext` and a message.  Returns a Schnorr signature.
    ///
    /// # Examples
    ///
    /// Internally, we manage signature transcripts using a 128 bit secure
    /// STROBE construction based on Keccak, which itself is extremely fast
    /// and secure.  You might however influence performance or security
    /// by prehashing your message, like
    ///
    /// ```
    /// use schnorrkel::{Signature,Keypair};
    /// use rand::prelude::*; // ThreadRng,thread_rng
    /// use sha3::Shake128;
    /// use sha3::digest::{Update};
    ///
    /// # #[cfg(all(feature = "std"))]
    /// # fn main() {
    /// let mut csprng: ThreadRng = thread_rng();
    /// let keypair: Keypair = Keypair::generate_with(&mut csprng);
    /// let message: &[u8] = b"All I want is to pet all of the dogs.";
    ///
    /// // Create a hash digest object and feed it the message:
    /// let prehashed = Shake128::default().chain(message);
    /// # }
    /// #
    /// # #[cfg(any(not(feature = "std")))]
    /// # fn main() { }
    /// ```
    ///
    /// We require a "context" string for all signatures, which should
    /// be chosen judiciously for your project.  It should represent the
    /// role the signature plays in your application.  If you use the
    /// context in two purposes, and the same key, then a signature for
    /// one purpose can be substituted for the other.
    ///
    /// ```
    /// # use schnorrkel::{Keypair,Signature,signing_context};
    /// # use rand::prelude::*; // ThreadRng,thread_rng
    /// # use sha3::digest::Update;
    /// #
    /// # #[cfg(all(feature = "std"))]
    /// # fn main() {
    /// # let mut csprng: ThreadRng = thread_rng();
    /// # let keypair: Keypair = Keypair::generate_with(&mut csprng);
    /// # let message: &[u8] = b"All I want is to pet all of the dogs.";
    /// # let prehashed = sha3::Shake256::default().chain(message);
    /// #
    /// let ctx = signing_context(b"My Signing Context");
    ///
    /// let sig: Signature = keypair.sign(ctx.xof(prehashed));
    /// # }
    /// #
    /// # #[cfg(any(not(feature = "std")))]
    /// # fn main() { }
    /// ```
    ///
    // lol  [terrible_idea]: https://github.com/isislovecruft/scripts/blob/master/gpgkey2bc.py
    pub fn sign<T: SigningTranscript>(&self, t: T) -> Signature
    {
        self.secret.sign(t, &self.public)
    }

    /// Sign a message with this keypair's secret key.
    pub fn sign_simple(&self, ctx: &[u8], msg: &[u8]) -> Signature
    {
        self.secret.sign_simple(ctx, msg, &self.public)
    }

    /// Verify a signature by keypair's public key on a transcript.
    ///
    /// Requires a `SigningTranscript`, normally created from a
    /// `SigningContext` and a message, as well as the signature
    /// to be verified.
    ///
    /// # Examples
    ///
    /// ```
    /// use schnorrkel::{Keypair,Signature,signing_context};
    /// use rand::prelude::*; // ThreadRng,thread_rng
    ///
    /// # fn main() {
    /// # #[cfg(feature = "getrandom")]
    /// # {
    /// let mut csprng: ThreadRng = thread_rng();
    /// let keypair: Keypair = Keypair::generate_with(&mut csprng);
    /// let message: &[u8] = b"All I want is to pet all of the dogs.";
    ///
    /// let ctx = signing_context(b"Some context string");
    ///
    /// let sig: Signature = keypair.sign(ctx.bytes(message));
    ///
    /// assert!( keypair.public.verify(ctx.bytes(message), &sig).is_ok() );
    /// # }
    /// # }
    /// ```
    pub fn verify<T: SigningTranscript>(&self, t: T, signature: &Signature) -> SignatureResult<()>
    {
        self.public.verify(t, signature)
    }

    /// Verify a signature by keypair's public key on a message.
    pub fn verify_simple(&self, ctx: &[u8], msg: &[u8], signature: &Signature) -> SignatureResult<()>
    {
        self.public.verify_simple(ctx, msg, signature)
    }


    /// Sign a message with this `SecretKey`, but doublecheck the result.
    pub fn sign_doublecheck<T>(&self, t: T) -> SignatureResult<Signature>
    where T: SigningTranscript+Clone
    {
        let sig = self.sign(t.clone());
        let sig = Signature::from_bytes(& sig.to_bytes()) ?;
        PublicKey::from_bytes(& self.public.to_bytes()) ?
        .verify(t,&sig).map(|()| sig)
    }

    /// Sign a message with this `SecretKey`, but doublecheck the result.
    pub fn sign_simple_doublecheck(&self, ctx: &[u8], msg: &[u8])
     -> SignatureResult<Signature>
    {
        let t = SigningContext::new(ctx).bytes(msg);
        let sig = self.sign(t);
        let sig = Signature::from_bytes(& sig.to_bytes()) ?;
        PublicKey::from_bytes(& self.public.to_bytes()) ?
        .verify_simple(ctx,msg,&sig).map(|()| sig)
    }

}


#[cfg(test)]
mod test {
    use sha3::Shake128;
    use curve25519_dalek::digest::{Update};

    use super::super::*;


    #[cfg(feature = "getrandom")]
    #[test]
    fn sign_verify_bytes() {
        let good_sig: Signature;
        let bad_sig:  Signature;

        let ctx = signing_context(b"good");

        let good: &[u8] = "test message".as_bytes();
        let bad:  &[u8] = "wrong message".as_bytes();

        let mut csprng = rand_core::OsRng;

        let keypair = Keypair::generate_with(&mut csprng);
        good_sig = keypair.sign(ctx.bytes(&good));
        bad_sig  = keypair.sign(ctx.bytes(&bad));

        let good_sig = Signature::from_bytes(&good_sig.to_bytes()[..]).unwrap();
        let bad_sig  = Signature::from_bytes(&bad_sig.to_bytes()[..]).unwrap();

        assert!(keypair.verify(ctx.bytes(&good), &good_sig).is_ok(),
                "Verification of a valid signature failed!");
        assert!(!keypair.verify(ctx.bytes(&good), &bad_sig).is_ok(),
                "Verification of a signature on a different message passed!");
        assert!(!keypair.verify(ctx.bytes(&bad),  &good_sig).is_ok(),
                "Verification of a signature on a different message passed!");
        assert!(!keypair.verify(signing_context(b"bad").bytes(&good),  &good_sig).is_ok(),
                "Verification of a signature on a different message passed!");
    }

    #[cfg(feature = "getrandom")]
    #[test]
    fn sign_verify_xof() {
        let good_sig: Signature;
        let bad_sig:  Signature;

        let ctx = signing_context(b"testing testing 1 2 3");

        let good: &[u8] = b"test message";
        let bad:  &[u8] = b"wrong message";

        let prehashed_good: Shake128 = Shake128::default().chain(good);
        let prehashed_bad: Shake128 = Shake128::default().chain(bad);
        // You may verify that `Shake128: Copy` is possible, making these clones below correct.

        let mut csprng = rand_core::OsRng;

        let keypair = Keypair::generate_with(&mut csprng);
        good_sig = keypair.sign(ctx.xof(prehashed_good.clone()));
        bad_sig  = keypair.sign(ctx.xof(prehashed_bad.clone()));

        let good_sig_d = Signature::from_bytes(&good_sig.to_bytes()[..]).unwrap();
        let bad_sig_d  = Signature::from_bytes(&bad_sig.to_bytes()[..]).unwrap();
        assert_eq!(good_sig, good_sig_d);
        assert_eq!(bad_sig, bad_sig_d);

        assert!(keypair.verify(ctx.xof(prehashed_good.clone()), &good_sig).is_ok(),
                "Verification of a valid signature failed!");
        assert!(! keypair.verify(ctx.xof(prehashed_good.clone()), &bad_sig).is_ok(),
                "Verification of a signature on a different message passed!");
        assert!(! keypair.verify(ctx.xof(prehashed_bad.clone()), &good_sig).is_ok(),
                "Verification of a signature on a different message passed!");
        assert!(! keypair.verify(signing_context(b"oops").xof(prehashed_good), &good_sig).is_ok(),
                "Verification of a signature on a different message passed!");
    }

    #[cfg(feature = "preaudit_deprecated")]
    #[test]
    fn can_verify_know_preaudit_deprecated_message() {
        use hex_literal::hex;
        const SIGNING_CTX : &'static [u8] = b"substrate";
        let message = b"Verifying that I am the owner of 5G9hQLdsKQswNPgB499DeA5PkFBbgkLPJWkkS6FAM6xGQ8xD. Hash: 221455a3\n";
        let public = hex!("b4bfa1f7a5166695eb75299fd1c4c03ea212871c342f2c5dfea0902b2c246918");
        let public = PublicKey::from_bytes(&public[..]).unwrap();
        let signature = hex!("5a9755f069939f45d96aaf125cf5ce7ba1db998686f87f2fb3cbdea922078741a73891ba265f70c31436e18a9acd14d189d73c12317ab6c313285cd938453202");
        assert!( public.verify_simple_preaudit_deprecated(SIGNING_CTX,message,&signature[..]).is_ok() );
    }
}
