use rand_core;
use zeroize::Zeroize;

use crate::strobe::Strobe128;

fn encode_u64(x: u64) -> [u8; 8] {
    use byteorder::{ByteOrder, LittleEndian};

    let mut buf = [0; 8];
    LittleEndian::write_u64(&mut buf, x);
    buf
}

fn encode_usize_as_u32(x: usize) -> [u8; 4] {
    use byteorder::{ByteOrder, LittleEndian};

    assert!(x <= (u32::max_value() as usize));

    let mut buf = [0; 4];
    LittleEndian::write_u32(&mut buf, x as u32);
    buf
}

/// A transcript of a public-coin argument.
///
/// The prover's messages are added to the transcript using
/// [`append_message`](Transcript::append_message), and the verifier's
/// challenges can be computed using
/// [`challenge_bytes`](Transcript::challenge_bytes).
///
/// # Creating and using a Merlin transcript
///
/// To create a Merlin transcript, use [`Transcript::new()`].  This
/// function takes a domain separation label which should be unique to
/// the application.
///
/// To use the transcript with a Merlin-based proof implementation,
/// the prover's side creates a Merlin transcript with an
/// application-specific domain separation label, and passes a `&mut`
/// reference to the transcript to the proving function(s).
///
/// To verify the resulting proof, the verifier creates their own
/// Merlin transcript using the same domain separation label, then
/// passes a `&mut` reference to the verifier's transcript to the
/// verification function.
///
/// # Implementing proofs using Merlin
///
/// For information on the design of Merlin and how to use it to
/// implement a proof system, see the documentation at
/// [merlin.cool](https://merlin.cool), particularly the [Using
/// Merlin](https://merlin.cool/use/index.html) section.
#[derive(Clone, Zeroize)]
pub struct Transcript {
    strobe: Strobe128,
}

impl Transcript {
    /// Initialize a new transcript with the supplied `label`, which
    /// is used as a domain separator.
    ///
    /// # Note
    ///
    /// This function should be called by a proof library's API
    /// consumer (i.e., the application using the proof library), and
    /// **not by the proof implementation**.  See the [Passing
    /// Transcripts](https://merlin.cool/use/passing.html) section of
    /// the Merlin website for more details on why.
    pub fn new(label: &'static [u8]) -> Transcript {
        use crate::constants::MERLIN_PROTOCOL_LABEL;

        #[cfg(feature = "debug-transcript")]
        {
            use std::str::from_utf8;
            println!(
                "Initialize STROBE-128({})\t# b\"{}\"",
                hex::encode(MERLIN_PROTOCOL_LABEL),
                from_utf8(MERLIN_PROTOCOL_LABEL).unwrap(),
            );
        }

        let mut transcript = Transcript {
            strobe: Strobe128::new(MERLIN_PROTOCOL_LABEL),
        };
        transcript.append_message(b"dom-sep", label);

        transcript
    }

    /// Append a prover's `message` to the transcript.
    ///
    /// The `label` parameter is metadata about the message, and is
    /// also appended to the transcript.  See the [Transcript
    /// Protocols](https://merlin.cool/use/protocol.html) section of
    /// the Merlin website for details on labels.
    pub fn append_message(&mut self, label: &'static [u8], message: &[u8]) {
        let data_len = encode_usize_as_u32(message.len());
        self.strobe.meta_ad(label, false);
        self.strobe.meta_ad(&data_len, true);
        self.strobe.ad(message, false);

        #[cfg(feature = "debug-transcript")]
        {
            use std::str::from_utf8;

            match from_utf8(label) {
                Ok(label_str) => {
                    println!(
                        "meta-AD : {} || LE32({})\t# b\"{}\"",
                        hex::encode(label),
                        message.len(),
                        label_str
                    );
                }
                Err(_) => {
                    println!(
                        "meta-AD : {} || LE32({})",
                        hex::encode(label),
                        message.len()
                    );
                }
            }
            match from_utf8(message) {
                Ok(message_str) => {
                    println!("     AD : {}\t# b\"{}\"", hex::encode(message), message_str);
                }
                Err(_) => {
                    println!("     AD : {}", hex::encode(message));
                }
            }
        }
    }

    /// Deprecated.  This function was renamed to
    /// [`append_message`](Transcript::append_message).
    ///
    /// This is intended to avoid any possible confusion between the
    /// transcript-level messages and protocol-level commitments.
    #[deprecated(since = "1.1.0", note = "renamed to append_message for clarity.")]
    pub fn commit_bytes(&mut self, label: &'static [u8], message: &[u8]) {
        self.append_message(label, message);
    }

