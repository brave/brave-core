use std::io;

use ff::{PrimeField, PrimeFieldBits};
use group::Curve;
use rand_core::{CryptoRng, RngCore};

#[cfg(feature = "pairing")]
use bls12_381::{hash_to_curve::HashToField, G1Affine, G1Projective, Scalar};
#[cfg(feature = "pairing")]
use hkdf::Hkdf;
#[cfg(feature = "pairing")]
use sha2::{digest::generic_array::typenum::U48, digest::generic_array::GenericArray, Sha256};

#[cfg(feature = "blst")]
use blstrs::{G1Affine, G1Projective, G2Affine, Scalar};
#[cfg(feature = "blst")]
use group::prime::PrimeCurveAffine;

pub(crate) struct ScalarRepr(<Scalar as PrimeFieldBits>::ReprBits);

use crate::error::Error;
use crate::signature::*;

pub(crate) const G1_COMPRESSED_SIZE: usize = 48;

#[derive(Debug, Copy, Clone, PartialEq)]
pub struct PublicKey(pub(crate) G1Projective);

#[derive(Debug, Copy, Clone, PartialEq)]
pub struct PrivateKey(pub(crate) Scalar);

impl From<G1Projective> for PublicKey {
    fn from(val: G1Projective) -> Self {
        PublicKey(val)
    }
}
impl From<PublicKey> for G1Projective {
    fn from(val: PublicKey) -> Self {
        val.0
    }
}

impl From<Scalar> for PrivateKey {
    fn from(val: Scalar) -> Self {
        PrivateKey(val)
    }
}

impl From<PrivateKey> for Scalar {
    fn from(val: PrivateKey) -> Self {
        val.0
    }
}

impl From<PrivateKey> for ScalarRepr {
    fn from(val: PrivateKey) -> Self {
        ScalarRepr(val.0.to_le_bits().into_inner())
    }
}

impl<'a> From<&'a PrivateKey> for ScalarRepr {
    fn from(val: &'a PrivateKey) -> Self {
        (*val).into()
    }
}

pub trait Serialize: ::std::fmt::Debug + Sized {
    /// Writes the key to the given writer.
    fn write_bytes(&self, dest: &mut impl io::Write) -> io::Result<()>;

    /// Recreate the key from bytes in the same form as `write_bytes` produced.
    fn from_bytes(raw: &[u8]) -> Result<Self, Error>;

    fn as_bytes(&self) -> Vec<u8> {
        let mut res = Vec::with_capacity(8 * 4);
        self.write_bytes(&mut res).expect("preallocated");
        res
    }
}

impl PrivateKey {
    /// Generate a deterministic private key from the given bytes.
    ///
    /// They must be at least 32 bytes long to be secure, will panic otherwise.
    pub fn new<T: AsRef<[u8]>>(msg: T) -> Self {
        PrivateKey(key_gen(msg))
    }

    /// Generate a new private key.
    pub fn generate<R: RngCore + CryptoRng>(rng: &mut R) -> Self {
        // IKM must be at least 32 bytes long:
        // https://tools.ietf.org/html/draft-irtf-cfrg-bls-signature-00#section-2.3
        let mut ikm = [0u8; 32];
        rng.try_fill_bytes(&mut ikm)
            .expect("unable to produce secure randomness");

        Self::new(ikm)
    }

    /// Sign the given message.
    /// Calculated by `signature = hash_into_g2(message) * sk`
    #[cfg(feature = "pairing")]
    pub fn sign<T: AsRef<[u8]>>(&self, message: T) -> Signature {
        let mut p = hash(message.as_ref());
        p *= self.0;

        p.into()
    }

    /// Sign the given message.
    /// Calculated by `signature = hash_into_g2(message) * sk`
    #[cfg(feature = "blst")]
    pub fn sign<T: AsRef<[u8]>>(&self, message: T) -> Signature {
        let p = hash(message.as_ref());
        let mut sig = G2Affine::identity();

        unsafe {
            blst_lib::blst_sign_pk2_in_g1(
                std::ptr::null_mut(),
                sig.as_mut(),
                p.as_ref(),
                &self.0.into(),
            );
        }

        sig.into()
    }

