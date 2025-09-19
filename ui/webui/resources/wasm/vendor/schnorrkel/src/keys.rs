// -*- mode: rust; -*-
//
// This file is part of schnorrkel.
// Copyright (c) 2019 isis lovecruft and Web 3 Foundation
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>
// - Jeff Burdges <jeff@web3.foundation>

//! ### Schnorr signatures on the 2-torsion free subgroup of ed25519, as provided by the Ristretto point compression.

use core::convert::AsRef;
use core::fmt::{Debug};

use rand_core::{RngCore,CryptoRng};

use curve25519_dalek::constants;
use curve25519_dalek::ristretto::{CompressedRistretto,RistrettoPoint};
use curve25519_dalek::scalar::Scalar;

use subtle::{Choice,ConstantTimeEq};
use zeroize::Zeroize;

use crate::scalars;
use crate::points::RistrettoBoth;
use crate::errors::{SignatureError,SignatureResult};


/// The length of a Ristretto Schnorr `MiniSecretKey`, in bytes.
pub const MINI_SECRET_KEY_LENGTH: usize = 32;

/// The length of a Ristretto Schnorr `PublicKey`, in bytes.
pub const PUBLIC_KEY_LENGTH: usize = 32;

/// The length of the "key" portion of a Ristretto Schnorr secret key, in bytes.
const SECRET_KEY_KEY_LENGTH: usize = 32;

/// The length of the "nonce" portion of a Ristretto Schnorr secret key, in bytes.
const SECRET_KEY_NONCE_LENGTH: usize = 32;

/// The length of a Ristretto Schnorr key, `SecretKey`, in bytes.
pub const SECRET_KEY_LENGTH: usize = SECRET_KEY_KEY_LENGTH + SECRET_KEY_NONCE_LENGTH;

/// The length of an Ristretto Schnorr `Keypair`, in bytes.
pub const KEYPAIR_LENGTH: usize = SECRET_KEY_LENGTH + PUBLIC_KEY_LENGTH;


/// Methods for expanding a `MiniSecretKey` into a `SecretKey`.
///
/// Our `SecretKey`s consist of a scalar and nonce seed, both 32 bytes,
/// what EdDSA/Ed25519 calls an extended secret key.  We normally create
/// `SecretKey`s by expanding a `MiniSecretKey`, what Esd25519 calls
/// a `SecretKey`.  We provide two such methods, our suggested approach
/// produces uniformly distribted secret key scalars, but another
/// approach retains the bit clamping form Ed25519.
pub enum ExpansionMode {
    /// Expand the `MiniSecretKey` into a uniformly distributed
    /// `SecretKey`.
    ///
    /// We produce the `SecretKey` using merlin and far more uniform
    /// sampling, which might benefits some future protocols, and
    /// might reduce binary size if used throughout.
    ///
    /// We slightly prefer this method, but some existing code uses
    /// `Ed25519` mode, so users cannot necessarily use this mode
    /// if they require compatability with existing systems.
    Uniform,

    /// Expand this `MiniSecretKey` into a `SecretKey` using
    /// ed25519-style bit clamping.
    ///
    /// Ristretto points are represented by Ed25519 points internally
    /// so conceivably some future standard might expose a mapping
    /// from Ristretto to Ed25519, which makes this mode useful.
    /// At present, there is no such exposed mapping however because
    /// two such mappings actually exist, depending upon the branch of
    /// the inverse square root chosen by a Ristretto implementation.
    /// There is however a concern that such a mapping would remain
    /// a second class citizen, meaning implementations differ and
    /// create incompatibility.
    ///
    /// We weakly recommend against employing this method.  We include
    /// it primarily because early Ristretto documentation touted the
    /// relationship with Ed25519, which led to some deployments adopting
    /// this expansion method.
    Ed25519,
}

/// An EdDSA-like "secret" key seed.
///
/// These are seeds from which we produce a real `SecretKey`, which
/// EdDSA itself calls an extended secret key by hashing.  We require
/// homomorphic properties unavailable from these seeds, so we renamed
/// these and reserve `SecretKey` for what EdDSA calls an extended
/// secret key.
#[derive(Clone,Zeroize)]
#[zeroize(drop)]
pub struct MiniSecretKey(pub (crate) [u8; MINI_SECRET_KEY_LENGTH]);

impl Debug for MiniSecretKey {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "MiniSecretKey: {:?}", &self.0[..])
    }
}

impl Eq for MiniSecretKey {}
impl PartialEq for MiniSecretKey {
    fn eq(&self, other: &Self) -> bool {
        self.ct_eq(other).unwrap_u8() == 1u8
    }
}
impl ConstantTimeEq for MiniSecretKey {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0.ct_eq(&other.0)
    }
}

impl MiniSecretKey {
    const DESCRIPTION : &'static str = "Analogous to ed25519 secret key as 32 bytes, see RFC8032.";

    /// Avoids importing `ExpansionMode`
    pub const UNIFORM_MODE : ExpansionMode = ExpansionMode::Uniform;

    /// Avoids importing `ExpansionMode`
    pub const ED25519_MODE : ExpansionMode = ExpansionMode::Ed25519;

