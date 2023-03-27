use std::io;
use std::sync::atomic::{AtomicBool, Ordering};

#[cfg(feature = "multicore")]
use rayon::prelude::*;

#[cfg(feature = "pairing")]
use bls12_381::{
    hash_to_curve::{ExpandMsgXmd, HashToCurve},
    Bls12, G1Affine, G2Affine, G2Projective, Gt, MillerLoopResult,
};
use pairing_lib::MultiMillerLoop;

#[cfg(feature = "blst")]
use blstrs::{Bls12, G1Affine, G2Affine, G2Projective, Gt, MillerLoopResult};
#[cfg(feature = "blst")]
use group::{prime::PrimeCurveAffine, Group};
#[cfg(feature = "blst")]
use pairing_lib::MillerLoopResult as _;

use crate::error::Error;
use crate::key::*;

const CSUITE: &[u8] = b"BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";
const G2_COMPRESSED_SIZE: usize = 96;

#[derive(Debug, Copy, Clone, PartialEq)]
pub struct Signature(G2Affine);

impl From<G2Projective> for Signature {
    fn from(val: G2Projective) -> Self {
        Signature(val.into())
    }
}
impl From<Signature> for G2Projective {
    fn from(val: Signature) -> Self {
        val.0.into()
    }
}

impl From<G2Affine> for Signature {
    fn from(val: G2Affine) -> Self {
        Signature(val)
    }
}

impl From<Signature> for G2Affine {
    fn from(val: Signature) -> Self {
        val.0
    }
}

impl Serialize for Signature {
    fn write_bytes(&self, dest: &mut impl io::Write) -> io::Result<()> {
        dest.write_all(&self.0.to_compressed())?;

        Ok(())
    }

    fn from_bytes(raw: &[u8]) -> Result<Self, Error> {
        let g2 = g2_from_slice(raw)?;
        Ok(g2.into())
    }
}

fn g2_from_slice(raw: &[u8]) -> Result<G2Affine, Error> {
    if raw.len() != G2_COMPRESSED_SIZE {
        return Err(Error::SizeMismatch);
    }

    let mut res = [0u8; G2_COMPRESSED_SIZE];
    res.copy_from_slice(raw);

    Option::from(G2Affine::from_compressed(&res)).ok_or(Error::GroupDecode)
}

/// Hash the given message, as used in the signature.
#[cfg(feature = "pairing")]
pub fn hash(msg: &[u8]) -> G2Projective {
    <G2Projective as HashToCurve<ExpandMsgXmd<sha2::Sha256>>>::hash_to_curve(msg, CSUITE)
}

#[cfg(feature = "blst")]
pub fn hash(msg: &[u8]) -> G2Projective {
    G2Projective::hash_to_curve(msg, CSUITE, &[])
}

/// Aggregate signatures by multiplying them together.
/// Calculated by `signature = \sum_{i = 0}^n signature_i`.
#[cfg(feature = "multicore")]
pub fn aggregate(signatures: &[Signature]) -> Result<Signature, Error> {
    if signatures.is_empty() {
        return Err(Error::ZeroSizedInput);
    }

    let res = signatures
        .into_par_iter()
        .fold(G2Projective::identity, |mut acc, signature| {
            acc += &signature.0;
            acc
        })
        .reduce(G2Projective::identity, |acc, val| acc + val);

    Ok(Signature(res.into()))
}

/// Aggregate signatures by multiplying them together.
/// Calculated by `signature = \sum_{i = 0}^n signature_i`.
#[cfg(not(feature = "multicore"))]
pub fn aggregate(signatures: &[Signature]) -> Result<Signature, Error> {
    if signatures.is_empty() {
        return Err(Error::ZeroSizedInput);
    }

    let res = signatures
        .into_iter()
        .fold(G2Projective::identity(), |acc, signature| {
            acc + &signature.0
        });

    Ok(Signature(res.into()))
}