    /// Get the public key for this private key.
    /// Calculated by `pk = g1 * sk`.
    #[cfg(feature = "pairing")]
    pub fn public_key(&self) -> PublicKey {
        let mut pk = G1Projective::generator();
        pk *= self.0;

        PublicKey(pk)
    }

    /// Get the public key for this private key.
    /// Calculated by `pk = g1 * sk`.
    #[cfg(feature = "blst")]
    pub fn public_key(&self) -> PublicKey {
        let mut pk = G1Affine::identity();

        unsafe {
            blst_lib::blst_sk_to_pk2_in_g1(std::ptr::null_mut(), pk.as_mut(), &self.0.into());
        }

        PublicKey(pk.into())
    }

    /// Deserializes a private key from the field element as a decimal number.
    pub fn from_string<T: AsRef<str>>(s: T) -> Result<Self, Error> {
        match Scalar::from_str_vartime(s.as_ref()) {
            Some(f) => Ok(f.into()),
            None => Err(Error::InvalidPrivateKey),
        }
    }
}

impl Serialize for PrivateKey {
    fn write_bytes(&self, dest: &mut impl io::Write) -> io::Result<()> {
        for digit in &self.0.to_le_bits().data {
            dest.write_all(&digit.to_le_bytes())?;
        }

        Ok(())
    }

    fn from_bytes(raw: &[u8]) -> Result<Self, Error> {
        const FR_SIZE: usize = (Scalar::NUM_BITS as usize + 8 - 1) / 8;
        if raw.len() != FR_SIZE {
            return Err(Error::SizeMismatch);
        }

        let mut res = [0u8; FR_SIZE];
        res.copy_from_slice(&raw[..FR_SIZE]);

        // TODO: once zero keys are rejected, insert check for zero.

        Scalar::from_repr_vartime(res)
            .map(Into::into)
            .ok_or(Error::InvalidPrivateKey)
    }
}

impl PublicKey {
    pub fn as_affine(&self) -> G1Affine {
        self.0.to_affine()
    }

    pub fn verify<T: AsRef<[u8]>>(&self, sig: Signature, message: T) -> bool {
        verify_messages(&sig, &[message.as_ref()], &[*self])
    }
}

impl Serialize for PublicKey {
    fn write_bytes(&self, dest: &mut impl io::Write) -> io::Result<()> {
        let t = self.0.to_affine();
        let tmp = t.to_compressed();
        dest.write_all(tmp.as_ref())?;

        Ok(())
    }

    fn from_bytes(raw: &[u8]) -> Result<Self, Error> {
        if raw.len() != G1_COMPRESSED_SIZE {
            return Err(Error::SizeMismatch);
        }

        let mut res = [0u8; G1_COMPRESSED_SIZE];
        res.as_mut().copy_from_slice(raw);
        let affine: G1Affine =
            Option::from(G1Affine::from_compressed(&res)).ok_or(Error::GroupDecode)?;

        Ok(PublicKey(affine.into()))
    }
}

/// Generates a secret key as defined in
/// https://tools.ietf.org/html/draft-irtf-cfrg-bls-signature-02#section-2.3
#[cfg(feature = "pairing")]
fn key_gen<T: AsRef<[u8]>>(data: T) -> Scalar {
    // "BLS-SIG-KEYGEN-SALT-"
    const SALT: &[u8] = b"BLS-SIG-KEYGEN-SALT-";

    let data = data.as_ref();
    assert!(data.len() >= 32, "IKM must be at least 32 bytes");

    // HKDF-Extract
    let mut msg = data.as_ref().to_vec();
    // append zero byte
    msg.push(0);
    let prk = Hkdf::<Sha256>::new(Some(SALT), &msg);

    // HKDF-Expand
    // `result` has enough length to hold the output from HKDF expansion
    let mut result = GenericArray::<u8, U48>::default();
    assert!(prk.expand(&[0, 48], &mut result).is_ok());

    Scalar::from_okm(&result)
}