    /// Expand this `MiniSecretKey` into a `SecretKey`
    ///
    /// We produce a secret keys using merlin and more uniformly
    /// with this method, which reduces binary size and benefits
    /// some future protocols.
    ///
    /// # Examples
    ///
    /// ```compile_fail
    /// # fn main() {
    /// use rand::{Rng, rngs::OsRng};
    /// use schnorrkel::{MiniSecretKey, SecretKey};
    ///
    /// let mini_secret_key: MiniSecretKey = MiniSecretKey::generate_with(OsRng);
    /// let secret_key: SecretKey = mini_secret_key.expand_uniform();
    /// # }
    /// ```
    fn expand_uniform(&self) -> SecretKey {
        let mut t = merlin::Transcript::new(b"ExpandSecretKeys");
        t.append_message(b"mini", &self.0[..]);

        let mut scalar_bytes = [0u8; 64];
        t.challenge_bytes(b"sk", &mut scalar_bytes);
        let key = Scalar::from_bytes_mod_order_wide(&scalar_bytes);

        let mut nonce = [0u8; 32];
        t.challenge_bytes(b"no", &mut nonce);

        SecretKey { key, nonce }
    }

    /// Expand this `MiniSecretKey` into a `SecretKey` using
    /// ed25519-style bit clamping.
    ///
    /// At present, there is no exposed mapping from Ristretto
    /// to the underlying Edwards curve because Ristretto involves
    /// an inverse square root, and thus two such mappings exist.
    /// Ristretto could be made usable with Ed25519 keys by choosing
    /// one mapping as standard, but doing so makes the standard more
    /// complex, and possibly harder to implement.  If anyone does
    /// standardize the mapping to the curve then this method permits
    /// compatible schnorrkel and ed25519 keys.
    ///
    /// # Examples
    ///
    /// ```compile_fail
    /// # #[cfg(feature = "getrandom")]
    /// # fn main() {
    /// use rand::{Rng, rngs::OsRng};
    /// use schnorrkel::{MiniSecretKey, SecretKey};
    ///
    /// let mini_secret_key: MiniSecretKey = MiniSecretKey::generate_with(OsRng);
    /// let secret_key: SecretKey = mini_secret_key.expand_ed25519();
    /// # }
    /// ```
    fn expand_ed25519(&self) -> SecretKey {
        use sha2::{Sha512, digest::{Update,FixedOutput}};

        let mut h = Sha512::default();
        h.update(self.as_bytes());
        let r = h.finalize_fixed();

        // We need not clamp in a Schnorr group like Ristretto, but here
        // we do so to improve Ed25519 comparability.
        let mut key = [0u8; 32];
        key.copy_from_slice(&r.as_slice()[0..32]);
        key[0]  &= 248;
        key[31] &=  63;
        key[31] |=  64;
        // We then divide by the cofactor to internally keep a clean
        // representation mod l.
        scalars::divide_scalar_bytes_by_cofactor(&mut key);

        #[allow(deprecated)] // Scalar's always reduced here, so this is OK.
        let key = Scalar::from_bits(key);

        let mut nonce = [0u8; 32];
        nonce.copy_from_slice(&r.as_slice()[32..64]);

        SecretKey{ key, nonce }
    }

    /// Derive the `SecretKey` corresponding to this `MiniSecretKey`.
    ///
    /// We caution that `mode` must always be chosen consistently.
    /// We slightly prefer `ExpansionMode::Uniform` here, but both
    /// remain secure under almost all situations.  There exists
    /// deployed code using `ExpansionMode::Ed25519`, so you might
    /// require that for compatability.
    ///
    /// ```
    /// # fn main() {
    /// use rand::{Rng, rngs::OsRng};
    /// # #[cfg(feature = "getrandom")]
    /// # {
    /// use schnorrkel::{MiniSecretKey, SecretKey, ExpansionMode};
    ///
    /// let mini_secret_key: MiniSecretKey = MiniSecretKey::generate_with(OsRng);
    /// let secret_key: SecretKey = mini_secret_key.expand(ExpansionMode::Uniform);
    /// # }
    /// # }
    /// ```
    pub fn expand(&self, mode: ExpansionMode) -> SecretKey {
        match mode {
            ExpansionMode::Uniform => self.expand_uniform(),
            ExpansionMode::Ed25519 => self.expand_ed25519(),
        }
    }

    /// Derive the `Keypair` corresponding to this `MiniSecretKey`.
    pub fn expand_to_keypair(&self, mode: ExpansionMode) -> Keypair {
        self.expand(mode).into()
    }

    /// Derive the `PublicKey` corresponding to this `MiniSecretKey`.
    pub fn expand_to_public(&self, mode: ExpansionMode) -> PublicKey {
        self.expand(mode).to_public()
    }

    /// Convert this secret key to a byte array.
    #[inline]
    pub fn to_bytes(&self) -> [u8; MINI_SECRET_KEY_LENGTH] {
        self.0
    }