/// Verifies that the signature is the actual aggregated signature of hashes - pubkeys.
/// Calculated by `e(g1, signature) == \prod_{i = 0}^n e(pk_i, hash_i)`.
pub fn verify(signature: &Signature, hashes: &[G2Projective], public_keys: &[PublicKey]) -> bool {
    if hashes.is_empty() || public_keys.is_empty() {
        return false;
    }

    let n_hashes = hashes.len();

    if n_hashes != public_keys.len() {
        return false;
    }

    // zero key & single hash should fail
    if n_hashes == 1 && public_keys[0].0.is_identity().into() {
        return false;
    }

    // Enforce that messages are distinct as a countermeasure against BLS's rogue-key attack.
    // See Section 3.1. of the IRTF's BLS signatures spec:
    // https://tools.ietf.org/html/draft-irtf-cfrg-bls-signature-02#section-3.1
    for i in 0..(n_hashes - 1) {
        for j in (i + 1)..n_hashes {
            if hashes[i] == hashes[j] {
                return false;
            }
        }
    }

    let is_valid = AtomicBool::new(true);

    #[cfg(feature = "multicore")]
    let mut ml = public_keys
        .par_iter()
        .zip(hashes.par_iter())
        .map(|(pk, h)| {
            if pk.0.is_identity().into() {
                is_valid.store(false, Ordering::Relaxed);
            }
            let pk = pk.as_affine();
            let h = G2Affine::from(h).into();
            Bls12::multi_miller_loop(&[(&pk, &h)])
        })
        .reduce(MillerLoopResult::default, |acc, cur| acc + cur);

    #[cfg(not(feature = "multicore"))]
    let mut ml = public_keys
        .iter()
        .zip(hashes.iter())
        .map(|(pk, h)| {
            if pk.0.is_identity().into() {
                is_valid.store(false, Ordering::Relaxed);
            }
            let pk = pk.as_affine();
            let h = G2Affine::from(h).into();
            Bls12::multi_miller_loop(&[(&pk, &h)])
        })
        .fold(MillerLoopResult::default(), |acc, cur| acc + cur);

    if !is_valid.load(Ordering::Relaxed) {
        return false;
    }

    let g1_neg = -G1Affine::generator();

    ml += Bls12::multi_miller_loop(&[(&g1_neg, &signature.0.into())]);

    ml.final_exponentiation() == Gt::identity()
}

/// Verifies that the signature is the actual aggregated signature of messages - pubkeys.
/// Calculated by `e(g1, signature) == \prod_{i = 0}^n e(pk_i, hash_i)`.
#[cfg(feature = "pairing")]
pub fn verify_messages(
    signature: &Signature,
    messages: &[&[u8]],
    public_keys: &[PublicKey],
) -> bool {
    #[cfg(feature = "multicore")]
    let hashes: Vec<_> = messages.par_iter().map(|msg| hash(msg)).collect();

    #[cfg(not(feature = "multicore"))]
    let hashes: Vec<_> = messages.iter().map(|msg| hash(msg)).collect();

    verify(signature, &hashes, public_keys)
}

