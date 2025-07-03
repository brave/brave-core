// -*- mode: rust; -*-
//
// This file is part of schnorrkel.
// Copyright (c) 2017-2019 isis lovecruft
// Copyright (c) 2019 Web 3 Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Jeffrey Burdges <jeff@web3.foundation>
// - isis agora lovecruft <isis@patternsinthevoid.net>

//! ### Schnorr signature batch verification.

use curve25519_dalek::constants;
use curve25519_dalek::ristretto::{CompressedRistretto, RistrettoPoint};
use curve25519_dalek::scalar::Scalar;

use super::*;
use crate::context::{SigningTranscript};

#[cfg(feature = "alloc")]
use alloc::vec::Vec;

const ASSERT_MESSAGE: &str = "The number of messages/transcripts, signatures, and public keys must be equal.";


/// Verify a batch of `signatures` on `messages` with their respective `public_keys`.
///
/// # Inputs
///
/// * `messages` is a slice of byte slices, one per signed message.
/// * `signatures` is a slice of `Signature`s.
/// * `public_keys` is a slice of `PublicKey`s.
/// * `deduplicate_public_keys`
/// * `csprng` is an implementation of `RngCore+CryptoRng`, such as `rand::ThreadRng`.
///
/// # Panics
///
/// This function will panic if the `messages, `signatures`, and `public_keys`
/// slices are not equal length.
///
/// # Returns
///
/// * A `Result` whose `Ok` value is an empty tuple and whose `Err` value is a
///   `SignatureError` containing a description of the internal error which
///   occurred.
///
/// # Examples
///
/// ```
/// use schnorrkel::{Keypair,PublicKey,Signature,verify_batch,signing_context};
///
/// # fn main() {
/// let ctx = signing_context(b"some batch");
/// let mut csprng = rand::thread_rng();
/// let keypairs: Vec<Keypair> = (0..64).map(|_| Keypair::generate_with(&mut csprng)).collect();
/// let msg: &[u8] = b"They're good dogs Brant";
/// let signatures:  Vec<Signature> = keypairs.iter().map(|key| key.sign(ctx.bytes(&msg))).collect();
/// let public_keys: Vec<PublicKey> = keypairs.iter().map(|key| key.public).collect();
///
/// let transcripts = std::iter::once(ctx.bytes(msg)).cycle().take(64);
///
/// assert!( verify_batch(transcripts, &signatures[..], &public_keys[..], false).is_ok() );
/// # }
/// ```
pub fn verify_batch<T,I>(
    transcripts: I,
    signatures: &[Signature],
    public_keys: &[PublicKey],
    deduplicate_public_keys: bool,
) -> SignatureResult<()>
where
    T: SigningTranscript,
    I: IntoIterator<Item=T>,
{
    verify_batch_rng(transcripts, signatures, public_keys, deduplicate_public_keys, getrandom_or_panic())
}

struct NotAnRng;
impl rand_core::RngCore for NotAnRng {
    fn next_u32(&mut self) -> u32 { rand_core::impls::next_u32_via_fill(self) }

    fn next_u64(&mut self) -> u64 { rand_core::impls::next_u64_via_fill(self) }

    /// A no-op function which leaves the destination bytes for randomness unchanged.
    fn fill_bytes(&mut self, dest: &mut [u8]) { zeroize::Zeroize::zeroize(dest) }

    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), rand_core::Error> {
        self.fill_bytes(dest);
        Ok(())
    }
}
impl rand_core::CryptoRng for NotAnRng {}

/// Verify a batch of `signatures` on `messages` with their respective `public_keys`.
///
/// Avoids using system randomness and instead depends entirely upon delinearization.
///
/// We break the `R: CryptRng` requirement from `verify_batch_rng`
/// here, but this appears fine using an Fiat-Shamir transform with
/// an argument similar to 
/// [public key delinearization](https://crypto.stanford.edu/~dabo/pubs/papers/BLSmultisig.html).
///
/// We caution deeterministic delinearization could interact poorly
/// with other functionality, *if* one delinarization scalar were
/// left constant.  We do not make that mistake here.
pub fn verify_batch_deterministic<T,I>(
    transcripts: I,
    signatures: &[Signature],
    public_keys: &[PublicKey],
    deduplicate_public_keys: bool,
) -> SignatureResult<()>
where
    T: SigningTranscript,
    I: IntoIterator<Item=T>,
{
    verify_batch_rng(transcripts, signatures, public_keys, deduplicate_public_keys, NotAnRng)
}