    /// View this secret key as a byte array.
    #[inline]
    pub fn as_bytes(&self) -> &[u8; MINI_SECRET_KEY_LENGTH] {
        &self.0
    }

    /// Construct a `MiniSecretKey` from a slice of bytes.
    ///
    /// # Example
    ///
    /// ```
    /// use schnorrkel::{MiniSecretKey, MINI_SECRET_KEY_LENGTH};
    ///
    /// let secret_key_bytes: [u8; MINI_SECRET_KEY_LENGTH] = [
    ///    157, 097, 177, 157, 239, 253, 090, 096,
    ///    186, 132, 074, 244, 146, 236, 044, 196,
    ///    068, 073, 197, 105, 123, 050, 105, 025,
    ///    112, 059, 172, 003, 028, 174, 127, 096, ];
    ///
    /// let secret_key: MiniSecretKey = MiniSecretKey::from_bytes(&secret_key_bytes).unwrap();
    /// ```
    ///
    /// # Returns
    ///
    /// A `Result` whose okay value is an EdDSA `MiniSecretKey` or whose error value
    /// is an `SignatureError` wrapping the internal error that occurred.
    #[inline]
    pub fn from_bytes(bytes: &[u8]) -> SignatureResult<MiniSecretKey> {
        if bytes.len() != MINI_SECRET_KEY_LENGTH {
            return Err(SignatureError::BytesLengthError {
                name: "MiniSecretKey",
                description: MiniSecretKey::DESCRIPTION,
                length: MINI_SECRET_KEY_LENGTH
            });
        }
        let mut bits: [u8; 32] = [0u8; 32];
        bits.copy_from_slice(&bytes[..32]);
        Ok(MiniSecretKey(bits))
    }

    /// Generate a `MiniSecretKey` from a `csprng`.
    ///
    /// # Example
    ///
    /// ```
    /// use rand::{Rng, rngs::OsRng};
    /// use schnorrkel::{PublicKey, MiniSecretKey, Signature};
    ///
    /// let secret_key: MiniSecretKey = MiniSecretKey::generate_with(OsRng);
    /// ```
    ///
    /// # Input
    ///
    /// A CSPRNG with a `fill_bytes()` method, e.g. `rand_chacha::ChaChaRng`
    pub fn generate_with<R>(mut csprng: R) -> MiniSecretKey
    where R: CryptoRng + RngCore,
    {
        let mut sk: MiniSecretKey = MiniSecretKey([0u8; 32]);
        csprng.fill_bytes(&mut sk.0);
        sk
    }

    /// Generate a `MiniSecretKey` from rand's `thread_rng`.
    ///
    /// # Example
    ///
    /// ```
    /// use schnorrkel::{PublicKey, MiniSecretKey, Signature};
    ///
    /// let secret_key: MiniSecretKey = MiniSecretKey::generate();
    /// ```
    ///
    /// Afterwards, you can generate the corresponding public key.
    ///
    /// ```
    /// # use rand::{Rng, SeedableRng};
    /// # use rand_chacha::ChaChaRng;
    /// # use schnorrkel::{PublicKey, MiniSecretKey, ExpansionMode, Signature};
    /// #
    /// # let mut csprng: ChaChaRng = ChaChaRng::from_seed([0u8; 32]);
    /// # let secret_key: MiniSecretKey = MiniSecretKey::generate_with(&mut csprng);
    ///
    /// let public_key: PublicKey = secret_key.expand_to_public(ExpansionMode::Ed25519);
    /// ```
    #[cfg(feature = "getrandom")]
    pub fn generate() -> MiniSecretKey {
        Self::generate_with(super::getrandom_or_panic())
    }
}

serde_boilerplate!(MiniSecretKey);


/// A secret key for use with Ristretto Schnorr signatures.
///
/// Internally, these consist of a scalar mod l along with a seed for
/// nonce generation.  In this way, we ensure all scalar arithmetic
/// works smoothly in operations like threshold or multi-signatures,
/// or hierarchical deterministic key derivations.
///
/// We keep our secret key serializaion "almost" compatable with EdDSA
/// "expanded" secret key serializaion by multiplying the scalar by the
/// cofactor 8, as integers, and dividing on deserializaion.
/// We do not however attempt to keep the scalar's high bit set, especially
/// not during hierarchical deterministic key derivations, so some Ed25519
/// libraries might compute the public key incorrectly from our secret key.
#[derive(Clone,Zeroize)]
#[zeroize(drop)]
pub struct SecretKey {
    /// Actual public key represented as a scalar.
    pub (crate) key: Scalar,
    /// Seed for deriving the nonces used in signing.
    ///
    /// We require this be random and secret or else key compromise attacks will ensue.
    /// Any modification here may disrupt some non-public key derivation techniques.
    pub (crate) nonce: [u8; 32],
}

impl Debug for SecretKey {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "SecretKey {{ key: {:?} nonce: {:?} }}", &self.key, &self.nonce)
    }
}

impl Eq for SecretKey {}
impl PartialEq for SecretKey {
    fn eq(&self, other: &Self) -> bool {
        self.ct_eq(other).unwrap_u8() == 1u8
    }
}
impl ConstantTimeEq for SecretKey {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.key.ct_eq(&other.key)
    }
}

