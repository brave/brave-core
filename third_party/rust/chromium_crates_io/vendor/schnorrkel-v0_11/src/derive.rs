// -*- mode: rust; -*-
//
// This file is part of schnorrkel.
// Copyright (c) 2019 Web 3 Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Jeffrey Burdges <jeff@web3.foundation>

//! ### Implementation of "hierarchical deterministic key derivation" (HDKD) for Schnorr signatures on Ristretto 
//! 
//! *Warning*  We warn that our VRF construction in vrf.rs supports
//! malleable VRF outputs via the `Malleable` type, which becomes
//! insecure when used in conjunction with our hierarchical key
//! derivation methods here.
//! Attackers could translate malleable VRF outputs from one soft subkey 
//! to another soft subkey, gaining early knowledge of the VRF output.
//! We think most VRF applications for which HDKH sounds suitable
//! benefit from using implicit certificates instead of HDKD anyways,
//! which should also be secure in combination with HDKH.
//! We always use non-malleable VRF inputs in our convenience methods.

//! We suggest using implicit certificates instead of HDKD when 
//! using VRFs.
//!
//! 

// use curve25519_dalek::digest::generic_array::typenum::U64;
// use curve25519_dalek::digest::Digest;

use curve25519_dalek::constants;
use curve25519_dalek::scalar::Scalar;

use super::*;
use crate::context::{SigningTranscript};

/// Length in bytes of our chain codes.
///
/// In fact, only 16 bytes sounds safe, but this never appears on chain,
/// so no downsides to using 32 bytes.
pub const CHAIN_CODE_LENGTH: usize = 32;

/// We cannot assume the original public key is secret and additional
/// inputs might have low entropy, like `i` in BIP32.  As in BIP32,
/// chain codes fill this gap by being a high entropy secret shared
/// between public and private key holders.  These are produced by
/// key derivations and can be incorporated into subsequence key
/// derivations. 
/// See https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#extended-keys
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct ChainCode(pub [u8; CHAIN_CODE_LENGTH]);

/// Key types that support "hierarchical deterministic" key derivation
pub trait Derivation : Sized {
    /// Derive key with subkey identified by a byte array
    /// presented via a `SigningTranscript`, and a chain code.
    fn derived_key<T>(&self, t: T, cc: ChainCode) -> (Self, ChainCode)
    where T: SigningTranscript;

    /// Derive key with subkey identified by a byte array 
    /// and a chain code.  We do not include a context here
    /// because the chain code could serve this purpose.
    fn derived_key_simple<B: AsRef<[u8]>>(&self, cc: ChainCode, i: B) -> (Self, ChainCode) {
        let mut t = merlin::Transcript::new(b"SchnorrRistrettoHDKD");
        t.append_message(b"sign-bytes", i.as_ref());
        self.derived_key(t, cc)
    }

    /// Derive key with subkey identified by a byte array
    /// and a chain code, and with external randomnesses.
    fn derived_key_simple_rng<B,R>(&self, cc: ChainCode, i: B, rng: R) -> (Self, ChainCode)
    where B: AsRef<[u8]>, R: RngCore+CryptoRng
    {
        let mut t = merlin::Transcript::new(b"SchnorrRistrettoHDKD");
        t.append_message(b"sign-bytes", i.as_ref());
        self.derived_key(super::context::attach_rng(t,rng), cc)
    }
}

impl PublicKey {
    /// Derive a mutating scalar and new chain code from a public key and chain code.
    ///
    /// If `i` is the "index", `c` is the chain code, and `pk` the public key,
    /// then we compute `H(i ++ c ++ pk)` and define our mutating scalar
    /// to be the 512 bits of output reduced mod l, and define the next chain
    /// code to be next 256 bits.  
    ///
    /// We update the signing transcript as a side effect.
    fn derive_scalar_and_chaincode<T>(&self, t: &mut T, cc: ChainCode) -> (Scalar, ChainCode)
    where T: SigningTranscript
    {
        t.commit_bytes(b"chain-code",&cc.0);
        t.commit_point(b"public-key",self.as_compressed());

        let scalar = t.challenge_scalar(b"HDKD-scalar");

        let mut chaincode = [0u8; 32];
        t.challenge_bytes(b"HDKD-chaincode", &mut chaincode);

        (scalar, ChainCode(chaincode))
    }
}

