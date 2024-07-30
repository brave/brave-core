#[cfg(all(feature = "alloc", not(feature = "std")))]
use alloc::vec::Vec;

#[cfg(all(feature = "std"))]
use std::vec::Vec;

use core::iter;

use curve25519_dalek::constants;
use curve25519_dalek::ristretto::RistrettoPoint;
use curve25519_dalek::scalar::Scalar;
use curve25519_dalek::traits::VartimeMultiscalarMul;
use digest::generic_array::typenum::U64;
use digest::Digest;
use rand::{CryptoRng, Rng, SeedableRng};
use rand_chacha::ChaChaRng;

use crate::errors::{InternalError, TokenError};
use crate::oprf::*;

/// The length of a `DLEQProof`, in bytes.
pub const DLEQ_PROOF_LENGTH: usize = 64;

/// A `DLEQProof` is a proof of the equivalence of the discrete logarithm between two pairs of points.
#[allow(non_snake_case)]
#[derive(Debug, Clone)]
pub struct DLEQProof {
    /// `c` is a `Scalar`
    /// \\(c=H_3(X,Y,P,Q,A,B)\\)
    pub(crate) c: Scalar,
    /// `s` is a `Scalar`
    /// \\(s = (t - ck) \mod q\\)
    pub(crate) s: Scalar,
}

#[cfg(any(test, feature = "base64"))]
impl_base64!(DLEQProof);

#[cfg(feature = "serde")]
impl_serde!(DLEQProof);

#[allow(non_snake_case)]
impl DLEQProof {
    /// Construct a new `DLEQProof`
    fn _new<D, T>(rng: &mut T, P: RistrettoPoint, Q: RistrettoPoint, k: &SigningKey) -> Self
    where
        D: Digest<OutputSize = U64> + Default,
        T: Rng + CryptoRng,
    {
        let t = Scalar::random(rng);

        let A = &t * &constants::RISTRETTO_BASEPOINT_TABLE;
        let B = t * P;

        let mut h = D::default();

        let X = constants::RISTRETTO_BASEPOINT_COMPRESSED;
        let Y = k.public_key.0;
        let P = P.compress();
        let Q = Q.compress();
        let A = A.compress();
        let B = B.compress();

        h.update(X.as_bytes());
        h.update(Y.as_bytes());
        h.update(P.as_bytes());
        h.update(Q.as_bytes());
        h.update(A.as_bytes());
        h.update(B.as_bytes());

        let c = Scalar::from_hash(h);

        let s = t - c * k.k;

        DLEQProof { c, s }
    }

    /// Construct a new `DLEQProof`
    pub fn new<D, T>(
        rng: &mut T,
        blinded_token: &BlindedToken,
        signed_token: &SignedToken,
        k: &SigningKey,
    ) -> Result<Self, TokenError>
    where
        D: Digest<OutputSize = U64> + Default,
        T: Rng + CryptoRng,
    {
        Ok(Self::_new::<D, T>(
            rng,
            blinded_token
                .0
                .decompress()
                .ok_or(TokenError(InternalError::PointDecompressionError))?,
            signed_token
                .0
                .decompress()
                .ok_or(TokenError(InternalError::PointDecompressionError))?,
            k,
        ))
    }

    /// Verify the `DLEQProof`
    fn _verify<D>(
        &self,
        P: RistrettoPoint,
        Q: RistrettoPoint,
        public_key: &PublicKey,
    ) -> Result<(), TokenError>
    where
        D: Digest<OutputSize = U64> + Default,
    {
        let X = constants::RISTRETTO_BASEPOINT_COMPRESSED;
        let Y = public_key.0;

        let A = (&self.s * &constants::RISTRETTO_BASEPOINT_TABLE)
            + (self.c
                * Y.decompress()
                    .ok_or(TokenError(InternalError::PointDecompressionError))?);
        let B = (self.s * P) + (self.c * Q);

        let A = A.compress();
        let B = B.compress();
        let P = P.compress();
        let Q = Q.compress();

        let mut h = D::default();

        h.update(X.as_bytes());
        h.update(Y.as_bytes());
        h.update(P.as_bytes());
        h.update(Q.as_bytes());
        h.update(A.as_bytes());
        h.update(B.as_bytes());

        let c = Scalar::from_hash(h);

        if c == self.c {
            Ok(())
        } else {
            Err(TokenError(InternalError::VerifyError))
        }
    }

