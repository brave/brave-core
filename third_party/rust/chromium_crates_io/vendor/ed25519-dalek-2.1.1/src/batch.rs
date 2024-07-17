// -*- mode: rust; -*-
//
// This file is part of ed25519-dalek.
// Copyright (c) 2017-2019 isis lovecruft
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>

//! Batch signature verification.

use alloc::vec::Vec;

use core::convert::TryFrom;
use core::iter::once;

use curve25519_dalek::constants;
use curve25519_dalek::edwards::EdwardsPoint;
use curve25519_dalek::scalar::Scalar;
use curve25519_dalek::traits::IsIdentity;
use curve25519_dalek::traits::VartimeMultiscalarMul;

pub use curve25519_dalek::digest::Digest;

use merlin::Transcript;

use rand_core::RngCore;

use sha2::Sha512;

use crate::errors::InternalError;
use crate::errors::SignatureError;
use crate::signature::InternalSignature;
use crate::VerifyingKey;

/// An implementation of `rand_core::RngCore` which does nothing. This is necessary because merlin
/// demands an `Rng` as input to `TranscriptRngBuilder::finalize()`. Using this with `finalize()`
/// yields a PRG whose input is the hashed transcript.
struct ZeroRng;

impl rand_core::RngCore for ZeroRng {
    fn next_u32(&mut self) -> u32 {
        rand_core::impls::next_u32_via_fill(self)
    }

    fn next_u64(&mut self) -> u64 {
        rand_core::impls::next_u64_via_fill(self)
    }

    /// A no-op function which leaves the destination bytes for randomness unchanged.
    ///
    /// In this case, the internal merlin code is initialising the destination
    /// by doing `[0u8; …]`, which means that when we call
    /// `merlin::TranscriptRngBuilder.finalize()`, rather than rekeying the
    /// STROBE state based on external randomness, we're doing an
    /// `ENC_{state}(00000000000000000000000000000000)` operation, which is
    /// identical to the STROBE `MAC` operation.
    fn fill_bytes(&mut self, _dest: &mut [u8]) {}

    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), rand_core::Error> {
        self.fill_bytes(dest);
        Ok(())
    }
}

// `TranscriptRngBuilder::finalize()` requires a `CryptoRng`
impl rand_core::CryptoRng for ZeroRng {}

// We write our own gen() function so we don't need to pull in the rand crate
fn gen_u128<R: RngCore>(rng: &mut R) -> u128 {
    let mut buf = [0u8; 16];
    rng.fill_bytes(&mut buf);
    u128::from_le_bytes(buf)
}