/*
impl From<&MiniSecretKey> for SecretKey {
    /// Construct an `SecretKey` from a `MiniSecretKey`.
    ///
    /// # Examples
    ///
    /// ```
    /// # #[cfg(feature = "getrandom")
    /// # fn main() {
    /// use rand::{Rng, rngs::OsRng};
    /// use schnorrkel::{MiniSecretKey, SecretKey};
    ///
    /// let mini_secret_key: MiniSecretKey = MiniSecretKey::generate_with(OsRng);
    /// let secret_key: SecretKey = SecretKey::from(&mini_secret_key);
    /// # }
    /// ```
    fn from(msk: &MiniSecretKey) -> SecretKey {
        msk.expand(ExpansionMode::Ed25519)
    }
}
*/

impl SecretKey {
    const DESCRIPTION : &'static str = "An ed25519-like expanded secret key as 64 bytes, as specified in RFC8032.";

    /// Convert this `SecretKey` into an array of 64 bytes with.
    ///
    /// Returns an array of 64 bytes, with the first 32 bytes being
    /// the secret scalar represented canonically, and the last
    /// 32 bytes being the seed for nonces.
    ///
    /// # Examples
    ///
    /// ```
    /// # #[cfg(feature = "getrandom")]
    /// # {
    /// use schnorrkel::{MiniSecretKey, SecretKey};
    ///
    /// let mini_secret_key: MiniSecretKey = MiniSecretKey::generate();
    /// let secret_key: SecretKey = mini_secret_key.expand(MiniSecretKey::UNIFORM_MODE);
    /// # // was SecretKey::from(&mini_secret_key);
    /// let secret_key_bytes: [u8; 64] = secret_key.to_bytes();
    /// let bytes: [u8; 64] = secret_key.to_bytes();
    /// let secret_key_again: SecretKey = SecretKey::from_bytes(&bytes[..]).unwrap();
    /// assert_eq!(&bytes[..], & secret_key_again.to_bytes()[..]);
    /// # }
    /// ```
    #[inline]
    pub fn to_bytes(&self) -> [u8; SECRET_KEY_LENGTH] {
        let mut bytes: [u8; 64] = [0u8; 64];
        bytes[..32].copy_from_slice(&self.key.to_bytes()[..]);
        bytes[32..].copy_from_slice(&self.nonce[..]);
        bytes
    }

    /// Construct an `SecretKey` from a slice of bytes.
    ///
    /// # Examples
    ///
    /// ```
    /// use schnorrkel::{MiniSecretKey, SecretKey, ExpansionMode, SignatureError};
    ///
    /// # #[cfg(feature = "getrandom")]
    /// # {
    /// let mini_secret_key: MiniSecretKey = MiniSecretKey::generate();
    /// let secret_key: SecretKey = mini_secret_key.expand(MiniSecretKey::ED25519_MODE);
    /// # // was SecretKey::from(&mini_secret_key);
    /// let bytes: [u8; 64] = secret_key.to_bytes();
    /// let secret_key_again: SecretKey = SecretKey::from_bytes(&bytes[..]).unwrap();
    /// assert_eq!(secret_key_again, secret_key);
    /// # }
    /// ```
    #[inline]
    pub fn from_bytes(bytes: &[u8]) -> SignatureResult<SecretKey> {
        if bytes.len() != SECRET_KEY_LENGTH {
            return Err(SignatureError::BytesLengthError{
                name: "SecretKey",
                description: SecretKey::DESCRIPTION,
                length: SECRET_KEY_LENGTH,
            });
        }

        let mut key: [u8; 32] = [0u8; 32];
        key.copy_from_slice(&bytes[00..32]);
        let key = crate::scalar_from_canonical_bytes(key).ok_or(SignatureError::ScalarFormatError) ?;

        let mut nonce: [u8; 32] = [0u8; 32];
        nonce.copy_from_slice(&bytes[32..64]);

        Ok(SecretKey{ key, nonce })
    }

    /// Convert this `SecretKey` into an array of 64 bytes, corresponding to
    /// an Ed25519 expanded secret key.
    ///
    /// Returns an array of 64 bytes, with the first 32 bytes being
    /// the secret scalar shifted ed25519 style, and the last 32 bytes
    /// being the seed for nonces.
    #[inline]
    pub fn to_ed25519_bytes(&self) -> [u8; SECRET_KEY_LENGTH] {
        let mut bytes: [u8; 64] = [0u8; 64];
        let mut key = self.key.to_bytes();
        // We multiply by the cofactor to improve ed25519 compatability,
        // while our internally using a scalar mod l.
        scalars::multiply_scalar_bytes_by_cofactor(&mut key);
        bytes[..32].copy_from_slice(&key[..]);
        bytes[32..].copy_from_slice(&self.nonce[..]);
        bytes
    }