impl SecretKey {
    /// Vaguely BIP32-like "hard" derivation of a `MiniSecretKey` from a `SecretKey`
    ///
    /// We do not envision any "good reasons" why these "hard"
    /// derivations should ever be used after the soft `Derivation`
    /// trait.  We similarly do not believe hard derivations
    /// make any sense for `ChainCode`s or `ExtendedKey`s types.
    /// Yet, some existing BIP32 workflows might do these things,
    /// due to BIP32's de facto standardization and poor design.
    /// In consequence, we provide this method to do "hard" derivations
    /// in a way that should work with all BIP32 workflows and any
    /// permissible mutations of `SecretKey`.  This means only that
    /// we hash the `SecretKey`'s scalar, but not its nonce because
    /// the secret key remains valid if the nonce is changed.
    pub fn hard_derive_mini_secret_key<B: AsRef<[u8]>>(&self, cc: Option<ChainCode>, i: B)
     -> (MiniSecretKey,ChainCode)
    {
        let mut t = merlin::Transcript::new(b"SchnorrRistrettoHDKD");
        t.append_message(b"sign-bytes", i.as_ref());

        if let Some(c) = cc { t.append_message(b"chain-code", &c.0); }
        t.append_message(b"secret-key",& self.key.to_bytes() as &[u8]);

        let mut msk = [0u8; MINI_SECRET_KEY_LENGTH]; 
        t.challenge_bytes(b"HDKD-hard",&mut msk);

        let mut chaincode = [0u8; 32];
        t.challenge_bytes(b"HDKD-chaincode", &mut chaincode);

        (MiniSecretKey(msk), ChainCode(chaincode))
    }
}

impl MiniSecretKey {
    /// Vaguely BIP32-like "hard" derivation of a `MiniSecretKey` from a `SecretKey`
    ///
    /// We do not envision any "good reasons" why these "hard"
    /// derivations should ever be used after the soft `Derivation`
    /// trait.  We similarly do not believe hard derivations
    /// make any sense for `ChainCode`s or `ExtendedKey`s types.
    /// Yet, some existing BIP32 workflows might do these things,
    /// due to BIP32's de facto standardization and poor design.
    /// In consequence, we provide this method to do "hard" derivations
    /// in a way that should work with all BIP32 workflows and any
    /// permissible mutations of `SecretKey`.  This means only that
    /// we hash the `SecretKey`'s scalar, but not its nonce because
    /// the secret key remains valid if the nonce is changed.
    pub fn hard_derive_mini_secret_key<B: AsRef<[u8]>>(&self, cc: Option<ChainCode>, i: B, mode: ExpansionMode)
     -> (MiniSecretKey,ChainCode)
    {
        self.expand(mode).hard_derive_mini_secret_key(cc,i)
    }
}

impl Keypair {
    /// Vaguely BIP32-like "hard" derivation of a `MiniSecretKey` from a `SecretKey`
    ///
    /// We do not envision any "good reasons" why these "hard"
    /// derivations should ever be used after the soft `Derivation`
    /// trait.  We similarly do not believe hard derivations
    /// make any sense for `ChainCode`s or `ExtendedKey`s types.
    /// Yet, some existing BIP32 workflows might do these things,
    /// due to BIP32's de facto standardization and poor design.
    /// In consequence, we provide this method to do "hard" derivations
    /// in a way that should work with all BIP32 workflows and any
    /// permissible mutations of `SecretKey`.  This means only that
    /// we hash the `SecretKey`'s scalar, but not its nonce because
    /// the secret key remains valid if the nonce is changed.
    pub fn hard_derive_mini_secret_key<B: AsRef<[u8]>>(&self, cc: Option<ChainCode>, i: B)
     -> (MiniSecretKey,ChainCode) {
        self.secret.hard_derive_mini_secret_key(cc,i)
    }

    /// Derive a secret key and new chain code from a key pair and chain code.
    ///
    /// We expect the trait methods of `Keypair as Derivation` to be
    /// more useful since signing anything requires the public key too.
    pub fn derive_secret_key<T>(&self, mut t: T, cc: ChainCode) -> (SecretKey, ChainCode)
    where T: SigningTranscript
    {
        let (scalar, chaincode) = self.public.derive_scalar_and_chaincode(&mut t, cc);

        // We can define the nonce however we like here since it only protects
        // the signature from bad random number generators.  It need not be
        // specified by any specification or standard.  It must however be
        // independent from the mutating scalar and new chain code.
        // We employ the witness mechanism here so that CSPRNG associated to our
        // `SigningTranscript` makes our new nonce seed independent from everything.
        let mut nonce = [0u8; 32];
        t.witness_bytes(b"HDKD-nonce", &mut nonce, &[&self.secret.nonce, &self.secret.to_bytes() as &[u8]]);

        (SecretKey {
            key: self.secret.key + scalar,
            nonce,
        }, chaincode)
    }
}

impl Derivation for Keypair {
    fn derived_key<T>(&self, t: T, cc: ChainCode) -> (Keypair, ChainCode)
    where T: SigningTranscript
    {
        let (secret, chaincode) = self.derive_secret_key(t, cc);
        let public = secret.to_public();
        (Keypair { secret, public }, chaincode)
    }
}

impl Derivation for SecretKey {
    fn derived_key<T>(&self, t: T, cc: ChainCode) -> (SecretKey, ChainCode)
    where T: SigningTranscript
    {
        self.clone().to_keypair().derive_secret_key(t, cc)
    }
}