/// Verify a batch of `signatures` on `messages` with their respective `public_keys`.
///
/// Inputs and return agree with `verify_batch` except the user supplies their own random number generator.
pub fn verify_batch_rng<T,I,R>(
    transcripts: I,
    signatures: &[Signature],
    public_keys: &[PublicKey],
    deduplicate_public_keys: bool,
    rng: R,
) -> SignatureResult<()>
where
    T: SigningTranscript,
    I: IntoIterator<Item=T>,
    R: RngCore+CryptoRng,
{
    assert!(signatures.len() == public_keys.len(), "{}", ASSERT_MESSAGE);  // Check transcripts length below

    let (zs, hrams) = prepare_batch(transcripts, signatures, public_keys, rng);

    // Compute the basepoint coefficient, ∑ s[i]z[i] (mod l)
    let bs: Scalar = signatures.iter()
        .map(|sig| sig.s)
        .zip(zs.iter())
        .map(|(s, z)| z * s)
        .sum();

    verify_batch_equation( bs, zs, hrams, signatures, public_keys, deduplicate_public_keys )
}


trait HasR {
    #[allow(non_snake_case)]
    fn get_R(&self) -> &CompressedRistretto;
}
impl HasR for Signature {
    #[allow(non_snake_case)]
    fn get_R(&self) -> &CompressedRistretto { &self.R }
}
impl HasR for CompressedRistretto {
    #[allow(non_snake_case)]
    fn get_R(&self) -> &CompressedRistretto { self }
}

/// First phase of batch verification that computes the delinierizing
/// coefficents and challenge hashes
#[allow(non_snake_case)]
fn prepare_batch<T,I,R>(
    transcripts: I,
    signatures: &[impl HasR],
    public_keys: &[PublicKey],
    mut rng: R,
) -> (Vec<Scalar>,Vec<Scalar>)
where
    T: SigningTranscript,
    I: IntoIterator<Item=T>,
    R: RngCore+CryptoRng,
{

    // Assumulate public keys, signatures, and transcripts for pseudo-random delinearization scalars
    let mut zs_t = merlin::Transcript::new(b"V-RNG");
    for pk in public_keys {
        zs_t.commit_point(b"",pk.as_compressed());
    }
    for sig in signatures {
        zs_t.commit_point(b"",sig.get_R());
    }

    // We might collect here anyways, but right now you cannot have
    //   IntoIterator<Item=T, IntoIter: ExactSizeIterator+TrustedLen>
    let mut transcripts = transcripts.into_iter();
    // Compute H(R || A || M) for each (signature, public_key, message) triplet
    let hrams: Vec<Scalar> = transcripts.by_ref()
        .zip(0..signatures.len())
        .map( |(mut t,i)| {
            let mut d = [0u8; 16];
            t.witness_bytes_rng(b"", &mut d, &[&[]], NotAnRng);  // Could speed this up using ZeroRng
            zs_t.append_message(b"",&d);

            t.proto_name(b"Schnorr-sig");
            t.commit_point(b"sign:pk",public_keys[i].as_compressed());
            t.commit_point(b"sign:R",signatures[i].get_R());
            t.challenge_scalar(b"sign:c")  // context, message, A/public_key, R=rG
        } ).collect();
    assert!(transcripts.next().is_none(), "{}", ASSERT_MESSAGE);
    assert!(hrams.len() == public_keys.len(), "{}", ASSERT_MESSAGE);

    // Use a random number generator keyed by both the public keys,
    // and the system random number generator
    let mut csprng = zs_t.build_rng().finalize(&mut rng);
    // Select a random 128-bit scalar for each signature.
    // We may represent these as scalars because we use
    // variable time 256 bit multiplication below.
    let rnd_128bit_scalar = |_| {
        let mut r = [0u8; 16];
        csprng.fill_bytes(&mut r);
        Scalar::from(u128::from_le_bytes(r))
    };
    let zs: Vec<Scalar> = signatures.iter().map(rnd_128bit_scalar).collect();

    (zs, hrams)
}