    /* Unused tooling removed to reduce dependencies. 
    /// Convert this `SecretKey` into an Ed25519 expanded secret key.
    #[cfg(feature = "ed25519_dalek")]
    pub fn to_ed25519_expanded_secret_key(&self) -> ed25519_dalek::ExpandedSecretKey {
        ed25519_dalek::ExpandedSecretKey::from_bytes(&self.to_ed25519_bytes()[..])
        .expect("Improper serialisation of Ed25519 secret key!")
    }
    */

    /// Construct an `SecretKey` from a slice of bytes, corresponding to
    /// an Ed25519 expanded secret key.
    ///
    /// # Example
    ///
    /// ```
    /// use schnorrkel::{SecretKey, SECRET_KEY_LENGTH};
    /// use hex_literal::hex;
    ///
    /// let secret = hex!("28b0ae221c6bb06856b287f60d7ea0d98552ea5a16db16956849aa371db3eb51fd190cce74df356432b410bd64682309d6dedb27c76845daf388557cbac3ca34");
    /// let public = hex!("46ebddef8cd9bb167dc30878d7113b7e168e6f0646beffd77d69d39bad76b47a");
    /// let secret_key = SecretKey::from_ed25519_bytes(&secret[..]).unwrap();
    /// assert_eq!(secret_key.to_public().to_bytes(), public);
    /// ```
    #[inline]
    pub fn from_ed25519_bytes(bytes: &[u8]) -> SignatureResult<SecretKey> {
        if bytes.len() != SECRET_KEY_LENGTH {
            return Err(SignatureError::BytesLengthError{
                name: "SecretKey",
                description: SecretKey::DESCRIPTION,
                length: SECRET_KEY_LENGTH,
            });
        }

        let mut key: [u8; 32] = [0u8; 32];
        key.copy_from_slice(&bytes[00..32]);
        // We divide by the cofactor to internally keep a clean
        // representation mod l.
        scalars::divide_scalar_bytes_by_cofactor(&mut key);

        let key = Scalar::from_canonical_bytes(key);
        if bool::from(key.is_none()) {
            // This should never trigger for keys which come from `to_ed25519_bytes`.
            return Err(SignatureError::InvalidKey);
        }

        let key = key.unwrap();
        let mut nonce: [u8; 32] = [0u8; 32];
        nonce.copy_from_slice(&bytes[32..64]);

        Ok(SecretKey{ key, nonce })
    }

    /// Generate an "unbiased" `SecretKey` directly from a user
    /// suplied `csprng` uniformly, bypassing the `MiniSecretKey`
    /// layer.
    pub fn generate_with<R>(mut csprng: R) -> SecretKey
    where R: CryptoRng + RngCore,
    {
        let mut key: [u8; 64] = [0u8; 64];
        csprng.fill_bytes(&mut key);
        let mut nonce: [u8; 32] = [0u8; 32];
        csprng.fill_bytes(&mut nonce);
        SecretKey { key: Scalar::from_bytes_mod_order_wide(&key), nonce }
    }

    /// Generate an "unbiased" `SecretKey` directly,
    /// bypassing the `MiniSecretKey` layer.
    #[cfg(feature = "getrandom")]
    pub fn generate() -> SecretKey {
        Self::generate_with(super::getrandom_or_panic())
    }

    /// Derive the `PublicKey` corresponding to this `SecretKey`.
    pub fn to_public(&self) -> PublicKey {
        // No clamping necessary in the ristretto255 group
        PublicKey::from_point(&self.key * constants::RISTRETTO_BASEPOINT_TABLE)
    }

    /// Derive the `PublicKey` corresponding to this `SecretKey`.
    pub fn to_keypair(self) -> Keypair {
        let public = self.to_public();
        Keypair { secret: self, public }
    }
}

serde_boilerplate!(SecretKey);


/// A Ristretto Schnorr public key.
///
/// Internally, these are represented as a `RistrettoPoint`, meaning
/// an Edwards point with a static guarantee to be 2-torsion free.
///
/// At present, we decompress `PublicKey`s into this representation
/// during deserialization, which improves error handling, but costs
/// a compression during signing and verification.
#[derive(Copy, Clone, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct PublicKey(pub (crate) RistrettoBoth);

impl Debug for PublicKey {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "PublicKey( {:?} )", self.0)
    }
}

impl ConstantTimeEq for PublicKey {
    fn ct_eq(&self, other: &PublicKey) -> Choice {
        self.0.ct_eq(&other.0)
    }
}

/*
impl Zeroize for PublicKey {
    fn zeroize(&mut self) {
        self.0.zeroize()
    }
}
*/

// We should imho drop this impl but it benifits users who start with ring.
impl AsRef<[u8]> for PublicKey {
    fn as_ref(&self) -> &[u8] {
        self.as_compressed().as_bytes()
    }
}

impl PublicKey {
    const DESCRIPTION : &'static str = "A Ristretto Schnorr public key represented as a 32-byte Ristretto compressed point";

    /// Access the compressed Ristretto form
    pub fn as_compressed(&self) -> &CompressedRistretto { self.0.as_compressed() }

    /// Extract the compressed Ristretto form
    pub fn into_compressed(self) -> CompressedRistretto { self.0.into_compressed() }

    /// Access the point form
    pub fn as_point(&self) -> &RistrettoPoint { self.0.as_point() }