    /// Verify the `DLEQProof`
    pub fn verify<D>(
        &self,
        blinded_token: &BlindedToken,
        signed_token: &SignedToken,
        public_key: &PublicKey,
    ) -> Result<(), TokenError>
    where
        D: Digest<OutputSize = U64> + Default,
    {
        self._verify::<D>(
            blinded_token
                .0
                .decompress()
                .ok_or(TokenError(InternalError::PointDecompressionError))?,
            signed_token
                .0
                .decompress()
                .ok_or(TokenError(InternalError::PointDecompressionError))?,
            public_key,
        )
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
#[derive(Debug, Clone)]
pub struct BatchDLEQProof(DLEQProof);

#[cfg(any(test, feature = "base64"))]
impl_base64!(BatchDLEQProof);

#[cfg(feature = "serde")]
impl_serde!(BatchDLEQProof);

#[allow(non_snake_case)]
impl BatchDLEQProof {
    fn calculate_composites<D>(
        blinded_tokens: &[BlindedToken],
        signed_tokens: &[SignedToken],
        public_key: &PublicKey,
    ) -> Result<(RistrettoPoint, RistrettoPoint), TokenError>
    where
        D: Digest<OutputSize = U64> + Default,
    {
        if blinded_tokens.len() != signed_tokens.len() {
            return Err(TokenError(InternalError::LengthMismatchError));
        }

        let mut h = D::default();

        h.update(constants::RISTRETTO_BASEPOINT_COMPRESSED.as_bytes());
        h.update(public_key.0.as_bytes());

        for (Pi, Qi) in blinded_tokens.iter().zip(signed_tokens.iter()) {
            h.update(Pi.0.as_bytes());
            h.update(Qi.0.as_bytes());
        }

        let result = h.finalize();

        let mut seed: [u8; 32] = [0u8; 32];
        seed.copy_from_slice(&result[..32]);

        let mut prng: ChaChaRng = SeedableRng::from_seed(seed);
        let c_m: Vec<Scalar> = iter::repeat_with(|| Scalar::random(&mut prng))
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
    pub fn new<D, T>(
        rng: &mut T,
        blinded_tokens: &[BlindedToken],
        signed_tokens: &[SignedToken],
        signing_key: &SigningKey,
    ) -> Result<Self, TokenError>
    where
        D: Digest<OutputSize = U64> + Default,
        T: Rng + CryptoRng,
    {
        let (M, Z) = BatchDLEQProof::calculate_composites::<D>(
            blinded_tokens,
            signed_tokens,
            &signing_key.public_key,
        )?;
        Ok(BatchDLEQProof(DLEQProof::_new::<D, T>(
            rng,
            M,
            Z,
            signing_key,
        )))
    }

    /// Verify a `BatchDLEQProof`
    pub fn verify<D>(
        &self,
        blinded_tokens: &[BlindedToken],
        signed_tokens: &[SignedToken],
        public_key: &PublicKey,
    ) -> Result<(), TokenError>
    where
        D: Digest<OutputSize = U64> + Default,
    {
        let (M, Z) =
            BatchDLEQProof::calculate_composites::<D>(blinded_tokens, signed_tokens, public_key)?;

        self.0._verify::<D>(M, Z, public_key)
    }

    /// Verify the `BatchDLEQProof` then unblind the `SignedToken`s using each corresponding `Token`
    pub fn verify_and_unblind<'a, D, I>(
        &self,
        tokens: I,
        blinded_tokens: &[BlindedToken],
        signed_tokens: &[SignedToken],
        public_key: &PublicKey,
    ) -> Result<Vec<UnblindedToken>, TokenError>
    where
        D: Digest<OutputSize = U64> + Default,
        I: IntoIterator<Item = &'a Token>,
    {
        self.verify::<D>(blinded_tokens, signed_tokens, public_key)?;

        let unblinded_tokens: Result<Vec<UnblindedToken>, TokenError> = tokens
            .into_iter()
            .zip(signed_tokens.iter())
            .map(|(token, signed_token)| token.unblind(signed_token))
            .collect();
        unblinded_tokens.and_then(|unblinded_tokens| {
            if unblinded_tokens.len() != signed_tokens.len() {
                return Err(TokenError(InternalError::LengthMismatchError));
            }
            Ok(unblinded_tokens)
        })
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

#[cfg(test)]
mod tests {
    use curve25519_dalek::ristretto::CompressedRistretto;
    use rand::rngs::OsRng;
    use sha2::Sha512;

    use super::*;
    use crate::oprf::Token;

    #[test]
    #[allow(non_snake_case)]
    fn works() {
        let mut rng = OsRng;

        let key1 = SigningKey::random(&mut rng);
        let key2 = SigningKey::random(&mut rng);

        let P = RistrettoPoint::random(&mut rng);
        let Q = key1.k * P;

        let proof = DLEQProof::_new::<Sha512, _>(&mut rng, P, Q, &key1);

        assert!(proof._verify::<Sha512>(P, Q, &key1.public_key).is_ok());

        let P = RistrettoPoint::random(&mut rng);
        let Q = key2.k * P;

        let proof = DLEQProof::_new::<Sha512, _>(&mut rng, P, Q, &key1);

        assert!(!proof._verify::<Sha512>(P, Q, &key1.public_key).is_ok());
    }

    #[allow(non_snake_case)]
    #[test]
    fn vector_tests() {
        // Generated using tools/dleq-test-gen
        let vectors = [
("SlPD+7xZlw7l+Fr4E4dd/8E6kEouU65+ZfoN6m5iyQE=", "nGajOcg0T5IvwyBstdroFKWUwBd90yNcJU2cQJpluAg=", "COY3iHwUTGQZ9q2bycTgMwgHcLZc+VqICT9kNoeNnTc=", "VLYFn6ZhrZpSHcRS2ffzbtYod1r6gWRc54hVFzXW+1s=", "4hl2AKMk2ueO7f67sHGol531jPu4DfDtDPrkFS10DgVj6RSbG13zWMt+KjpwVtAUdEMkJuYv8b0NPaBZrMlaAg=="),
("rBKvQjzCywrH+WAHjvVpB4P59cy1A0CCcYjeUioWdA0=", "fEgJRiE5cPsSkdQR+gM+EeGbAja5GYenhQPRuirUGHM=", "Hnvv9L/So4IvS2FPpzvRJL3pZ9ow01IHri3qefZ673Y=", "mszTCP66IXeEPOV7fvVR7bLj3NwxweLxH8wq3XQCTDc=", "u9DkAnPlj21cKR0CHhuGFbi419mRl0fnB5/s6VhDVABwLm1C7zLpiCwRzFHyEwkIBam/2Nt+/ZsuZL9B5cZpDQ=="),
("vu+HsyYOp5bteH7HKTl22hV/pgqGg1Xo3LIDW56ASQY=", "0EnIefouYrEhIxV06aSvEb0wlADk9pPt2IRJAPkBPAA=", "dpF+MlDg5pNOH0C53DhdWAnsqLz6haKZx/6M+wfzsz4=", "dgvVTGJZTUiGQgRZqcKq26a0ERoNGPNtMJ2Hromjox4=", "7NoWAv8wvYJoNZtPmRK1L+xpbR2NDOpbnh/8ccGiOABfxGf6f/BmQjVDZQiGbYXnyQjzM5NfcjiAP/shOK5lCg=="),
("tviSLm/W8oFds67y9lMs990fjh08hQNV17/4V2bmOQY=", "5ufRlCvVKvXp1yuxxS7Jvw9LSwQUl6Q/MlT6HY2l1Hc=", "zOVEbK4KQ1GBW97YUVNguoN+NntwtGi1t+EeioMusXY=", "lH2gNbwqSC1nYYxT3I7fNQagTsD4OvSbzwrSCpanQkQ=", "NJF9U3TWiCWMd6Qh/vA90F/2N6udsXbTvifNxf0rzgbhInoEvYDi5jZAZUQEi7x7mmP8iFq7+ukoOroy6/8jCw=="),
("Ge3prZ2jJSoh1A3ZvrSfaSA1kDziGW2I+Gmh6jniaAs=", "2nNCd5YN9H5EYlOL9/kmLYNBMkaLwnG3wjyd7jw2QAY=", "YHdAzlpSTAMy3mB+F4mPwlyVl+V9Yt4f3cDPNJpWdns=", "gEnqgXg3FDaCQFayTXrIfpbZ2n0P6FD/95LuMsdIfFk=", "Fj2/YunbQs5XxSyLxl/fC4dAfRlErGurTtHHSfGKyQTzrLZrO7VghmGFQaMAXZ+jg+6v99YL6FWj1Y/5WFt2Aw=="),
        ];
        for i in 0..vectors.len() {
            let (k, Y, P, Q_b64, dleq_b64) = vectors[i];

            let server_key = SigningKey::decode_base64(k).unwrap();

            assert_eq!(server_key.public_key.encode_base64(), Y);

            let P_bytes = base64::decode(P).unwrap();
            let mut P_bits: [u8; 32] = [0u8; 32];
            P_bits.copy_from_slice(&P_bytes[..32]);
            let P = CompressedRistretto(P_bits).decompress().unwrap();

            let Q = P * server_key.k;

            assert_eq!(base64::encode(&Q.compress().to_bytes()[..]), Q_b64);

            let seed: [u8; 32] = [0u8; 32];
            let mut prng: ChaChaRng = SeedableRng::from_seed(seed);

            let dleq = DLEQProof::_new::<Sha512, _>(&mut prng, P, Q, &server_key);
            assert_eq!(dleq.encode_base64(), dleq_b64);

            assert!(dleq._verify::<Sha512>(P, Q, &server_key.public_key).is_ok());
        }
    }

    #[allow(non_snake_case)]
    #[test]
    fn batch_vector_tests() {
        // Generated using tools/dleq-test-gen
        let vectors = [
("8n06lJH/lkGHsPVPehyAwYA4BX97JbFmDmJzPO1oIQE=", "4KbMr7/9/ZxNG2Baixy/3MFFOCzFFfJLjsUAoetKOjo=", "xGUA5fGnGgvg1zzC3mWeq7K2jqVBfJwJ2uJ6upqPxgA=,LHTaaZi7YqIsuqJ1P+HV8vYIwHFKePF+xton737J3SE=,8MFJE9IYULJT+LsiI3h+kDLc4rmd809utaDo1xic3GQ=,HOMibeqOkyJsN+N5eRJrQkAT+G32L6eOhU+1/Drsbks=,PGLb/Ael7MnFYaS6YjCR/lRIkKONoAsEKuJ5SRvJMwg=", "fgOAdXLxUs/pfT3ustR9mBbUyIbcx1Nis0zWuIz8TS8=,Lh0ekCvZ0cWhDTTxUQV3frSb2+hW9mDWunYWGv/foTA=,EvA36hZe6DD9WOU/F7rnJ8kgXyUgDZ908Li4F1IqAko=,gtbw9qu9qpp1nYEejzzhsWkMyOM8kbvOOMBTdArj82E=,ypADHr5ETijWYAQTw9f6J7OSIk24S1JwBVrgrIsAAE8=", "jhS4wBez3EdGoGEW4ENMW0UpZHn1aiJa9WUWp5RJHyk=", "xJfJMAnGI0ZrEuJcuBaa7cZ84HCe62f9eWIQE2JeiUU=", "X9bPqHlbzd+PzBCTMoZMdF8joNsbZkODlPABwhgD7w3XLP6J8BQP80bfgagB3pXWwa0Uhd1NquBhOL5uCNy9DA=="),
("iE4eMIMtnV1rgsSMpuE9rfT486QnmuyNAsDB8JSwkgI=", "MIo6R2WZkAylW7XeUOwcYClUOpXj9wZ14Tr6wYY7FDs=", "flw+gZoFAcOlEPmGqr3LliVkshxK5XfBNlow5X+clSs=,roiJVY4XvfsBh6iGSzz9OM/40Nx4VrG9SriyifVOc0A=,UMnZMVFyE9keC0PSx2KSZ2Y8DEM8tT6/XGQAV6AXGBg=,dPcHlc7Q3X/9F9uKd4T2bCk9uOANGBoh72N4tyn/6Xg=,pHS/yXXFw+wYP8FOAqKFCyzuI8552dyKoaEVcA9PXnw=", "sL8mkkfISTi+0cvfZLaB+bb0zbp9vSo3WEt8Q/t5VR0=,quVLKurPfGk/waJnPXniatNV0byGYL9R04PI0Bap5H0=,/naRAvDSk/6QwHC5dFuGKqCwB6Jy+r30EqL2+5y06XI=,Ms7MKju+/YdXOaMjYRouki5VhPA3O6Ng8Hw4DDRdAxU=,6A2sYddm/4xRSJkTpUDrPUuK1Cy0OCUpUthlTgjEQWM=", "an0BjUmpwb/SON3OS/Unoj4+AJ0I7+27WS83LFKtKk4=", "rIv5UFDK/jGS2jy/tHV4vKMXdr8bDMjJt6JFOksb7gQ=", "LR8s0d+woWFwCJpdDek4cWt6d0lbiGoEZMmxa+rILggX0zKz5/T2QQr4qQJAXSWnkySjIwVGtydn+sha8nJJBw=="),
("CDATdJXtmEMd+a8MiNyBzJfml/tlFeUt+7fqfSdvGgI=", "oCjGp1LXIuALA14oUuDzMvXxZQqNmA9zQvpW7gj8gQc=", "uqzF6Yud/TcOb0Qqh6W5pVXDVTfccpd6vDFKl6cko2s=,1JHJSOeBP4YZJU8vxBQJky++2CCnSXR7AiVYhWIcxhc=,xlt2BWmGn2HHNQRMouv/4CZTjbA5OzAW/OUHy/xMRwc=,jneVhZ2uILeea08oFNHrsMrySZ5NpGZ2rIT8pM2IPxM=,KFtx8ceLvtuL2+dRPm7bDZ5FdrL0f/TRNNkIzMtttl4=", "mF7kNktgoP9OHSA8mgm6t+Vmx9bhO79TG0AjYNjbz24=,7IbBiJX/cH4//616F8UCgo2Lzh3RATwRlrtLl2GmjlM=,YBl7hRaHTwQ19HwtbrVcRTBQ3xhpDrb6EjzS80LO2wQ=,Rs6W8kUZCBam3BZKyvC/Zc7S3bVIWEBXzNVwKsSk/Us=,ylT/xqEBYjWDWfZd0lH9w08/c55SsA9KSgAOtNz48HE=", "AjQpqYvy6onAHHyD8ZtsVGYW/Y84bwvusSy3MEYyQng=", "wESOv7FzkbDc3NJraYWj58aeFRXbt0VAEb1PgLsJ73g=", "hDDZcM6e+X9UV8ZtV9QwQ7w4PeHb+A1ug7WfsKiAXQEHk7P4LIrXSH2AvI09NGUG642xKweGXmkvELD3Q7yxAQ=="),
("siv+BM3AvP8Jv1aL4MFhMs9Xa6jxUNhFXpTWDfGrZQQ=", "XgFOlHEz5zm5dtx6ptYIXNg1NsJ/3vAq+cf/9eBkbxI=", "dsaMl4/9FcOFtaW3l65y1Z9ETJR36aTcXPMp+w4HGUY=,aH2q1HiReMA/Ney2NNZCgl+5GKK9xrxVwdC+THq9pFY=,uGRqS51VD7DuK0gSpMb3owRld57W6DqOyZpygXJVpmI=,Bvv+lqtCg39SD1H218rPZdQTmYPe2HD3QScntqw1oFA=,9IHWUyv/SCwZ4WKEGi58+bQ5nHsaDBXCku2vOzGvgUY=", "4lPV/OyNjVy4VTUvaROxCuq4ryfegkt7jt5IhrX9THo=,4EAmV5Mv3a/IQFsfVlaFxErNc96Ns980FT4yLlCdoxA=,bgGM37uBMLdRRAd1cu/4Iq+FzFwzRFLVhqp2uGFnPQc=,rCn4OuWnV5tTsgcPJAYRSqfONZf9k/92fwzWHtUxxh8=,lHCDzazDlU0w735u7OQmJM96WGeaNFILanawmC9EwFE=", "jotItTWLW/kpDeh8KJQtNqM7ON0YibEJ7R8VnMHP7Cs=", "Pu49xb3Ixn+Dfg6s+wgjyoPy7ickB5lM7/MxQVdpaUI=", "N6vDmGbYZ0aa9S2JqWSYppiX1AV33QDXGc8FHaF0aQ9y6Hp68UEkI2x5AJQ3URqS+5/x1AuucMH0AOMcLNqODg=="),
("jTTf/D0gicaG++cQJ1X4qYaOqk4YPo0p6Mo2B95kJAg=", "LCtewONuYTXljy+oK73/m7CON/vr/e1r4aDaVE3xDnc=", "GsomH3aBo6qBHaNGzZZ/pNOviBTbZrUfpthgYU5jAmU=,wHvznjZDA9L8dgQGEj7wf1/QGxunE5/WYdxUpQX4Umc=,ZMxkDV7epUgmpix38jBfWv42VeMQefypY56dnysikWA=,aEy+/J0AFmjYGTjcv5y942fleEk/0rwqlD+kXSn0vCM=,QJMHfIYUraWdJKzenROtgLyjU9MrDtDDwFfDNIcjCHA=", "aGUXY5bBYoDmw4x1muwHzKp1w2sITQMeDfsyxv9EUDc=,qh0wfC/wAdclIJ79R+IIpPJLJM5aBe/i5i54dQOn3Vk=,zCi7XaHyO/b9SfN2AYuJcC60zqnIorkXMjjbixhEKxk=,XlBf7DyH5FtcUyr9Gfnj8i3cnKKPtWGHXm/LpIQX4gM=,wHtZg2i40wdxQvHehZEunTHiODSuEMv8suwFaqynmTQ=", "osUDqpps33Jw0k7vEHFCAk+iywlE7YrXrX5RfqC0olA=", "LnD9wlZrDo3v6dw56owm6NazoBLKwtqMPWdtxSRVWxc=", "KVEotBgIaz5Rymqpy4paroHGQyD/80FdvLCrONxDzQUWgNZxZ6aiCJ2VxIGP+6+86FZXS1sXGgs3dwft/VMCCw=="),
        ];
        for i in 0..vectors.len() {
            let (k, Y, P, Q, M_b64, Z_b64, dleq_b64) = vectors[i];

            let server_key = SigningKey::decode_base64(k).unwrap();

            assert_eq!(server_key.public_key.encode_base64(), Y);

            let P: Vec<&str> = P.split(',').collect();
            let Q: Vec<&str> = Q.split(',').collect();

            let P: Vec<BlindedToken> = P
                .iter()
                .map(|P_i| BlindedToken::decode_base64(P_i).unwrap())
                .collect();

            let Q: Vec<SignedToken> = P
                .iter()
                .zip(Q.into_iter())
                .map(|(P_i, Q_i_b64)| {
                    let Q_i = server_key.sign(P_i).unwrap();
                    assert_eq!(Q_i.encode_base64(), Q_i_b64);
                    Q_i
                })
                .collect();

            let (M, Z) =
                BatchDLEQProof::calculate_composites::<Sha512>(&P, &Q, &server_key.public_key)
                    .unwrap();

            assert_eq!(base64::encode(&M.compress().to_bytes()[..]), M_b64);
            assert_eq!(base64::encode(&Z.compress().to_bytes()[..]), Z_b64);

            let seed: [u8; 32] = [0u8; 32];
            let mut prng: ChaChaRng = SeedableRng::from_seed(seed);

            let batch_proof =
                BatchDLEQProof::new::<Sha512, _>(&mut prng, &P, &Q, &server_key).unwrap();
            assert_eq!(batch_proof.encode_base64(), dleq_b64);

            assert!(batch_proof
                .verify::<Sha512>(&P, &Q, &server_key.public_key)
                .is_ok());
        }
    }

    #[test]
    #[allow(non_snake_case)]
    fn batch_works() {
        use std::vec::Vec;

        let mut rng = OsRng;

        let key = SigningKey::random(&mut rng);

        let blinded_tokens = vec![Token::random::<Sha512, _>(&mut rng).blind()];
        let signed_tokens: Vec<SignedToken> = blinded_tokens
            .iter()
            .filter_map(|t| key.sign(t).ok())
            .collect();

        let batch_proof =
            BatchDLEQProof::new::<Sha512, _>(&mut rng, &blinded_tokens, &signed_tokens, &key)
                .unwrap();

        assert!(batch_proof
            .verify::<Sha512>(&blinded_tokens, &signed_tokens, &key.public_key)
            .is_ok());
    }
}