    /// Convenience method for appending a `u64` to the transcript.
    ///
    /// The `label` parameter is metadata about the message, and is
    /// also appended to the transcript.  See the [Transcript
    /// Protocols](https://merlin.cool/use/protocol.html) section of
    /// the Merlin website for details on labels.
    ///
    /// # Implementation
    ///
    /// Calls `append_message` with the 8-byte little-endian encoding
    /// of `x`.
    pub fn append_u64(&mut self, label: &'static [u8], x: u64) {
        self.append_message(label, &encode_u64(x));
    }

    /// Deprecated.  This function was renamed to
    /// [`append_u64`](Transcript::append_u64).
    ///
    /// This is intended to avoid any possible confusion between the
    /// transcript-level messages and protocol-level commitments.
    #[deprecated(since = "1.1.0", note = "renamed to append_u64 for clarity.")]
    pub fn commit_u64(&mut self, label: &'static [u8], x: u64) {
        self.append_u64(label, x);
    }

    /// Fill the supplied buffer with the verifier's challenge bytes.
    ///
    /// The `label` parameter is metadata about the challenge, and is
    /// also appended to the transcript.  See the [Transcript
    /// Protocols](https://merlin.cool/use/protocol.html) section of
    /// the Merlin website for details on labels.
    pub fn challenge_bytes(&mut self, label: &'static [u8], dest: &mut [u8]) {
        let data_len = encode_usize_as_u32(dest.len());
        self.strobe.meta_ad(label, false);
        self.strobe.meta_ad(&data_len, true);
        self.strobe.prf(dest, false);

        #[cfg(feature = "debug-transcript")]
        {
            use std::str::from_utf8;

            match from_utf8(label) {
                Ok(label_str) => {
                    println!(
                        "meta-AD : {} || LE32({})\t# b\"{}\"",
                        hex::encode(label),
                        dest.len(),
                        label_str
                    );
                }
                Err(_) => {
                    println!("meta-AD : {} || LE32({})", hex::encode(label), dest.len());
                }
            }
            println!("     PRF: {}", hex::encode(dest));
        }
    }

    /// Fork the current [`Transcript`] to construct an RNG whose output is bound
    /// to the current transcript state as well as prover's secrets.
    ///
    /// See the [`TranscriptRngBuilder`] documentation for more details.
    pub fn build_rng(&self) -> TranscriptRngBuilder {
        TranscriptRngBuilder {
            strobe: self.strobe.clone(),
        }
    }
}

/// Constructs a [`TranscriptRng`] by rekeying the [`Transcript`] with
/// prover secrets and an external RNG.
///
/// The prover uses a [`TranscriptRngBuilder`] to rekey with its
/// witness data, before using an external RNG to finalize to a
/// [`TranscriptRng`].  The resulting [`TranscriptRng`] will be a PRF
/// of all of the entire public transcript, the prover's secret
/// witness data, and randomness from the external RNG.
///
/// # Usage
///
/// To construct a [`TranscriptRng`], a prover calls
/// [`Transcript::build_rng()`] to clone the transcript state, then
/// uses [`rekey_with_witness_bytes()`][rekey_with_witness_bytes] to rekey the
/// transcript with the prover's secrets, before finally calling
/// [`finalize()`][finalize].  This rekeys the transcript with the
/// output of an external [`rand_core::RngCore`] instance and returns
/// a finalized [`TranscriptRng`].
///
/// These methods are intended to be chained, passing from a borrowed
/// [`Transcript`] to an owned [`TranscriptRng`] as follows:
/// ```
/// # extern crate merlin;
/// # extern crate rand_core;
/// # use merlin::Transcript;
/// # fn main() {
/// # let mut transcript = Transcript::new(b"TranscriptRng doctest");
/// # let public_data = b"public data";
/// # let witness_data = b"witness data";
/// # let more_witness_data = b"witness data";
/// transcript.append_message(b"public", public_data);
///
/// let mut rng = transcript
///     .build_rng()
///     .rekey_with_witness_bytes(b"witness1", witness_data)
///     .rekey_with_witness_bytes(b"witness2", more_witness_data)
///     .finalize(&mut rand_core::OsRng);
/// # }
/// ```
/// In this example, the final `rng` is a PRF of `public_data`
/// (as well as all previous `transcript` state), and of the prover's
/// secret `witness_data` and `more_witness_data`, and finally, of the
/// output of the thread-local RNG.
/// Note that because the [`TranscriptRng`] is produced from
/// [`finalize()`][finalize], it's impossible to forget
/// to rekey the transcript with external randomness.
///
/// # Note
///
/// Protocols that require randomness in multiple places (e.g., to
/// choose blinding factors for a multi-round protocol) should create
/// a fresh [`TranscriptRng`] **each time they need randomness**,
/// rather than reusing a single instance.  This ensures that the
/// randomness in each round is bound to the latest transcript state,
/// rather than just the state of the transcript when randomness was
/// first required.
///
/// # Typed Witness Data
///
/// Like the [`Transcript`], the [`TranscriptRngBuilder`] provides a
/// minimal, byte-oriented API, and like the [`Transcript`], this API
/// can be extended to allow rekeying with protocol-specific types
/// using an extension trait.  See the [Transcript
/// Protocols](https://merlin.cool/use/protocol.html) section of the
/// Merlin website for more details.
///
/// [rekey_with_witness_bytes]: TranscriptRngBuilder::rekey_with_witness_bytes
/// [finalize]: TranscriptRngBuilder::finalize
pub struct TranscriptRngBuilder {
    strobe: Strobe128,
}