/// Last phase of batch verification that checks the verification equation
#[allow(non_snake_case)]
fn verify_batch_equation(
    bs: Scalar,
    zs: Vec<Scalar>,
    mut hrams: Vec<Scalar>,
    signatures: &[impl HasR],
    public_keys: &[PublicKey],
    deduplicate_public_keys: bool,
) -> SignatureResult<()>
{
    use curve25519_dalek::traits::IsIdentity;
    use curve25519_dalek::traits::VartimeMultiscalarMul;

    use core::iter::once;

    let B = once(Some(constants::RISTRETTO_BASEPOINT_POINT));

    let Rs = signatures.iter().map(|sig| sig.get_R().decompress());

    let mut ppks = Vec::new();
    let As = if ! deduplicate_public_keys {
        // Multiply each H(R || A || M) by the random value
        for (hram, z) in hrams.iter_mut().zip(zs.iter()) {
            *hram *= z;
        }
        public_keys
    } else {
        // TODO: Actually deduplicate all if deduplicate_public_keys is set?
        ppks.reserve( public_keys.len() );
        // Multiply each H(R || A || M) by the random value
        for i in 0..public_keys.len() {
            let zhram = hrams[i] * zs[i];
            let j = ppks.len().checked_sub(1);
            if j.is_none() || ppks[j.unwrap()] != public_keys[i] {
                ppks.push(public_keys[i]);
                hrams[ppks.len()-1] = zhram;
            } else {
                hrams[ppks.len()-1] = hrams[ppks.len()-1] + zhram;
            }
        }
        hrams.truncate(ppks.len());
        ppks.as_slice()
    }.iter().map(|pk| Some(*pk.as_point()));

    // Compute (-∑ z[i]s[i] (mod l)) B + ∑ z[i]R[i] + ∑ (z[i]H(R||A||M)[i] (mod l)) A[i] = 0
    let b = RistrettoPoint::optional_multiscalar_mul(
        once(-bs).chain(zs.iter().cloned()).chain(hrams),
        B.chain(Rs).chain(As),
    ).map(|id| id.is_identity()).unwrap_or(false);
    // We need not return SignatureError::PointDecompressionError because
    // the decompression failures occur for R represent invalid signatures.

    if b { Ok(()) } else { Err(SignatureError::EquationFalse) }
}



/// Half-aggregated aka prepared batch signature
/// 
/// Implementation of "Non-interactive half-aggregation of EdDSA and
/// variantsof Schnorr signatures" by  Konstantinos Chalkias,
/// François Garillot, Yashvanth Kondi, and Valeria Nikolaenko
/// available from https://eprint.iacr.org/2021/350.pdf
#[allow(non_snake_case)]
pub struct PreparedBatch {
    bs: Scalar,
    Rs: Vec<CompressedRistretto>,
}

impl PreparedBatch{

    /// Create a half-aggregated aka prepared batch signature from many other signatures.
    #[allow(non_snake_case)]
    pub fn new<T,I,R>(
        transcripts: I,
        signatures: &[Signature],
        public_keys: &[PublicKey],
    ) -> PreparedBatch
    where
        T: SigningTranscript,
        I: IntoIterator<Item=T>,
    {
        assert!(signatures.len() == public_keys.len(), "{}", ASSERT_MESSAGE);  // Check transcripts length below

        let (zs, _hrams) = prepare_batch(transcripts, signatures, public_keys, NotAnRng);

        // Compute the basepoint coefficient, ∑ s[i]z[i] (mod l)
        let bs: Scalar = signatures.iter()
            .map(|sig| sig.s)
            .zip(zs.iter())
            .map(|(s, z)| z * s)
            .sum();

        let Rs = signatures.iter().map(|sig| sig.R).collect();
        PreparedBatch { bs, Rs, }
    }

    /// Verify a half-aggregated aka prepared batch signature
    #[allow(non_snake_case)]
    pub fn verify<T,I>(
        &self,
        transcripts: I,
        public_keys: &[PublicKey],
        deduplicate_public_keys: bool,
    ) -> SignatureResult<()>
    where
        T: SigningTranscript,
        I: IntoIterator<Item=T>,
    {
        assert!(self.Rs.len() == public_keys.len(), "{}", ASSERT_MESSAGE);  // Check transcripts length below

        let (zs, hrams) = prepare_batch(transcripts, self.Rs.as_slice(), public_keys, NotAnRng);

        verify_batch_equation(
            self.bs,
            zs, hrams,
            self.Rs.as_slice(),
            public_keys, deduplicate_public_keys
        )
    }