impl Derivation for PublicKey {
    fn derived_key<T>(&self, mut t: T, cc: ChainCode) -> (PublicKey, ChainCode)
    where T: SigningTranscript
    {
        let (scalar, chaincode) = self.derive_scalar_and_chaincode(&mut t, cc);
        let point = self.as_point() + (&scalar * constants::RISTRETTO_BASEPOINT_TABLE);
        (PublicKey::from_point(point), chaincode)
    }
}

/// A convenience wraper that combines derivable key and a chain code.
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct ExtendedKey<K> {
    /// Appropriate key type
    pub key: K,
    /// We cannot assume the original public key is secret and additional
    /// inputs might have low entropy, like `i` in BIP32.  As in BIP32,
    /// chain codes fill this gap by being a high entropy secret shared
    /// between public and private key holders.  These are produced by
    /// key derivations and can be incorporated into subsequence key
    /// derivations.  
    pub chaincode: ChainCode,
}
// TODO: Serialization

impl<K: Derivation> ExtendedKey<K> {
    /// Derive key with subkey identified by a byte array
    /// presented as a hash, and a chain code.
    pub fn derived_key<T>(&self, t: T) -> ExtendedKey<K>
    where T: SigningTranscript
    {
        let (key, chaincode) = self.key.derived_key(t, self.chaincode);
        ExtendedKey { key, chaincode }
    }

    /// Derive key with subkey identified by a byte array and 
    /// a chain code in the extended key.
    pub fn derived_key_simple<B: AsRef<[u8]>>(&self, i: B) -> ExtendedKey<K>
    {
        let (key, chaincode) = self.key.derived_key_simple(self.chaincode, i);
        ExtendedKey { key, chaincode }
    }
}

impl ExtendedKey<SecretKey> {
    /// Vaguely BIP32-like "hard" derivation of a `MiniSecretKey` from a `SecretKey`
    ///
    /// We do not envision any "good reasons" why these "hard"
    /// derivations should ever be used after the soft `Derivation`
    /// trait.  We similarly do not believe hard derivations
    /// make any sense for `ChainCode`s or `ExtendedKey`s types.
    /// Yet, some existing BIP32 workflows might do these things,
    /// due to BIP32's de facto standardization and poor design.
    /// In consequence, we provide this method to do "hard" derivations
    /// in a way that should work with all BIP32 workflows and any
    /// permissible mutations of `SecretKey`.  This means only that
    /// we hash the `SecretKey`'s scalar, but not its nonce because
    /// the secret key remains valid if the nonce is changed.
    pub fn hard_derive_mini_secret_key<B: AsRef<[u8]>>(&self, i: B, mode: ExpansionMode)
     -> ExtendedKey<SecretKey> 
     {
        let (key,chaincode) = self.key.hard_derive_mini_secret_key(Some(self.chaincode), i);
        let key = key.expand(mode);
        ExtendedKey { key, chaincode }
    }
}

#[cfg(test)]
mod tests {
    use sha3::digest::{Update}; // ExtendableOutput,XofReader
    use sha3::{Shake128};

    use super::*;

    #[cfg(feature = "getrandom")]
    #[test]
    fn derive_key_public_vs_private_paths() {
        let chaincode = ChainCode([0u8; CHAIN_CODE_LENGTH]);
        let msg : &'static [u8] = b"Just some test message!";
        let mut h = Shake128::default().chain(msg);

        let mut csprng = rand_core::OsRng;
        let key = Keypair::generate_with(&mut csprng);

        let mut extended_public_key = ExtendedKey {
            key: key.public.clone(),
            chaincode,
        };
        let mut extended_keypair = ExtendedKey { key, chaincode, };

        let ctx = signing_context(b"testing testing 1 2 3");

        for i in 0..30 {
            let extended_keypair1 = extended_keypair.derived_key_simple(msg);
            let extended_public_key1 = extended_public_key.derived_key_simple(msg);
            assert_eq!(
                extended_keypair1.chaincode, extended_public_key1.chaincode,
                "Chain code derivation failed!"
            );
            assert_eq!(
                extended_keypair1.key.public, extended_public_key1.key,
                "Public and secret key derivation missmatch!"
            );
            extended_keypair = extended_keypair1;
            extended_public_key = extended_public_key1;

            h.update(b"Another");

            if i % 5 == 0 {
                let good_sig = extended_keypair.key.sign(ctx.xof(h.clone()));
                let h_bad = h.clone().chain(b"oops");
                let bad_sig = extended_keypair.key.sign(ctx.xof(h_bad.clone()));

                assert!(
                    extended_public_key.key.verify(ctx.xof(h.clone()), &good_sig).is_ok(),
                    "Verification of a valid signature failed!"
                );
                assert!(
                    ! extended_public_key.key.verify(ctx.xof(h.clone()), &bad_sig).is_ok(),
                    "Verification of a signature on a different message passed!"
                );
                assert!(
                    ! extended_public_key.key.verify(ctx.xof(h_bad), &good_sig).is_ok(),
                    "Verification of a signature on a different message passed!"
                );
            }
        }
    }
}
