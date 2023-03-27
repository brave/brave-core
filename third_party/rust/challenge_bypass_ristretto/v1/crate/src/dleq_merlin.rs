#[cfg(all(feature = "alloc", not(feature = "std")))]
use alloc::vec::Vec;

#[cfg(all(feature = "std"))]
use std::vec::Vec;

use core::iter;

use curve25519_dalek::ristretto::CompressedRistretto;
use curve25519_dalek::ristretto::RistrettoPoint;
use curve25519_dalek::scalar::Scalar;
use curve25519_dalek::traits::VartimeMultiscalarMul;
use merlin::Transcript;
use rand;

use crate::errors::{InternalError, TokenError};
use crate::voprf::{BlindedToken, PublicKey, SignedToken, SigningKey};
use curve25519_dalek::constants;

use rand_chacha::ChaChaRng;
use rand_core::SeedableRng;

/// The length of a `DLEQProof`, in bytes.
pub const DLEQ_PROOF_LENGTH: usize = 64;

trait TranscriptProtocol {
    fn dleq_domain_sep(&mut self);
    fn batch_dleq_domain_sep(&mut self);
    fn commit_point(&mut self, label: &'static [u8], point: &CompressedRistretto);
    fn challenge_scalar(&mut self, label: &'static [u8]) -> Scalar;
}

impl TranscriptProtocol for Transcript {
    fn dleq_domain_sep(&mut self) {
        self.append_message(b"dom-sep", b"dleq");
    }

    fn batch_dleq_domain_sep(&mut self) {
        self.append_message(b"dom-sep", b"batch-dleq");
    }

    fn commit_point(&mut self, label: &'static [u8], point: &CompressedRistretto) {
        self.append_message(label, point.as_bytes());
    }

    fn challenge_scalar(&mut self, label: &'static [u8]) -> Scalar {
        let mut buf = [0; 64];
        self.challenge_bytes(label, &mut buf);
        Scalar::from_bytes_mod_order_wide(&buf)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use crate::voprf::Token;
    use rand::rngs::OsRng;
    use sha2::Sha512;

    #[test]
    #[allow(non_snake_case)]
    fn dleq_proof_works() {
        let mut rng = OsRng;

        let key1 = SigningKey::random(&mut rng);
        let key2 = SigningKey::random(&mut rng);

        let P = RistrettoPoint::random(&mut rng);
        let Q = key1.k * P;

        let mut verifier = Transcript::new(b"dleqtest");
        let proof = DLEQProof::_new(&mut verifier, P, Q, &key1).unwrap();

        let mut verifier = Transcript::new(b"dleqtest");
        assert!(proof._verify(&mut verifier, P, Q, &key1.public_key).is_ok());

        let P = RistrettoPoint::random(&mut rng);
        let Q = key2.k * P;

        let mut transcript = Transcript::new(b"dleqtest");
        let proof = DLEQProof::_new(&mut transcript, P, Q, &key1).unwrap();

        let mut transcript = Transcript::new(b"dleqtest");
        assert!(!proof
            ._verify(&mut transcript, P, Q, &key1.public_key)
            .is_ok());
    }

    #[test]
    #[allow(non_snake_case)]
    fn batch_dleq_proof_works() {
        use std::vec::Vec;

        let mut rng = OsRng;

        let key = SigningKey::random(&mut rng);

        let blinded_tokens = vec![Token::random::<Sha512, OsRng>(&mut rng).blind()];
        let signed_tokens: Vec<SignedToken> = blinded_tokens
            .iter()
            .filter_map(|t| key.sign(t).ok())
            .collect();

        let mut transcript = Transcript::new(b"batchdleqtest");
        let batch_proof =
            BatchDLEQProof::new(&mut transcript, &blinded_tokens, &signed_tokens, &key).unwrap();

        let mut transcript = Transcript::new(b"batchdleqtest");
        assert!(batch_proof
            .verify(
                &mut transcript,
                &blinded_tokens,
                &signed_tokens,
                &key.public_key
            )
            .is_ok());
    }
}

/// A `DLEQProof` is a proof of the equivalence of the discrete logarithm between two pairs of points.
#[allow(non_snake_case)]
pub struct DLEQProof {
    /// `c` is a `Scalar`
    /// \\(c=H_3(X,Y,P,Q,A,B)\\)
    pub(crate) c: Scalar,
    /// `s` is a `Scalar`
    /// \\(s = (t - ck) \mod q\\)
    pub(crate) s: Scalar,
}

#[cfg(feature = "base64")]
impl_base64!(DLEQProof);

#[cfg(feature = "serde")]
impl_serde!(DLEQProof);

#[allow(non_snake_case)]
impl DLEQProof {
    /// Construct a new `DLEQProof`
    fn _new(
        transcript: &mut Transcript,
        P: RistrettoPoint,
        Q: RistrettoPoint,
        secret_key: &SigningKey,
    ) -> Result<Self, TokenError> {
        let X = constants::RISTRETTO_BASEPOINT_COMPRESSED;
        let Y = secret_key.public_key.0;

        transcript.dleq_domain_sep();

        transcript.commit_point(b"X", &X);
        transcript.commit_point(b"Y", &Y);
        transcript.commit_point(b"P", &P.compress());
        transcript.commit_point(b"Q", &Q.compress());

        let mut rng = transcript
            .build_rng()
            .rekey_with_witness_bytes(b"k", secret_key.k.as_bytes())
            .finalize(&mut ChaChaRng::from_seed([0; 32]));

        let t = Scalar::random(&mut rng);

        let A = t * X
            .decompress()
            .ok_or(TokenError(InternalError::PointDecompressionError))?;
        let B = t * P;

        transcript.commit_point(b"A", &A.compress());
        transcript.commit_point(b"B", &B.compress());

        let c = transcript.challenge_scalar(b"c");

        let s = t - c * secret_key.k;

        Ok(DLEQProof { c, s })
    }

    /// Verify the `DLEQProof`
    fn _verify(
        &self,
        transcript: &mut Transcript,
        P: RistrettoPoint,
        Q: RistrettoPoint,
        public_key: &PublicKey,
    ) -> Result<(), TokenError> {
        let X = constants::RISTRETTO_BASEPOINT_COMPRESSED;
        let Y = public_key.0;

        transcript.dleq_domain_sep();

        let A = (&self.s * &constants::RISTRETTO_BASEPOINT_TABLE)
            + (self.c
                * Y.decompress()
                    .ok_or(TokenError(InternalError::PointDecompressionError))?);
        let B = (self.s * P) + (self.c * Q);

        let P = P.compress();
        let Q = Q.compress();
        let A = A.compress();
        let B = B.compress();

        transcript.commit_point(b"X", &X);
        transcript.commit_point(b"Y", &Y);
        transcript.commit_point(b"P", &P);
        transcript.commit_point(b"Q", &Q);
        transcript.commit_point(b"A", &A);
        transcript.commit_point(b"B", &B);

        let c = transcript.challenge_scalar(b"c");

        if c == self.c {
            Ok(())
        } else {
            Err(TokenError(InternalError::VerifyError))
        }
    }
}

impl DLEQProof {
    /// Convert this `DLEQProof` to a byte array.
    pub fn to_bytes(&self) -> [u8; DLEQ_PROOF_LENGTH] {
        let mut proof_bytes: [u8; DLEQ_PROOF_LENGTH] = [0u8; DLEQ_PROOF_LENGTH];

        proof_bytes[..32].copy_from_slice(&self.c.to_bytes());
        proof_bytes[32..].copy_from_slice(&self.s.to_bytes());
        proof_bytes
    }

    fn bytes_length_error() -> TokenError {
        TokenError(InternalError::BytesLengthError {
            name: "DLEQProof",
            length: DLEQ_PROOF_LENGTH,
        })
    }

    /// Construct a `DLEQProof` from a slice of bytes.
    pub fn from_bytes(bytes: &[u8]) -> Result<DLEQProof, TokenError> {
        if bytes.len() != DLEQ_PROOF_LENGTH {
            return Err(DLEQProof::bytes_length_error());
        }

        let mut c_bits: [u8; 32] = [0u8; 32];
        let mut s_bits: [u8; 32] = [0u8; 32];

        c_bits.copy_from_slice(&bytes[..32]);
        s_bits.copy_from_slice(&bytes[32..]);

        let c = Scalar::from_canonical_bytes(c_bits)
            .ok_or(TokenError(InternalError::ScalarFormatError))?;
        let s = Scalar::from_canonical_bytes(s_bits)
            .ok_or(TokenError(InternalError::ScalarFormatError))?;

        Ok(DLEQProof { c, s })
    }
}

/// A `BatchDLEQProof` is a proof of the equivalence of the discrete logarithm between a common
/// pair of points and one or more other pairs of points.
#[allow(non_snake_case)]
pub struct BatchDLEQProof(DLEQProof);

#[cfg(feature = "base64")]
impl_base64!(BatchDLEQProof);

#[cfg(feature = "serde")]
impl_serde!(BatchDLEQProof);

#[allow(non_snake_case)]
impl BatchDLEQProof {
    fn calculate_composites(
        transcript: &mut Transcript,
        blinded_tokens: &[BlindedToken],
        signed_tokens: &[SignedToken],
        public_key: &PublicKey,
    ) -> Result<(RistrettoPoint, RistrettoPoint), TokenError> {
        if blinded_tokens.len() != signed_tokens.len() {
            return Err(TokenError(InternalError::LengthMismatchError));
        }

        transcript.commit_point(b"X", &constants::RISTRETTO_BASEPOINT_COMPRESSED);
        transcript.commit_point(b"Y", &public_key.0);

        for (Pi, Qi) in blinded_tokens.iter().zip(signed_tokens.iter()) {
            transcript.commit_point(b"Pi", &Pi.0);
            transcript.commit_point(b"Qi", &Qi.0);
        }

        let c_m: Vec<Scalar> = iter::repeat_with(|| transcript.challenge_scalar(b"c_i"))
            .take(blinded_tokens.len())
            .collect();

        let M = RistrettoPoint::optional_multiscalar_mul(
            &c_m,
            blinded_tokens.iter().map(|Pi| Pi.0.decompress()),
        )
        .ok_or(TokenError(InternalError::PointDecompressionError))?;

        let Z = RistrettoPoint::optional_multiscalar_mul(
            &c_m,
            signed_tokens.iter().map(|Qi| Qi.0.decompress()),
        )
        .ok_or(TokenError(InternalError::PointDecompressionError))?;

        Ok((M, Z))
    }

    /// Construct a new `BatchDLEQProof`
    pub fn new(
        transcript: &mut Transcript,
        blinded_tokens: &[BlindedToken],
        signed_tokens: &[SignedToken],
        signing_key: &SigningKey,
    ) -> Result<Self, TokenError> {
        transcript.dleq_domain_sep();

        let (M, Z) = BatchDLEQProof::calculate_composites(
            transcript,
            blinded_tokens,
            signed_tokens,
            &signing_key.public_key,
        )?;

        Ok(BatchDLEQProof(DLEQProof::_new(
            transcript,
            M,
            Z,
            signing_key,
        )?))
    }

    /// Verify a `BatchDLEQProof`
    pub fn verify(
        &self,
        transcript: &mut Transcript,
        blinded_tokens: &[BlindedToken],
        signed_tokens: &[SignedToken],
        public_key: &PublicKey,
    ) -> Result<(), TokenError> {
        transcript.dleq_domain_sep();

        let (M, Z) = BatchDLEQProof::calculate_composites(
            transcript,
            blinded_tokens,
            signed_tokens,
            public_key,
        )?;

        self.0._verify(transcript, M, Z, public_key)
    }
}

impl BatchDLEQProof {
    /// Convert this `BatchDLEQProof` to a byte array.
    pub fn to_bytes(&self) -> [u8; DLEQ_PROOF_LENGTH] {
        self.0.to_bytes()
    }

    #[cfg(feature = "serde")]
    fn bytes_length_error() -> TokenError {
        DLEQProof::bytes_length_error()
    }

    /// Construct a `BatchDLEQProof` from a slice of bytes.
    pub fn from_bytes(bytes: &[u8]) -> Result<BatchDLEQProof, TokenError> {
        DLEQProof::from_bytes(bytes).map(BatchDLEQProof)
    }
}