/// Verifies that the signature is the actual aggregated signature of messages - pubkeys.
/// Calculated by `e(g1, signature) == \prod_{i = 0}^n e(pk_i, hash_i)`.
#[cfg(all(feature = "blst", feature = "multicore"))]
pub fn verify_messages(
    signature: &Signature,
    messages: &[&[u8]],
    public_keys: &[PublicKey],
) -> bool {
    if messages.is_empty() || public_keys.is_empty() {
        return false;
    }

    let n_messages = messages.len();

    if n_messages != public_keys.len() {
        return false;
    }

    // zero key & single message should fail
    if n_messages == 1 && public_keys[0].0.is_identity().into() {
        return false;
    }

    // Enforce that messages are distinct as a countermeasure against BLS's rogue-key attack.
    // See Section 3.1. of the IRTF's BLS signatures spec:
    // https://tools.ietf.org/html/draft-irtf-cfrg-bls-signature-02#section-3.1
    if !blstrs::unique_messages(messages) {
        return false;
    }

    let valid = AtomicBool::new(true);

    let n_workers = std::cmp::min(rayon::current_num_threads(), n_messages);
    let mut pairings = messages
        .par_iter()
        .zip(public_keys.par_iter())
        .chunks(n_messages / n_workers)
        .map(|chunk| {
            let mut pairing = blstrs::PairingG1G2::new(true, CSUITE);

            for (message, public_key) in chunk {
                let res = pairing.aggregate(&public_key.0.into(), None, message, &[]);
                if res.is_err() {
                    valid.store(false, Ordering::Relaxed);
                    break;
                }
            }
            if valid.load(Ordering::Relaxed) {
                pairing.commit();
            }

            pairing
        })
        .collect::<Vec<_>>();

    let mut gtsig = Gt::default();
    if valid.load(Ordering::Relaxed) {
        blstrs::PairingG1G2::aggregated(&mut gtsig, &signature.0);
    }

    let mut acc = pairings.pop().unwrap();
    for pairing in &pairings {
        let res = acc.merge(pairing);
        if res.is_err() {
            return false;
        }
    }

    valid.load(Ordering::Relaxed) && acc.finalverify(Some(&gtsig))
}

/// Verifies that the signature is the actual aggregated signature of messages - pubkeys.
/// Calculated by `e(g1, signature) == \prod_{i = 0}^n e(pk_i, hash_i)`.
#[cfg(all(feature = "blst", not(feature = "multicore")))]
pub fn verify_messages(
    signature: &Signature,
    messages: &[&[u8]],
    public_keys: &[PublicKey],
) -> bool {
    if messages.is_empty() || public_keys.is_empty() {
        return false;
    }

    let n_messages = messages.len();

    if n_messages != public_keys.len() {
        return false;
    }

    // zero key & single message should fail
    if n_messages == 1 && public_keys[0].0.is_identity().into() {
        return false;
    }

    // Enforce that messages are distinct as a countermeasure against BLS's rogue-key attack.
    // See Section 3.1. of the IRTF's BLS signatures spec:
    // https://tools.ietf.org/html/draft-irtf-cfrg-bls-signature-02#section-3.1
    if !blstrs::unique_messages(messages) {
        return false;
    }

    let mut valid = true;
    let mut pairing = blstrs::PairingG1G2::new(true, CSUITE);
    for (message, public_key) in messages.iter().zip(public_keys.iter()) {
        let res = pairing.aggregate(&public_key.0.into(), None, message, &[]);
        if res.is_err() {
            valid = false;
            break;
        }

        pairing.commit();
    }

    let mut gtsig = Gt::default();
    if valid {
        blstrs::PairingG1G2::aggregated(&mut gtsig, &signature.0);
    }

    valid && pairing.finalverify(Some(&gtsig))
}

#[cfg(test)]
mod tests {
    use super::*;

    use base64::STANDARD;
    use rand::{Rng, SeedableRng};
    use rand_chacha::ChaCha8Rng;
    use serde::Deserialize;

    #[cfg(feature = "pairing")]
    use crate::key::G1_COMPRESSED_SIZE;
    #[cfg(feature = "pairing")]
    use bls12_381::{G1Projective, Scalar};
    #[cfg(feature = "blst")]
    use blstrs::{G1Projective, Scalar};
    #[cfg(feature = "blst")]
    use ff::Field;

    #[test]
    fn basic_aggregation() {
        let mut rng = ChaCha8Rng::seed_from_u64(12);

        let num_messages = 10;

        // generate private keys
        let private_keys: Vec<_> = (0..num_messages)
            .map(|_| PrivateKey::generate(&mut rng))
            .collect();

        // generate messages
        let messages: Vec<Vec<u8>> = (0..num_messages)
            .map(|_| (0..64).map(|_| rng.gen()).collect())
            .collect();

        // sign messages
        let sigs = messages
            .iter()
            .zip(&private_keys)
            .map(|(message, pk)| pk.sign(message))
            .collect::<Vec<Signature>>();

        let aggregated_signature = aggregate(&sigs).expect("failed to aggregate");

        let hashes = messages
            .iter()
            .map(|message| hash(message))
            .collect::<Vec<_>>();
        let public_keys = private_keys
            .iter()
            .map(|pk| pk.public_key())
            .collect::<Vec<_>>();

        assert!(
            verify(&aggregated_signature, &hashes, &public_keys),
            "failed to verify"
        );

        let messages = messages.iter().map(|r| &r[..]).collect::<Vec<_>>();
        assert!(verify_messages(
            &aggregated_signature,
            &messages[..],
            &public_keys
        ));
    }