/// Generates a secret key as defined in
/// https://tools.ietf.org/html/draft-irtf-cfrg-bls-signature-02#section-2.3
#[cfg(feature = "blst")]
fn key_gen<T: AsRef<[u8]>>(data: T) -> Scalar {
    use std::convert::TryInto;

    let data = data.as_ref();
    assert!(data.len() >= 32, "IKM must be at least 32 bytes");

    let key_info = &[];
    let mut out = blst_lib::blst_scalar::default();
    unsafe {
        blst_lib::blst_keygen(
            &mut out,
            data.as_ptr(),
            data.len(),
            key_info.as_ptr(),
            key_info.len(),
        )
    };

    out.try_into().expect("invalid key generated")
}

#[cfg(test)]
mod tests {
    use super::*;

    use rand::SeedableRng;
    use rand_chacha::ChaCha8Rng;

    #[test]
    fn test_bytes_roundtrip() {
        let rng = &mut ChaCha8Rng::seed_from_u64(12);
        let sk = PrivateKey::generate(rng);
        let sk_bytes = sk.as_bytes();

        assert_eq!(sk_bytes.len(), 32);
        assert_eq!(PrivateKey::from_bytes(&sk_bytes).unwrap(), sk);

        let pk = sk.public_key();
        let pk_bytes = pk.as_bytes();

        assert_eq!(pk_bytes.len(), 48);
        assert_eq!(PublicKey::from_bytes(&pk_bytes).unwrap(), pk);
    }

    #[test]
    fn test_key_gen() {
        let key_material = "hello world (it's a secret!) very secret stuff";
        let fr_val = key_gen(key_material);
        #[cfg(feature = "blst")]
        let expect = Scalar::from_u64s_le(&[
            0x8a223b0f9e257f7d,
            0x2d80f7b7f5ea6cc4,
            0xcc9e063a0ea0009c,
            0x4a73baed5cb75109,
        ])
        .unwrap();

        #[cfg(feature = "pairing")]
        let expect = Scalar::from_raw([
            0xa9f8187b89e6d49a,
            0xf870f34063ce4b16,
            0xc2aa3c1fff1bbaa3,
            0x60417787ee46e23f,
        ]);

        assert_eq!(fr_val, expect);
    }

    #[test]
    fn test_sig() {
        let msg = "this is the message";
        let sk = "this is the key and it is very secret";

        let sk = PrivateKey::new(sk);
        let sig = sk.sign(msg);
        let pk = sk.public_key();

        assert!(pk.verify(sig, msg));
    }

    #[test]
    fn test_from_bytes() {
        // Larger than the modulus
        assert!(PrivateKey::from_bytes(&[255u8; 32]).is_err());

        // Scalar field modulus' bigint (i.e. non-Montgomery form) little-endian bytes.
        let modulus_repr: [u8; 32] = [
            0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x5b, 0xfe, 0xff, 0x02, 0xa4,
            0xbd, 0x53, 0x05, 0xd8, 0xa1, 0x09, 0x08, 0xd8, 0x39, 0x33, 0x48, 0x7d, 0x9d, 0x29,
            0x53, 0xa7, 0xed, 0x73,
        ];

        // Largest field element.
        let neg1_repr: [u8; 32] = {
            let mut repr = modulus_repr;
            repr[0] -= 1;
            repr
        };
        assert!(PrivateKey::from_bytes(&neg1_repr).is_ok());

        // Smallest integer greater than the modulus.
        let modulus_plus_1_repr = {
            let mut repr = modulus_repr;
            repr[0] += 1;
            repr
        };
        assert!(PrivateKey::from_bytes(&modulus_plus_1_repr).is_err());

        // simple numbers below the modulus
        assert!(PrivateKey::from_bytes(&Scalar::from(1).to_repr()).is_ok());
        assert!(PrivateKey::from_bytes(&Scalar::from(10).to_repr()).is_ok());
        assert!(PrivateKey::from_bytes(&Scalar::from(100).to_repr()).is_ok());

        // Larger than the modulus
        assert!(PublicKey::from_bytes(&[255u8; 48]).is_err());
    }
}
