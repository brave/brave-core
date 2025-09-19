// -*- mode: rust; -*-
//
// This file is part of schnorrkel.
// Copyright (c) 2019 Web 3 Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Jeffrey Burdges <jeff@web3.foundation>

//! ### Schnorr signature contexts and configuration, adaptable to most Schnorr signature schemes.

use core::cell::RefCell;

use rand_core::{RngCore,CryptoRng};

use merlin::Transcript;

use curve25519_dalek::digest::{Update,FixedOutput,ExtendableOutput,XofReader};
use curve25519_dalek::digest::generic_array::typenum::{U32,U64};

use curve25519_dalek::ristretto::CompressedRistretto; // RistrettoPoint
use curve25519_dalek::scalar::Scalar;


// === Signing context as transcript === //

/// Schnorr signing transcript
///
/// We envision signatures being on messages, but if a signature occurs
/// inside a larger protocol then the signature scheme's internal
/// transcript may exist before or persist after signing.
///
/// In this trait, we provide an interface for Schnorr signature-like
/// constructions that is compatable with `merlin::Transcript`, but
/// abstract enough to support conventional hash functions as well.
///
/// We warn however that conventional hash functions do not provide
/// strong enough domain seperation for usage via `&mut` references.
///
/// We fold randomness into witness generation here too, which
/// gives every function that takes a `SigningTranscript` a default
/// argument `rng: impl Rng = thread_rng()` too.
///
/// We also abstract over owned and borrowed `merlin::Transcript`s,
/// so that simple use cases do not suffer from our support for.
pub trait SigningTranscript {
    /// Extend transcript with some bytes, shadowed by `merlin::Transcript`.
    fn commit_bytes(&mut self, label: &'static [u8], bytes: &[u8]);

    /// Extend transcript with a protocol name
    fn proto_name(&mut self, label: &'static [u8]) {
        self.commit_bytes(b"proto-name", label);
    }

    /// Extend the transcript with a compressed Ristretto point
    fn commit_point(&mut self, label: &'static [u8], compressed: &CompressedRistretto) {
        self.commit_bytes(label, compressed.as_bytes());
    }

    /// Produce some challenge bytes, shadowed by `merlin::Transcript`.
    fn challenge_bytes(&mut self, label: &'static [u8], dest: &mut [u8]);

    /// Produce the public challenge scalar `e`.
    fn challenge_scalar(&mut self, label: &'static [u8]) -> Scalar {
        let mut buf = [0; 64];
        self.challenge_bytes(label, &mut buf);
        Scalar::from_bytes_mod_order_wide(&buf)
    }

    /// Produce a secret witness scalar `k`, aka nonce, from the protocol
    /// transcript and any "nonce seeds" kept with the secret keys.
    fn witness_scalar(&self, label: &'static [u8], nonce_seeds: &[&[u8]]) -> Scalar {
        let mut scalar_bytes = [0u8; 64];
        self.witness_bytes(label, &mut scalar_bytes, nonce_seeds);
        Scalar::from_bytes_mod_order_wide(&scalar_bytes)
    }

    /// Produce secret witness bytes from the protocol transcript
    /// and any "nonce seeds" kept with the secret keys.
    fn witness_bytes(&self, label: &'static [u8], dest: &mut [u8], nonce_seeds: &[&[u8]]) {
        self.witness_bytes_rng(label, dest, nonce_seeds, super::getrandom_or_panic())
    }

    /// Produce secret witness bytes from the protocol transcript
    /// and any "nonce seeds" kept with the secret keys.
    fn witness_bytes_rng<R>(&self, label: &'static [u8], dest: &mut [u8], nonce_seeds: &[&[u8]], rng: R)
    where R: RngCore+CryptoRng;
}


/// We delegates any mutable reference to its base type, like `&mut Rng`
/// or similar to `BorrowMut<..>` do, but doing so here simplifies
/// alternative implementations.
impl<T> SigningTranscript for &mut T
where T: SigningTranscript + ?Sized,
{
    #[inline(always)]
    fn commit_bytes(&mut self, label: &'static [u8], bytes: &[u8])
        {  (**self).commit_bytes(label,bytes)  }
    #[inline(always)]
    fn proto_name(&mut self, label: &'static [u8])
        {  (**self).proto_name(label)  }
    #[inline(always)]
    fn commit_point(&mut self, label: &'static [u8], compressed: &CompressedRistretto)
        {  (**self).commit_point(label, compressed)  }
    #[inline(always)]
    fn challenge_bytes(&mut self, label: &'static [u8], dest: &mut [u8])
        {  (**self).challenge_bytes(label,dest)  }
    #[inline(always)]
    fn challenge_scalar(&mut self, label: &'static [u8]) -> Scalar
        {  (**self).challenge_scalar(label)  }
    #[inline(always)]
    fn witness_scalar(&self, label: &'static [u8], nonce_seeds: &[&[u8]]) -> Scalar
        {  (**self).witness_scalar(label,nonce_seeds)  }
    #[inline(always)]
    fn witness_bytes(&self, label: &'static [u8], dest: &mut [u8], nonce_seeds: &[&[u8]])
        {  (**self).witness_bytes(label,dest,nonce_seeds)  }
    #[inline(always)]
    fn witness_bytes_rng<R>(&self, label: &'static [u8], dest: &mut [u8], nonce_seeds: &[&[u8]], rng: R)
    where R: RngCore+CryptoRng
        {  (**self).witness_bytes_rng(label,dest,nonce_seeds,rng)  }
}

/// We delegate `SigningTranscript` methods to the corresponding
/// inherent methods of `merlin::Transcript` and implement two
/// witness methods to avoid overwriting the `merlin::TranscriptRng`
/// machinery.
impl SigningTranscript for Transcript {
    fn commit_bytes(&mut self, label: &'static [u8], bytes: &[u8]) {
        Transcript::append_message(self, label, bytes)
    }

    fn challenge_bytes(&mut self, label: &'static [u8], dest: &mut [u8]) {
        Transcript::challenge_bytes(self, label, dest)
    }

    fn witness_bytes_rng<R>(&self, label: &'static [u8], dest: &mut [u8], nonce_seeds: &[&[u8]], mut rng: R)
    where R: RngCore+CryptoRng
    {
        let mut br = self.build_rng();
        for ns in nonce_seeds {
            br = br.rekey_with_witness_bytes(label, ns);
        }
        let mut r = br.finalize(&mut rng);
        r.fill_bytes(dest)
    }
}


/// Schnorr signing context
///
/// We expect users to have separate `SigningContext`s for each role
/// that signature play in their protocol.  These `SigningContext`s
/// may be global `lazy_static!`s, or perhaps constants in future.
///
/// To sign a message, apply the appropriate inherent method to create
/// a signature transcript.
///
/// You should use `merlin::Transcript`s directly if you must do
/// anything more complex, like use signatures in larger zero-knowledge
/// protocols or sign several components but only reveal one later.
///
/// We declare these methods `#[inline(always)]` because rustc does
/// not handle large returns as efficiently as one might like.
/// https://github.com/rust-random/rand/issues/817
#[derive(Clone)] // Debug
pub struct SigningContext(Transcript);

/// Initialize a signing context from a static byte string that
/// identifies the signature's role in the larger protocol.
#[inline(always)]
pub fn signing_context(context : &[u8]) -> SigningContext {
    SigningContext::new(context)
}

impl SigningContext {
    /// Initialize a signing context from a static byte string that
    /// identifies the signature's role in the larger protocol.
    #[inline(always)]
    pub fn new(context : &[u8]) -> SigningContext {
        let mut t = Transcript::new(b"SigningContext");
        t.append_message(b"",context);
        SigningContext(t)
    }

    /// Initialize an owned signing transcript on a message provided as a byte array.
    ///
    /// Avoid this method when processing large slices because it
    /// calls `merlin::Transcript::append_message` directly and
    /// `merlin` is designed for domain seperation, not performance.
    #[inline(always)]
    pub fn bytes(&self, bytes: &[u8]) -> Transcript {
        let mut t = self.0.clone();
        t.append_message(b"sign-bytes", bytes);
        t
    }

    /// Initialize an owned signing transcript on a message provided
    /// as a hash function with extensible output mode (XOF) by
    /// finalizing the hash and extracting 32 bytes from XOF.
    #[inline(always)]
    pub fn xof<D: ExtendableOutput>(&self, h: D) -> Transcript {
        let mut prehash = [0u8; 32];
        h.finalize_xof().read(&mut prehash);
        let mut t = self.0.clone();
        t.append_message(b"sign-XoF", &prehash);
        t
    }

    /// Initialize an owned signing transcript on a message provided as
    /// a hash function with 256 bit output.
    #[inline(always)]
    pub fn hash256<D: FixedOutput<OutputSize=U32>>(&self, h: D) -> Transcript {
        let mut prehash = [0u8; 32];
        prehash.copy_from_slice(h.finalize_fixed().as_slice());
        let mut t = self.0.clone();
        t.append_message(b"sign-256", &prehash);
        t
    }

    /// Initialize an owned signing transcript on a message provided as
    /// a hash function with 512 bit output, usually a gross over kill.
    #[inline(always)]
    pub fn hash512<D: FixedOutput<OutputSize=U64>>(&self, h: D) -> Transcript {
        let mut prehash = [0u8; 64];
        prehash.copy_from_slice(h.finalize_fixed().as_slice());
        let mut t = self.0.clone();
        t.append_message(b"sign-512", &prehash);
        t
    }
}


/// Very simple transcript construction from a modern hash function.
///
/// We provide this transcript type to directly use conventional hash
/// functions with an extensible output mode, like Shake128 and
/// Blake2x.
///
/// We recommend using `merlin::Transcript` instead because merlin
/// provides the transcript abstraction natively and might function
/// better in low memory environments.  We therefore do not provide
/// conveniences like `signing_context` for this.
///
/// We note that merlin already uses Keccak, upon which Shake128 is based,
/// and that no rust implementation for Blake2x currently exists.
///
/// We caution that our transcript abstractions cannot provide the
/// protections against hash collisions that Ed25519 provides via
/// double hashing, but that prehashed Ed25519 variants lose.
/// As such, any hash function used here must be collision resistant.
/// We strongly recommend against building XOFs from weaker hash
/// functions like SHA1 with HKDF constructions or similar.
///
/// In `XoFTranscript` style, we never expose the hash function `H`
/// underlying this type, so that developers cannot circumvent the
/// domain separation provided by our methods.  We do this to make
/// `&mut XoFTranscript : SigningTranscript` safe.
pub struct XoFTranscript<H>(H)
where H: Update + ExtendableOutput + Clone;

fn input_bytes<H: Update>(h: &mut H, bytes: &[u8]) {
    let l = bytes.len() as u64;
    h.update(&l.to_le_bytes());
    h.update(bytes);
}

impl<H> XoFTranscript<H>
where H: Update + ExtendableOutput + Clone
{
    /// Create a `XoFTranscript` from a conventional hash functions with an extensible output mode.
    ///
    /// We intentionally consume and never reexpose the hash function
    /// provided, so that our domain separation works correctly even
    /// when using `&mut XoFTranscript : SigningTranscript`.
    #[inline(always)]
    pub fn new(h: H) -> XoFTranscript<H> { XoFTranscript(h) }
}

impl<H> From<H> for XoFTranscript<H>
where H: Update + ExtendableOutput + Clone
{
    #[inline(always)]
    fn from(h: H) -> XoFTranscript<H> { XoFTranscript(h) }
}

impl<H> SigningTranscript for XoFTranscript<H>
where H: Update + ExtendableOutput + Clone
{
    fn commit_bytes(&mut self, label: &'static [u8], bytes: &[u8]) {
        self.0.update(b"co");
        input_bytes(&mut self.0, label);
        input_bytes(&mut self.0, bytes);
    }

    fn challenge_bytes(&mut self, label: &'static [u8], dest: &mut [u8]) {
        self.0.update(b"ch");
        input_bytes(&mut self.0, label);
        let l = dest.len() as u64;
        self.0.update(&l.to_le_bytes());
        self.0.clone().chain(b"xof").finalize_xof().read(dest);
    }

    fn witness_bytes_rng<R>(&self, label: &'static [u8], dest: &mut [u8], nonce_seeds: &[&[u8]], mut rng: R)
    where R: RngCore+CryptoRng
    {
        let mut h = self.0.clone().chain(b"wb");
        input_bytes(&mut h, label);
        for ns in nonce_seeds {
            input_bytes(&mut h, ns);
        }
        let l = dest.len() as u64;
        h.update(&l.to_le_bytes());

        let mut r = [0u8; 32];
        rng.fill_bytes(&mut r);
        h.update(&r);
        h.finalize_xof().read(dest);
    }
}


/// Schnorr signing transcript with the default `ThreadRng` replaced
/// by an arbitrary `CryptoRng`.
///
/// If `ThreadRng` breaks on your platform, or merely if you're paranoid,
/// then you might "upgrade" from `ThreadRng` to `OsRng` by using calls
/// like `keypair.sign( attach_rng(t,OSRng::new()) )`.
/// However, we recommend instead simply fixing `ThreadRng` for your platform.
///
/// There are also derandomization tricks like
/// `attach_rng(t,ChaChaRng::from_seed([0u8; 32]))`
/// for deterministic signing in tests too.  Although derandomization
/// produces secure signatures, we recommend against doing this in
/// production because we implement protocols like multi-signatures
/// which likely become vulnerable when derandomized.
pub struct SigningTranscriptWithRng<T,R>
where T: SigningTranscript, R: RngCore+CryptoRng
{
    t: T,
    rng: RefCell<R>,
}

impl<T,R> SigningTranscript for SigningTranscriptWithRng<T,R>
where T: SigningTranscript, R: RngCore+CryptoRng
{
    fn commit_bytes(&mut self, label: &'static [u8], bytes: &[u8])
        {  self.t.commit_bytes(label, bytes)  }

    fn challenge_bytes(&mut self, label: &'static [u8], dest: &mut [u8])
        {  self.t.challenge_bytes(label, dest)  }

    fn witness_bytes(&self, label: &'static [u8], dest: &mut [u8], nonce_seeds: &[&[u8]])
       {  self.witness_bytes_rng(label, dest, nonce_seeds, &mut *self.rng.borrow_mut())  }

    fn witness_bytes_rng<RR>(&self, label: &'static [u8], dest: &mut [u8], nonce_seeds: &[&[u8]], rng: RR)
    where RR: RngCore+CryptoRng
       {  self.t.witness_bytes_rng(label,dest,nonce_seeds,rng)  }

}

/// Attach a `CryptoRng` to a `SigningTranscript` to replace the default `ThreadRng`.
///
/// There are tricks like `attach_rng(t,ChaChaRng::from_seed([0u8; 32]))`
/// for deterministic tests.  We warn against doing this in production
/// however because, although such derandomization produces secure Schnorr
/// signatures, we do implement protocols here like multi-signatures which
/// likely become vulnerable when derandomized.
pub fn attach_rng<T,R>(t: T, rng: R) -> SigningTranscriptWithRng<T,R>
where T: SigningTranscript, R: RngCore+CryptoRng
{
    SigningTranscriptWithRng {
        t, rng: RefCell::new(rng)
    }
}

#[cfg(feature = "rand_chacha")]
use rand_chacha::ChaChaRng;

/// Attach a `ChaChaRng` to a `Transcript` to repalce the default `ThreadRng`
#[cfg(feature = "rand_chacha")]
pub fn attach_chacharng<T>(t: T, seed: [u8; 32]) -> SigningTranscriptWithRng<T,ChaChaRng>
where T: SigningTranscript
{
    use rand_core::SeedableRng;
    attach_rng(t,ChaChaRng::from_seed(seed))
}