    #[test]
    fn aggregation_same_messages() {
        let mut rng = ChaCha8Rng::seed_from_u64(12);

        let num_messages = 10;

        // generate private keys
        let private_keys: Vec<_> = (0..num_messages)
            .map(|_| PrivateKey::generate(&mut rng))
            .collect();

        // generate messages
        let message: Vec<u8> = (0..64).map(|_| rng.gen()).collect();

        // sign messages
        let sigs = private_keys
            .iter()
            .map(|pk| pk.sign(&message))
            .collect::<Vec<Signature>>();

        let aggregated_signature = aggregate(&sigs).expect("failed to aggregate");

        // check that equal messages can not be aggreagated
        let hashes: Vec<_> = (0..num_messages).map(|_| hash(&message)).collect();
        let public_keys = private_keys
            .iter()
            .map(|pk| pk.public_key())
            .collect::<Vec<_>>();
        assert!(
            !verify(&aggregated_signature, &hashes, &public_keys),
            "must not verify aggregate with the same messages"
        );
        let messages = vec![&message[..]; num_messages];

        assert!(!verify_messages(
            &aggregated_signature,
            &messages[..],
            &public_keys
        ));
    }

    #[test]
    fn test_zero_key() {
        let mut rng = ChaCha8Rng::seed_from_u64(12);

        // In the current iteration we expect the zero key to be valid and work.
        let zero_key: PrivateKey = Scalar::zero().into();
        assert!(bool::from(zero_key.public_key().0.is_identity()));

        println!(
            "{:?}\n{:?}",
            zero_key.public_key().as_bytes(),
            zero_key.as_bytes()
        );
        let num_messages = 10;

        // generate private keys
        let mut private_keys: Vec<_> = (0..num_messages - 1)
            .map(|_| PrivateKey::generate(&mut rng))
            .collect();

        private_keys.push(zero_key);

        // generate messages
        let messages: Vec<Vec<u8>> = (0..num_messages)
            .map(|_| (0..64).map(|_| rng.gen()).collect())
            .collect();

        // sign messages
        let sigs = messages
            .iter()
            .zip(&private_keys)
            .map(|(message, pk)| pk.sign(message))
            .collect::<Vec<Signature>>();

        let aggregated_signature = aggregate(&sigs).expect("failed to aggregate");

        let hashes = messages
            .iter()
            .map(|message| hash(message))
            .collect::<Vec<_>>();
        let public_keys = private_keys
            .iter()
            .map(|pk| pk.public_key())
            .collect::<Vec<_>>();

        assert!(
            !verify(&aggregated_signature, &hashes, &public_keys),
            "verified with zero key"
        );

        let messages = messages.iter().map(|r| &r[..]).collect::<Vec<_>>();
        assert!(!verify_messages(
            &aggregated_signature,
            &messages[..],
            &public_keys
        ));

        // single message is rejected
        let signature = zero_key.sign(&messages[0]);

        assert!(!zero_key.public_key().verify(signature, &messages[0]));

        let aggregated_signature = aggregate(&[signature][..]).expect("failed to aggregate");
        assert!(!verify_messages(
            &aggregated_signature,
            &messages[..1],
            &[zero_key.public_key()][..],
        ));
    }