impl TranscriptRngBuilder {
    /// Rekey the transcript using the provided witness data.
    ///
    /// The `label` parameter is metadata about `witness`.
    pub fn rekey_with_witness_bytes(
        mut self,
        label: &'static [u8],
        witness: &[u8],
    ) -> TranscriptRngBuilder {
        let witness_len = encode_usize_as_u32(witness.len());
        self.strobe.meta_ad(label, false);
        self.strobe.meta_ad(&witness_len, true);
        self.strobe.key(witness, false);

        self
    }

    /// Deprecated.  This function was renamed to
    /// [`rekey_with_witness_bytes`](Transcript::rekey_with_witness_bytes).
    ///
    /// This is intended to avoid any possible confusion between the
    /// transcript-level messages and protocol-level commitments.
    #[deprecated(
        since = "1.1.0",
        note = "renamed to rekey_with_witness_bytes for clarity."
    )]
    pub fn commit_witness_bytes(
        self,
        label: &'static [u8],
        witness: &[u8],
    ) -> TranscriptRngBuilder {
        self.rekey_with_witness_bytes(label, witness)
    }

    /// Use the supplied external `rng` to rekey the transcript, so
    /// that the finalized [`TranscriptRng`] is a PRF bound to
    /// randomness from the external RNG, as well as all other
    /// transcript data.
    pub fn finalize<R>(mut self, rng: &mut R) -> TranscriptRng
    where
        R: rand_core::RngCore + rand_core::CryptoRng,
    {
        let random_bytes = {
            let mut bytes = [0u8; 32];
            rng.fill_bytes(&mut bytes);
            bytes
        };

        self.strobe.meta_ad(b"rng", false);
        self.strobe.key(&random_bytes, false);

        TranscriptRng {
            strobe: self.strobe,
        }
    }
}

/// An RNG providing synthetic randomness to the prover.
///
/// A [`TranscriptRng`] is constructed from a [`Transcript`] using a
/// [`TranscriptRngBuilder`]; see its documentation for details on
/// how to construct one.
///
/// The transcript RNG construction is described in the [Generating
/// Randomness](https://merlin.cool/transcript/rng.html) section of
/// the Merlin website.
pub struct TranscriptRng {
    strobe: Strobe128,
}

impl rand_core::RngCore for TranscriptRng {
    fn next_u32(&mut self) -> u32 {
        rand_core::impls::next_u32_via_fill(self)
    }

    fn next_u64(&mut self) -> u64 {
        rand_core::impls::next_u64_via_fill(self)
    }

    fn fill_bytes(&mut self, dest: &mut [u8]) {
        let dest_len = encode_usize_as_u32(dest.len());
        self.strobe.meta_ad(&dest_len, false);
        self.strobe.prf(dest, false);
    }

    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), rand_core::Error> {
        self.fill_bytes(dest);
        Ok(())
    }
}

impl rand_core::CryptoRng for TranscriptRng {}

#[cfg(test)]
mod tests {
    use strobe_rs::SecParam;
    use strobe_rs::Strobe;

    use super::*;

    /// Test against a full strobe implementation to ensure we match the few
    /// operations we're interested in.
    struct TestTranscript {
        state: Strobe,
    }

    impl TestTranscript {
        /// Strobe init; meta-AD(label)
        pub fn new(label: &[u8]) -> TestTranscript {
            use crate::constants::MERLIN_PROTOCOL_LABEL;

            let mut tt = TestTranscript {
                state: Strobe::new(MERLIN_PROTOCOL_LABEL, SecParam::B128),
            };
            tt.append_message(b"dom-sep", label);

            tt
        }

        /// Strobe op: meta-AD(label || len(message)); AD(message)
        pub fn append_message(&mut self, label: &[u8], message: &[u8]) {
            // metadata = label || len(message);
            let mut metadata: Vec<u8> = Vec::with_capacity(label.len() + 4);
            metadata.extend_from_slice(label);
            metadata.extend_from_slice(&encode_usize_as_u32(message.len()));

            self.state.meta_ad(&metadata, false);
            self.state.ad(&message, false);
        }