/// Verify a batch of `signatures` on `messages` with their respective `verifying_keys`.
///
/// # Inputs
///
/// * `messages` is a slice of byte slices, one per signed message.
/// * `signatures` is a slice of `Signature`s.
/// * `verifying_keys` is a slice of `VerifyingKey`s.
///
/// # Returns
///
/// * A `Result` whose `Ok` value is an empty tuple and whose `Err` value is a
///   `SignatureError` containing a description of the internal error which
///   occurred.
///
/// ## On Deterministic Nonces
///
/// The nonces for batch signature verification are derived purely from the inputs to this function
/// themselves.
///
/// In any sigma protocol it is wise to include as much context pertaining
/// to the public state in the protocol as possible, to avoid malleability
/// attacks where an adversary alters publics in an algebraic manner that
/// manages to satisfy the equations for the protocol in question.
///
/// For ed25519 batch verification we include the following as scalars in the protocol transcript:
///
/// * All of the computed `H(R||A||M)`s to the protocol transcript, and
/// * All of the `s` components of each signature.
///
/// The former, while not quite as elegant as adding the `R`s, `A`s, and
/// `M`s separately, saves us a bit of context hashing since the
/// `H(R||A||M)`s need to be computed for the verification equation anyway.
///
/// The latter prevents a malleability attack wherein an adversary, without access
/// to the signing key(s), can take any valid signature, `(s,R)`, and swap
/// `s` with `s' = -z1`.  This doesn't constitute a signature forgery, merely
/// a vulnerability, as the resulting signature will not pass single
/// signature verification.  (Thanks to Github users @real_or_random and
/// @jonasnick for pointing out this malleability issue.)
///
/// # Examples
///
/// ```
/// use ed25519_dalek::{
///     verify_batch, SigningKey, VerifyingKey, Signer, Signature,
/// };
/// use rand::rngs::OsRng;
///
/// # fn main() {
/// let mut csprng = OsRng;
/// let signing_keys: Vec<_> = (0..64).map(|_| SigningKey::generate(&mut csprng)).collect();
/// let msg: &[u8] = b"They're good dogs Brant";
/// let messages: Vec<_> = (0..64).map(|_| msg).collect();
/// let signatures:  Vec<_> = signing_keys.iter().map(|key| key.sign(&msg)).collect();
/// let verifying_keys: Vec<_> = signing_keys.iter().map(|key| key.verifying_key()).collect();
///
/// let result = verify_batch(&messages, &signatures, &verifying_keys);
/// assert!(result.is_ok());
/// # }
/// ```
#[allow(non_snake_case)]
pub fn verify_batch(
    messages: &[&[u8]],
    signatures: &[ed25519::Signature],
    verifying_keys: &[VerifyingKey],
) -> Result<(), SignatureError> {
    // Return an Error if any of the vectors were not the same size as the others.
    if signatures.len() != messages.len()
        || signatures.len() != verifying_keys.len()
        || verifying_keys.len() != messages.len()
    {
        return Err(InternalError::ArrayLength {
            name_a: "signatures",
            length_a: signatures.len(),
            name_b: "messages",
            length_b: messages.len(),
            name_c: "verifying_keys",
            length_c: verifying_keys.len(),
        }
        .into());
    }

    // Make a transcript which logs all inputs to this function
    let mut transcript: Transcript = Transcript::new(b"ed25519 batch verification");

    // We make one optimization in the transcript: since we will end up computing H(R || A || M)
    // for each (R, A, M) triplet, we will feed _that_ into our transcript rather than each R, A, M
    // individually. Since R and A are fixed-length, this modification is secure so long as SHA-512
    // is collision-resistant.
    // It suffices to take `verifying_keys[i].as_bytes()` even though a `VerifyingKey` has two
    // fields, and `as_bytes()` only returns the bytes of the first. This is because of an
    // invariant guaranteed by `VerifyingKey`: the second field is always the (unique)
    // decompression of the first. Thus, the serialized first field is a unique representation of
    // the entire `VerifyingKey`.
    let hrams: Vec<[u8; 64]> = (0..signatures.len())
        .map(|i| {
            // Compute H(R || A || M), where
            // R = sig.R
            // A = verifying key
            // M = msg
            let mut h: Sha512 = Sha512::default();
            h.update(signatures[i].r_bytes());
            h.update(verifying_keys[i].as_bytes());
            h.update(messages[i]);
            *h.finalize().as_ref()
        })
        .collect();

    // Update transcript with the hashes above. This covers verifying_keys, messages, and the R
    // half of signatures
    for hram in hrams.iter() {
        transcript.append_message(b"hram", hram);
    }
    // Update transcript with the rest of the data. This covers the s half of the signatures
    for sig in signatures {
        transcript.append_message(b"sig.s", sig.s_bytes());
    }

    // All function inputs have now been hashed into the transcript. Finalize it and use it as
    // randomness for the batch verification.
    let mut rng = transcript.build_rng().finalize(&mut ZeroRng);

    // Convert all signatures to `InternalSignature`
    let signatures = signatures
        .iter()
        .map(InternalSignature::try_from)
        .collect::<Result<Vec<_>, _>>()?;
    // Convert the H(R || A || M) values into scalars
    let hrams: Vec<Scalar> = hrams
        .iter()
        .map(Scalar::from_bytes_mod_order_wide)
        .collect();

    // Select a random 128-bit scalar for each signature.
    let zs: Vec<Scalar> = signatures
        .iter()
        .map(|_| Scalar::from(gen_u128(&mut rng)))
        .collect();

    // Compute the basepoint coefficient, ∑ s[i]z[i] (mod l)
    let B_coefficient: Scalar = signatures
        .iter()
        .map(|sig| sig.s)
        .zip(zs.iter())
        .map(|(s, z)| z * s)
        .sum();

    // Multiply each H(R || A || M) by the random value
    let zhrams = hrams.iter().zip(zs.iter()).map(|(hram, z)| hram * z);

    let Rs = signatures.iter().map(|sig| sig.R.decompress());
    let As = verifying_keys.iter().map(|pk| Some(pk.point));
    let B = once(Some(constants::ED25519_BASEPOINT_POINT));

    // Compute (-∑ z[i]s[i] (mod l)) B + ∑ z[i]R[i] + ∑ (z[i]H(R||A||M)[i] (mod l)) A[i] = 0
    let id = EdwardsPoint::optional_multiscalar_mul(
        once(-B_coefficient).chain(zs.iter().cloned()).chain(zhrams),
        B.chain(Rs).chain(As),
    )
    .ok_or(InternalError::Verify)?;

    if id.is_identity() {
        Ok(())
    } else {
        Err(InternalError::Verify.into())
    }
}