    #[test]
    fn test_bytes_roundtrip() {
        let mut rng = ChaCha8Rng::seed_from_u64(12);
        let sk = PrivateKey::generate(&mut rng);

        let msg = (0..64).map(|_| rng.gen()).collect::<Vec<u8>>();
        let signature = sk.sign(&msg);

        let signature_bytes = signature.as_bytes();
        assert_eq!(signature_bytes.len(), 96);
        assert_eq!(Signature::from_bytes(&signature_bytes).unwrap(), signature);
    }

    base64_serde_type!(Base64Standard, STANDARD);

    #[derive(Debug, Clone, Deserialize)]
    struct Case {
        #[serde(rename = "Msg")]
        msg: String,
        #[serde(rename = "Ciphersuite")]
        ciphersuite: String,
        #[serde(rename = "G1Compressed", with = "Base64Standard")]
        g1_compressed: Vec<u8>,
        #[serde(rename = "G2Compressed", with = "Base64Standard")]
        g2_compressed: Vec<u8>,
        #[serde(rename = "BLSPrivKey")]
        priv_key: Option<String>,
        #[serde(rename = "BLSPubKey")]
        pub_key: Option<String>,
        #[serde(rename = "BLSSigG2")]
        signature: Option<String>,
    }

    #[derive(Debug, Clone, Deserialize)]
    struct Cases {
        cases: Vec<Case>,
    }

    fn g1_from_slice(raw: &[u8]) -> Result<G1Affine, Error> {
        if raw.len() != G1_COMPRESSED_SIZE {
            return Err(Error::SizeMismatch);
        }

        let mut res = [0u8; G1_COMPRESSED_SIZE];
        res.as_mut().copy_from_slice(raw);

        Option::from(G1Affine::from_compressed(&res)).ok_or(Error::GroupDecode)
    }

    #[cfg(feature = "pairing")]
    fn hash_to_g1(msg: &[u8], suite: &[u8]) -> G1Projective {
        <G1Projective as HashToCurve<ExpandMsgXmd<sha2::Sha256>>>::hash_to_curve(msg, suite)
    }
    #[cfg(feature = "blst")]
    fn hash_to_g1(msg: &[u8], suite: &[u8]) -> G1Projective {
        G1Projective::hash_to_curve(msg, suite, &[])
    }

    #[cfg(feature = "pairing")]
    fn hash_to_g2(msg: &[u8], suite: &[u8]) -> G2Projective {
        <G2Projective as HashToCurve<ExpandMsgXmd<sha2::Sha256>>>::hash_to_curve(msg, suite)
    }
    #[cfg(feature = "blst")]
    fn hash_to_g2(msg: &[u8], suite: &[u8]) -> G2Projective {
        G2Projective::hash_to_curve(msg, suite, &[])
    }

    #[test]
    fn test_vectors() {
        let cases: Cases =
            serde_json::from_slice(&std::fs::read("./tests/data.json").unwrap()).unwrap();

        for case in cases.cases {
            let g1: G1Projective = g1_from_slice(&case.g1_compressed).unwrap().into();

            assert_eq!(
                g1,
                hash_to_g1(case.msg.as_bytes(), case.ciphersuite.as_bytes())
            );

            let g2: G2Projective = g2_from_slice(&case.g2_compressed).unwrap().into();
            assert_eq!(
                g2,
                hash_to_g2(case.msg.as_bytes(), case.ciphersuite.as_bytes())
            );

            if case.ciphersuite.as_bytes() == CSUITE {
                let pub_key =
                    PublicKey::from_bytes(&base64::decode(case.pub_key.as_ref().unwrap()).unwrap())
                        .unwrap();
                let priv_key = PrivateKey::from_string(case.priv_key.as_ref().unwrap()).unwrap();
                let signature = Signature::from_bytes(
                    &base64::decode(case.signature.as_ref().unwrap()).unwrap(),
                )
                .unwrap();

                let sig2 = priv_key.sign(&case.msg);
                assert_eq!(signature, sig2, "signatures do not match");

                assert!(pub_key.verify(signature, &case.msg), "failed to verify");
            }
        }
    }
}