        /// Strobe op: meta-AD(label || len(dest)); PRF into challenge_bytes
        pub fn challenge_bytes(&mut self, label: &[u8], dest: &mut [u8]) {
            let prf_len = dest.len();

            // metadata = label || len(challenge_bytes);
            let mut metadata: Vec<u8> = Vec::with_capacity(label.len() + 4);
            metadata.extend_from_slice(label);
            metadata.extend_from_slice(&encode_usize_as_u32(prf_len));

            self.state.meta_ad(&metadata, false);
            self.state.prf(dest, false);
        }
    }

    /// Test a simple protocol with one message and one challenge
    #[test]
    fn equivalence_simple() {
        let mut real_transcript = Transcript::new(b"test protocol");
        let mut test_transcript = TestTranscript::new(b"test protocol");

        real_transcript.append_message(b"some label", b"some data");
        test_transcript.append_message(b"some label", b"some data");

        let mut real_challenge = [0u8; 32];
        let mut test_challenge = [0u8; 32];

        real_transcript.challenge_bytes(b"challenge", &mut real_challenge);
        test_transcript.challenge_bytes(b"challenge", &mut test_challenge);

        assert_eq!(real_challenge, test_challenge);
    }

    /// Test a complex protocol with multiple messages and challenges,
    /// with messages long enough to wrap around the sponge state, and
    /// with multiple rounds of messages and challenges.
    #[test]
    fn equivalence_complex() {
        let mut real_transcript = Transcript::new(b"test protocol");
        let mut test_transcript = TestTranscript::new(b"test protocol");

        let data = vec![99; 1024];

        real_transcript.append_message(b"step1", b"some data");
        test_transcript.append_message(b"step1", b"some data");

        let mut real_challenge = [0u8; 32];
        let mut test_challenge = [0u8; 32];

        for _ in 0..32 {
            real_transcript.challenge_bytes(b"challenge", &mut real_challenge);
            test_transcript.challenge_bytes(b"challenge", &mut test_challenge);

            assert_eq!(real_challenge, test_challenge);

            real_transcript.append_message(b"bigdata", &data);
            test_transcript.append_message(b"bigdata", &data);

            real_transcript.append_message(b"challengedata", &real_challenge);
            test_transcript.append_message(b"challengedata", &test_challenge);
        }
    }

    #[test]
    fn transcript_rng_is_bound_to_transcript_and_witnesses() {
        use curve25519_dalek::scalar::Scalar;
        use rand_chacha::ChaChaRng;
        use rand_core::SeedableRng;

        // Check that the TranscriptRng is bound to the transcript and
        // the witnesses.  This is done by producing a sequence of
        // transcripts that diverge at different points and checking
        // that they produce different challenges.

        let protocol_label = b"test TranscriptRng collisions";
        let commitment1 = b"commitment data 1";
        let commitment2 = b"commitment data 2";
        let witness1 = b"witness data 1";
        let witness2 = b"witness data 2";

        let mut t1 = Transcript::new(protocol_label);
        let mut t2 = Transcript::new(protocol_label);
        let mut t3 = Transcript::new(protocol_label);
        let mut t4 = Transcript::new(protocol_label);

        t1.append_message(b"com", commitment1);
        t2.append_message(b"com", commitment2);
        t3.append_message(b"com", commitment2);
        t4.append_message(b"com", commitment2);

        let mut r1 = t1
            .build_rng()
            .rekey_with_witness_bytes(b"witness", witness1)
            .finalize(&mut ChaChaRng::from_seed([0; 32]));

        let mut r2 = t2
            .build_rng()
            .rekey_with_witness_bytes(b"witness", witness1)
            .finalize(&mut ChaChaRng::from_seed([0; 32]));

        let mut r3 = t3
            .build_rng()
            .rekey_with_witness_bytes(b"witness", witness2)
            .finalize(&mut ChaChaRng::from_seed([0; 32]));

        let mut r4 = t4
            .build_rng()
            .rekey_with_witness_bytes(b"witness", witness2)
            .finalize(&mut ChaChaRng::from_seed([0; 32]));

        let s1 = Scalar::random(&mut r1);
        let s2 = Scalar::random(&mut r2);
        let s3 = Scalar::random(&mut r3);
        let s4 = Scalar::random(&mut r4);

        // Transcript t1 has different commitments than t2, t3, t4, so
        // it should produce distinct challenges from all of them.
        assert_ne!(s1, s2);
        assert_ne!(s1, s3);
        assert_ne!(s1, s4);

        // Transcript t2 has different witness variables from t3, t4,
        // so it should produce distinct challenges from all of them.
        assert_ne!(s2, s3);
        assert_ne!(s2, s4);

        // Transcripts t3 and t4 have the same commitments and
        // witnesses, so they should give different challenges only
        // based on the RNG. Checking that they're equal in the
        // presence of a bad RNG checks that the different challenges
        // above aren't because the RNG is accidentally different.
        assert_eq!(s3, s4);
    }
}