    /// Extract the point form
    pub fn into_point(self) -> RistrettoPoint { self.0.into_point() }

    /// Decompress into the `PublicKey` format that also retains the
    /// compressed form.
    pub fn from_compressed(compressed: CompressedRistretto) -> SignatureResult<PublicKey> {
        Ok(PublicKey(RistrettoBoth::from_compressed(compressed) ?))
    }

    /// Compress into the `PublicKey` format that also retains the
    /// uncompressed form.
    pub fn from_point(point: RistrettoPoint) -> PublicKey {
        PublicKey(RistrettoBoth::from_point(point))
    }

    /// Convert this public key to a byte array.
    /// # Example
    ///
    /// ```
    /// # #[cfg(feature = "getrandom")]
    /// # {
    /// use schnorrkel::{SecretKey, PublicKey, PUBLIC_KEY_LENGTH, SignatureError};
    ///
    /// let public_key: PublicKey = SecretKey::generate().to_public();
    /// let public_key_bytes = public_key.to_bytes();
    /// let public_key_again: PublicKey = PublicKey::from_bytes(&public_key_bytes[..]).unwrap();
    /// assert_eq!(public_key_bytes, public_key_again.to_bytes());
    /// # }
    /// ```
    #[inline]
    pub fn to_bytes(&self) -> [u8; PUBLIC_KEY_LENGTH] {
        self.as_compressed().to_bytes()
    }

    /// Construct a `PublicKey` from a slice of bytes.
    ///
    /// # Example
    ///
    /// ```
    /// use schnorrkel::{PublicKey, PUBLIC_KEY_LENGTH, SignatureError};
    ///
    /// let public_key_bytes: [u8; PUBLIC_KEY_LENGTH] = [
    ///     208, 120, 140, 129, 177, 179, 237, 159,
    ///     252, 160, 028, 013, 206, 005, 211, 241,
    ///     192, 218, 001, 097, 130, 241, 020, 169,
    ///     119, 046, 246, 029, 079, 080, 077, 084];
    ///
    /// let public_key = PublicKey::from_bytes(&public_key_bytes).unwrap();
    /// assert_eq!(public_key.to_bytes(), public_key_bytes);
    /// ```
    ///
    /// # Returns
    ///
    /// A `Result` whose okay value is an EdDSA `PublicKey` or whose error value
    /// is an `SignatureError` describing the error that occurred.
    #[inline]
    pub fn from_bytes(bytes: &[u8]) -> SignatureResult<PublicKey> {
        Ok(PublicKey(RistrettoBoth::from_bytes_ser("PublicKey",PublicKey::DESCRIPTION,bytes) ?))
    }
}

impl From<SecretKey> for PublicKey {
    fn from(source: SecretKey) -> PublicKey {
        source.to_public()
    }
}

serde_boilerplate!(PublicKey);


/// A Ristretto Schnorr keypair.
#[derive(Clone,Debug)]
// #[derive(Clone,Zeroize)]
// #[zeroize(drop)]
pub struct Keypair {
    /// The secret half of this keypair.
    pub secret: SecretKey,
    /// The public half of this keypair.
    pub public: PublicKey,
}

impl Zeroize for Keypair {
    fn zeroize(&mut self) {
        self.secret.zeroize();
    }
}
impl Drop for Keypair {
    fn drop(&mut self) {
        self.zeroize();
    }
}

impl From<SecretKey> for Keypair {
    fn from(secret: SecretKey) -> Keypair {
        let public = secret.to_public();
        Keypair{ secret, public }
    }
}

impl Keypair {
    const DESCRIPTION : &'static str = "A 96 bytes Ristretto Schnorr keypair";
    /*
    const DESCRIPTION_LONG : &'static str =
        "An ristretto schnorr keypair, 96 bytes in total, where the \
        first 64 bytes contains the secret key represented as an \
        ed25519 expanded secret key, as specified in RFC8032, and \
        the subsequent 32 bytes gives the public key as a compressed \
        ristretto point.";
    */

    /// Serialize `Keypair` to bytes.
    ///
    /// # Returns
    ///
    /// A byte array `[u8; KEYPAIR_LENGTH]` consisting of first a
    /// `SecretKey` serialized canonically, and next the Ristterro
    /// `PublicKey`
    ///
    /// # Examples
    ///
    /// ```
    /// # #[cfg(feature = "getrandom")]
    /// # {
    /// use schnorrkel::{Keypair, KEYPAIR_LENGTH};
    ///
    /// let keypair: Keypair = Keypair::generate();
    /// let bytes: [u8; KEYPAIR_LENGTH] = keypair.to_bytes();
    /// let keypair_too = Keypair::from_bytes(&bytes[..]).unwrap();
    /// assert_eq!(&bytes[..], & keypair_too.to_bytes()[..]);
    /// # }
    /// ```
    pub fn to_bytes(&self) -> [u8; KEYPAIR_LENGTH] {
        let mut bytes: [u8; KEYPAIR_LENGTH] = [0u8; KEYPAIR_LENGTH];

        bytes[..SECRET_KEY_LENGTH].copy_from_slice(& self.secret.to_bytes());
        bytes[SECRET_KEY_LENGTH..].copy_from_slice(& self.public.to_bytes());
        bytes
    }