    /// Reads a `PreparedBatch` from a correctly sized buffer
    #[allow(non_snake_case)]
    pub fn read_bytes(&self, mut bytes: &[u8]) -> SignatureResult<PreparedBatch> {
        use arrayref::array_ref;
        if bytes.len() % 32 != 0 || bytes.len() < 64 { 
            return Err(SignatureError::BytesLengthError {
                name: "PreparedBatch",
                description: "A Prepared batched signature",
                length: 0 // TODO: Maybe get rid of this silly field?
            });
        }
        let l = (bytes.len() % 32) - 1;
        let mut read = || {
            let (head,tail) = bytes.split_at(32);
            bytes = tail;
            *array_ref![head,0,32]
        };
        let mut bs = read();
        bs[31] &= 127;
        let bs = super::sign::check_scalar(bs) ?;
        let mut Rs = Vec::with_capacity(l);
        for _ in 0..l {
            Rs.push( CompressedRistretto(read()) );
        }
        Ok(PreparedBatch { bs, Rs })
    }
    
    /// Returns buffer size required for serialization
    #[allow(non_snake_case)]
    pub fn byte_len(&self) -> usize {
        32 + 32 * self.Rs.len()
    }

    /// Serializes into exactly sized buffer
    #[allow(non_snake_case)]
    pub fn write_bytes(&self, mut bytes: &mut [u8]) {
        assert!(bytes.len() == self.byte_len());        
        let mut place = |s: &[u8]| reserve_mut(&mut bytes,32).copy_from_slice(s);
        let mut bs = self.bs.to_bytes();
        bs[31] |= 128;
        place(&bs[..]);
        for R in self.Rs.iter() {
            place(R.as_bytes());
        }
    }    
}


pub fn reserve_mut<'heap, T>(heap: &mut &'heap mut [T], len: usize) -> &'heap mut [T] {
    let tmp: &'heap mut [T] = core::mem::take(&mut *heap);
    let (reserved, tmp) = tmp.split_at_mut(len);
    *heap = tmp;
    reserved
}


#[cfg(test)]
mod test {
    #[cfg(feature = "alloc")]
    use alloc::vec::Vec;

    use rand::prelude::*; // ThreadRng,thread_rng

    use super::super::*;

    #[cfg(feature = "alloc")]
    #[test]
    fn verify_batch_seven_signatures() {
        let ctx = signing_context(b"my batch context");

        let messages: [&[u8]; 7] = [
            b"Watch closely everyone, I'm going to show you how to kill a god.",
            b"I'm not a cryptographer I just encrypt a lot.",
            b"Still not a cryptographer.",
            b"This is a test of the tsunami alert system. This is only a test.",
            b"Fuck dumbin' it down, spit ice, skip jewellery: Molotov cocktails on me like accessories.",
            b"Hey, I never cared about your bucks, so if I run up with a mask on, probably got a gas can too.",
            b"And I'm not here to fill 'er up. Nope, we came to riot, here to incite, we don't want any of your stuff.", ];
        let mut csprng: ThreadRng = thread_rng();
        let mut keypairs: Vec<Keypair> = Vec::new();
        let mut signatures: Vec<Signature> = Vec::new();

        for i in 0..messages.len() {
            let mut keypair: Keypair = Keypair::generate_with(&mut csprng);
            if i == 3 || i == 4 { keypair = keypairs[0].clone(); }
            signatures.push(keypair.sign(ctx.bytes(messages[i])));
            keypairs.push(keypair);
        }
        let mut public_keys: Vec<PublicKey> = keypairs.iter().map(|key| key.public).collect();

        public_keys.swap(1,2);
        let transcripts = messages.iter().map(|m| ctx.bytes(m));
        assert!( verify_batch(transcripts, &signatures[..], &public_keys[..], false).is_err() );
        let transcripts = messages.iter().map(|m| ctx.bytes(m));
        assert!( verify_batch(transcripts, &signatures[..], &public_keys[..], true).is_err() );

        public_keys.swap(1,2);
        let transcripts = messages.iter().map(|m| ctx.bytes(m));
        assert!( verify_batch(transcripts, &signatures[..], &public_keys[..], false).is_ok() );
        let transcripts = messages.iter().map(|m| ctx.bytes(m));
        assert!( verify_batch(transcripts, &signatures[..], &public_keys[..], true).is_ok() );

        signatures.swap(1,2);
        let transcripts = messages.iter().map(|m| ctx.bytes(m));
        assert!( verify_batch(transcripts, &signatures[..], &public_keys[..], false).is_err() );
        let transcripts = messages.iter().map(|m| ctx.bytes(m));
        assert!( verify_batch(transcripts, &signatures[..], &public_keys[..], true).is_err() );
    }
}