    /// Deserialize a `Keypair` from bytes.
    ///
    /// # Inputs
    ///
    /// * `bytes`: an `&[u8]` consisting of byte representations of
    /// first a `SecretKey` and then the corresponding ristretto
    /// `PublicKey`.
    ///
    /// # Examples
    ///
    /// ```
    /// use schnorrkel::{Keypair, KEYPAIR_LENGTH};
    /// use hex_literal::hex;
    ///
    /// // TODO: Fix test vector
    /// // let keypair_bytes = hex!("28b0ae221c6bb06856b287f60d7ea0d98552ea5a16db16956849aa371db3eb51fd190cce74df356432b410bd64682309d6dedb27c76845daf388557cbac3ca3446ebddef8cd9bb167dc30878d7113b7e168e6f0646beffd77d69d39bad76b47a");
    /// // let keypair: Keypair = Keypair::from_bytes(&keypair_bytes[..]).unwrap();
    /// // assert_eq!(&keypair_bytes[..], & keypair.to_bytes()[..]);
    /// ```
    ///
    /// # Returns
    ///
    /// A `Result` whose okay value is an EdDSA `Keypair` or whose error value
    /// is an `SignatureError` describing the error that occurred.
    pub fn from_bytes(bytes: &[u8]) -> SignatureResult<Keypair> {
        if bytes.len() != KEYPAIR_LENGTH {
            return Err(SignatureError::BytesLengthError {
                name: "Keypair",
                description: Keypair::DESCRIPTION,
                length: KEYPAIR_LENGTH
            });
        }
        let secret = SecretKey::from_bytes(&bytes[..SECRET_KEY_LENGTH]) ?;
        let public = PublicKey::from_bytes(&bytes[SECRET_KEY_LENGTH..]) ?;

        Ok(Keypair{ secret, public })
    }

    /// Serialize `Keypair` to bytes with Ed25519 secret key format.
    ///
    /// # Returns
    ///
    /// A byte array `[u8; KEYPAIR_LENGTH]` consisting of first a
    /// `SecretKey` serialized like Ed25519, and next the Ristterro
    /// `PublicKey`
    ///
    ///
    pub fn to_half_ed25519_bytes(&self) -> [u8; KEYPAIR_LENGTH] {
        let mut bytes: [u8; KEYPAIR_LENGTH] = [0u8; KEYPAIR_LENGTH];

        bytes[..SECRET_KEY_LENGTH].copy_from_slice(& self.secret.to_ed25519_bytes());
        bytes[SECRET_KEY_LENGTH..].copy_from_slice(& self.public.to_bytes());
        bytes
    }

    /// Deserialize a `Keypair` from bytes with Ed25519 style `SecretKey` format.
    ///
    /// # Inputs
    ///
    /// * `bytes`: an `&[u8]` representing the scalar for the secret key, and a
    ///   compressed Ristretto point, both as bytes.
    ///
    /// # Examples
    ///
    /// ```
    /// use schnorrkel::{Keypair, KEYPAIR_LENGTH};
    /// use hex_literal::hex;
    ///
    /// let keypair_bytes = hex!("28b0ae221c6bb06856b287f60d7ea0d98552ea5a16db16956849aa371db3eb51fd190cce74df356432b410bd64682309d6dedb27c76845daf388557cbac3ca3446ebddef8cd9bb167dc30878d7113b7e168e6f0646beffd77d69d39bad76b47a");
    /// let keypair: Keypair = Keypair::from_half_ed25519_bytes(&keypair_bytes[..]).unwrap();
    /// assert_eq!(&keypair_bytes[..], & keypair.to_half_ed25519_bytes()[..]);
    /// ```
    ///
    /// # Returns
    ///
    /// A `Result` whose okay value is an EdDSA `Keypair` or whose error value
    /// is an `SignatureError` describing the error that occurred.
    pub fn from_half_ed25519_bytes(bytes: &[u8]) -> SignatureResult<Keypair> {
        if bytes.len() != KEYPAIR_LENGTH {
            return Err(SignatureError::BytesLengthError {
                name: "Keypair",
                description: Keypair::DESCRIPTION,
                length: KEYPAIR_LENGTH
            });
        }
        let secret = SecretKey::from_ed25519_bytes(&bytes[..SECRET_KEY_LENGTH]) ?;
        let public = PublicKey::from_bytes(&bytes[SECRET_KEY_LENGTH..]) ?;

        Ok(Keypair{ secret, public })
    }

    /// Generate a Ristretto Schnorr `Keypair` directly,
    /// bypassing the `MiniSecretKey` layer.
    ///
    /// # Example
    ///
    /// ```
    /// # fn main() {
    ///
    /// use rand::{Rng, rngs::OsRng};
    /// # #[cfg(feature = "getrandom")]
    /// use schnorrkel::Keypair;
    /// use schnorrkel::Signature;
    ///
    /// # #[cfg(feature = "getrandom")]
    /// let keypair: Keypair = Keypair::generate_with(OsRng);
    ///
    /// # }
    /// ```
    ///
    /// # Input
    ///
    /// A CSPRNG with a `fill_bytes()` method, e.g. `rand_chacha::ChaChaRng`.
    ///
    /// We generate a `SecretKey` directly bypassing `MiniSecretKey`,
    /// so our secret keys do not satisfy the high bit "clamping"
    /// imposed on Ed25519 keys.
    pub fn generate_with<R>(csprng: R) -> Keypair
    where R: CryptoRng + RngCore,
    {
        let secret: SecretKey = SecretKey::generate_with(csprng);
        let public: PublicKey = secret.to_public();

        Keypair{ public, secret }
    }

    /// Generate a Ristretto Schnorr `Keypair` directly, from a user
    /// suplied `csprng`, bypassing the `MiniSecretKey` layer.
    #[cfg(feature = "getrandom")]
    pub fn generate() -> Keypair {
        Self::generate_with(super::getrandom_or_panic())
    }
}

serde_boilerplate!(Keypair);


#[cfg(test)]
mod test {
    // use std::vec::Vec;
    use super::*;

    /*
    TODO: Use some Ristretto point to do this test correctly.
    use curve25519_dalek::edwards::{CompressedEdwardsY};  // EdwardsPoint
    #[test]
    fn public_key_from_bytes() {
        static ED25519_PUBLIC_KEY : CompressedEdwardsY = CompressedEdwardsY([
            215, 090, 152, 001, 130, 177, 010, 183,
            213, 075, 254, 211, 201, 100, 007, 058,
            014, 225, 114, 243, 218, 166, 035, 037,
            175, 002, 026, 104, 247, 007, 081, 026, ]);
        let pk = ED25519_PUBLIC_KEY.decompress().unwrap();
        // let pk = unsafe { std::mem::transmute::<EdwardsPoint,RistrettoPoint>(pk) };
        let point = super::super::ed25519::edwards_to_ristretto(pk).unwrap();
        let ristretto_public_key = PublicKey::from_point(point);

        assert_eq!(
            ristretto_public_key.to_ed25519_public_key_bytes(),
            pk.mul_by_cofactor().compress().0
        );

        // Make another function so that we can test the ? operator.
        fn do_the_test(s: &[u8]) -> Result<PublicKey, SignatureError> {
            let public_key = PublicKey::from_bytes(s) ?;
            Ok(public_key)
        }
        assert_eq!(
            do_the_test(ristretto_public_key.as_ref()),
            Ok(ristretto_public_key)
        );
        assert_eq!(
            do_the_test(&ED25519_PUBLIC_KEY.0),  // Not a Ristretto public key
            Err(SignatureError::PointDecompressionError)
        );
    }
    */

    #[test]
    fn derives_from_core() {
        let pk_d = PublicKey::default();
        debug_assert_eq!(
            pk_d.as_point().compress(),
            CompressedRistretto::default()
        );
        debug_assert_eq!(
            pk_d.as_compressed().decompress().unwrap(),
            RistrettoPoint::default()
        );
    }

    #[cfg(feature = "getrandom")]
    #[test]
    fn keypair_zeroize() {
        let mut csprng = rand_core::OsRng;

        let mut keypair = Keypair::generate_with(&mut csprng);

        keypair.zeroize();

        fn as_bytes<T>(x: &T) -> &[u8] {
            use core::mem;
            use core::slice;

            unsafe {
                slice::from_raw_parts(x as *const T as *const u8, mem::size_of_val(x))
            }
        }

        assert!(!as_bytes(&keypair).iter().all(|x| *x == 0u8));
    }

    #[cfg(feature = "getrandom")]
    #[test]
    fn pubkey_from_mini_secret_and_expanded_secret() {
        let mut csprng = rand_core::OsRng;

        let mini_secret: MiniSecretKey = MiniSecretKey::generate_with(&mut csprng);
        let secret: SecretKey = mini_secret.expand(ExpansionMode::Ed25519);
        let public_from_mini_secret: PublicKey = mini_secret.expand_to_public(ExpansionMode::Ed25519);
        let public_from_secret: PublicKey = secret.to_public();
        assert!(public_from_mini_secret == public_from_secret);
        let secret: SecretKey = mini_secret.expand(ExpansionMode::Uniform);
        let public_from_mini_secret: PublicKey = mini_secret.expand_to_public(ExpansionMode::Uniform);
        let public_from_secret: PublicKey = secret.to_public();
        assert!(public_from_mini_secret == public_from_secret);
    }

    #[cfg(feature = "getrandom")]
    #[test]
    fn secret_key_can_be_converted_to_ed25519_bytes_and_back() {
        let count = if cfg!(debug_assertions) {
            200000
        } else {
            2000000
        };

        for _ in 0..count {
            let key = SecretKey::generate();
            let bytes = key.to_ed25519_bytes();
            let key_deserialized = SecretKey::from_ed25519_bytes(&bytes).unwrap();
            assert_eq!(key_deserialized, key);
        }
    }
}
