//! This module provides the implementation of the STAR (distributed
//! Secret-sharing for Threshold AggRegation of data) protocol. The STAR
//! protocol provides the ability for clients to report secret
//! measurements to servers, whilst maintaining k-anonymity-like
//! guarantees.
//!
//! In essence, such measurements are only revealed if a `threshold`
//! number of clients all send the same message. Clients are permitted
//! to also send relevant, arbitrary associated data that can also be
//! revealed.
//!
//! In STAR, clients derive randomness from a separate server that
//! implements a puncturable partially oblivious pseudorandom function
//! (PPOPRF) protocol. In STARLite, clients derive randomness used for
//! hiding their measurements locally from the measurement itself. The
//! PPOPRF protocol takes in the client measurement, a server secret
//! key, and the current epoch metadata tag as input, and outputs a
//! random (deterministic) value.
//!
//! In the case of STARLite, the design is simpler than in STAR, but
//! security is ONLY maintained in the case where client measurements
//! are sampled from a high-entropy domain. In the case of STAR, client
//! security guarantees hold even for low-entropy inputs, as long as the
//! randomness is only revealed after the epoch metadata tag has been
//! punctured from the randomness server's secret key.
//!
//! See the [full paper](https://arxiv.org/abs/2109.10074) for more
//! details.
//!
//! # Example (client)
//!
//! The following example shows how to generate a message triple of `(ciphertext,
//! share, tag)`. This message can then be sent to the aggregation server.
//!
//! ```
//! # use sta_rs::*;
//! # let threshold = 2;
//! # let epoch = "t";
//! let measurement = SingleMeasurement::new("hello world".as_bytes());
//! let mg = MessageGenerator::new(measurement, threshold, epoch.as_bytes());
//! let mut rnd = [0u8; 32];
//! // NOTE: this is for STARLite. Randomness must be sampled from a
//! // randomness server in order to implement the full STAR protocol.
//! mg.sample_local_randomness(&mut rnd);
//!
//! let Message {
//!   ciphertext,
//!   share,
//!   tag,
//! } = Message::generate(&mg, &mut rnd, None)
//!     .expect("Could not generate message triplet");
//! ```
//! # Example (WASM client)
//!
//! The following example shows how to generate a triple of `(key,
//! share, tag)` for each client in the STARLite protocol, which is used
//! in the existing WASM integration. The STAR protocol is not yet
//! supported.
//!
//! In the WASM integration the `key` MUST then be used to encrypt the
//! measurement and associated data into a `ciphertext` in the
//! higher-level application. The message triple `(ciphertext, share,
//! tag)` is then sent to the server.
//!
//! ```
//! # use sta_rs::*;
//! # let threshold = 2;
//! # let epoch = "t";
//! let measurement = SingleMeasurement::new("hello world".as_bytes());
//! let mg = MessageGenerator::new(measurement, threshold, epoch.as_bytes());
//! let mut rnd = [0u8; 32];
//! // NOTE: this is for STARLite. Randomness must be sampled from a
//! // randomness server in order to implement the full STAR protocol.
//! mg.sample_local_randomness(&mut rnd);
//! let WASMSharingMaterial {
//!   key,
//!   share,
//!   tag,
//! } = mg.share_with_local_randomness().unwrap();
//! ```
//!
//! # Example (server)
//!
//! Once over `threshold` shares are recovered from clients, it is
//! possible to recover the randomness encoded in each of the shares
//!
//! ```
//! # use sta_rs::*;
//! # use star_test_utils::*;
//! # let mut messages = Vec::new();
//! # let threshold = 2;
//! # let epoch = "t";
//! # let measurement = SingleMeasurement::new("hello world".as_bytes());
//!
//! # let mg = MessageGenerator::new(measurement, threshold, epoch.as_bytes());
//! # for i in 0..3 {
//! #     let mut rnd = [0u8; 32];
//! #     mg.sample_local_randomness(&mut rnd);
//! #     messages.push(Message::generate(&mg, &mut rnd, None).unwrap());
//! # }
//! # let shares: Vec<Share> = messages.iter().map(|triple| triple.share.clone()).collect();
//! let value = share_recover(&shares).unwrap().get_message();
//!
//! // derive key for decrypting payload data in client message
//! let mut enc_key = vec![0u8; 16];
//! derive_ske_key(&value, epoch.as_bytes(), &mut enc_key);
//! ```
use std::error::Error;
use std::str;

use rand::Rng;
mod strobe_rng;
use strobe_rng::StrobeRng;
use strobe_rs::{SecParam, Strobe};
use zeroize::{Zeroize, ZeroizeOnDrop};

use adss::{recover, Commune};
pub use {adss::load_bytes, adss::store_bytes, adss::Share as InternalShare};

#[cfg(feature = "star2")]
use ppoprf::ppoprf::{end_to_end_evaluation, Server as PPOPRFServer};

pub const AES_BLOCK_LEN: usize = 24;
pub const DIGEST_LEN: usize = 32;

// A `Measurement` provides the wrapper for a client-generated value in
// the STAR protocol that is later aggregated and processed at the
// server-side. Measurements are only revealed on the server-side if the
// `threshold` is met, in terms of clients that send the same
// `Measurement` value.
#[derive(Clone, Debug, PartialEq, Eq, Zeroize, ZeroizeOnDrop)]
pub struct SingleMeasurement(Vec<u8>);
impl SingleMeasurement {
  pub fn new(x: &[u8]) -> Self {
    Self(x.to_vec())
  }

  pub fn as_slice(&self) -> &[u8] {
    self.0.as_slice()
  }

  pub fn as_vec(&self) -> Vec<u8> {
    self.0.clone()
  }

  pub fn byte_len(&self) -> usize {
    self.0.len()
  }

  pub fn is_empty(&self) -> bool {
    self.0.is_empty()
  }
}

impl From<&str> for SingleMeasurement {
  fn from(s: &str) -> Self {
    SingleMeasurement::new(s.as_bytes())
  }
}

// The `AssociatedData` struct wraps the arbitrary data that a client
// can encode in its message to the `Server`. Such data is also only
// revealed in the case that the `threshold` is met.
#[derive(Debug)]
pub struct AssociatedData(Vec<u8>);
impl AssociatedData {
  pub fn new(buf: &[u8]) -> Self {
    Self(buf.to_vec())
  }

  pub fn as_slice(&self) -> &[u8] {
    self.0.as_slice()
  }

  pub fn as_vec(&self) -> Vec<u8> {
    self.0.clone()
  }
}
impl From<&str> for AssociatedData {
  fn from(s: &str) -> Self {
    AssociatedData::from(s.as_bytes())
  }
}
impl From<&[u8]> for AssociatedData {
  fn from(buf: &[u8]) -> Self {
    AssociatedData::new(buf)
  }
}

// Wrapper type for `adss::Share` to implement `ZeroizeOnDrop`properly.
#[derive(Clone, Debug, PartialEq, Eq, Zeroize)]
pub struct Share(InternalShare);
impl Share {
  pub fn to_bytes(&self) -> Vec<u8> {
    self.0.to_bytes()
  }

  pub fn from_bytes(bytes: &[u8]) -> Option<Self> {
    Some(Self(InternalShare::from_bytes(bytes)?))
  }
}
impl Drop for Share {
  fn drop(&mut self) {
    self.0.zeroize();
  }
}

// The `Ciphertext` struct holds the symmetrically encrypted data that
// corresponds to the concatenation of `Measurement` and any optional
// `AssociatedData`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Ciphertext {
  bytes: Vec<u8>,
}
impl Ciphertext {
  pub fn new(enc_key_buf: &[u8], data: &[u8], label: &str) -> Self {
    let mut s = Strobe::new(label.as_bytes(), SecParam::B128);
    s.key(enc_key_buf, false);
    let mut x = vec![0u8; data.len()];
    x.copy_from_slice(data);
    s.send_enc(&mut x, false);

    Self { bytes: x.to_vec() }
  }

  pub fn decrypt(&self, enc_key_buf: &[u8], label: &str) -> Vec<u8> {
    let mut s = Strobe::new(label.as_bytes(), SecParam::B128);
    s.key(enc_key_buf, false);
    let mut m = vec![0u8; self.bytes.len()];
    m.copy_from_slice(&self.bytes);
    s.recv_enc(&mut m, false);
    m
  }

  pub fn to_bytes(&self) -> Vec<u8> {
    self.bytes.clone()
  }

  pub fn from_bytes(bytes: &[u8]) -> Ciphertext {
    Self {
      bytes: bytes.to_vec(),
    }
  }
}
impl From<Vec<u8>> for Ciphertext {
  fn from(bytes: Vec<u8>) -> Self {
    Self { bytes }
  }
}

// A `Message` is the message that a client sends to the server during
// the STAR protocol. Consisting of a `Ciphertext`, a `Share`, and a
// `tag`. The `Ciphertext`can only be decrypted if a `threshold` number
// of clients possess the same measurement.
//
// This struct should only be used by applications that do not perform
// encryption at the higher application-levels.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Message {
  pub ciphertext: Ciphertext,
  pub share: Share,
  pub tag: Vec<u8>,
}
impl Message {
  fn new(c: Ciphertext, share: Share, tag: &[u8]) -> Self {
    Self {
      ciphertext: c,
      share,
      tag: tag.to_vec(),
    }
  }

  // Generates a message that is used in the aggregation phase
  pub fn generate(
    mg: &MessageGenerator,
    rnd: &[u8; 32],
    aux: Option<AssociatedData>,
  ) -> Result<Self, Box<dyn Error>> {
    let r = mg.derive_random_values(rnd);

    // key is then used for encrypting measurement and associated
    // data
    let key = mg.derive_key(&r[0]);
    let share = mg.share(&r[0], &r[1])?;
    let tag = r[2];

    let mut data: Vec<u8> = Vec::new();
    store_bytes(mg.x.as_slice(), &mut data);
    if let Some(ad) = aux {
      store_bytes(ad.as_slice(), &mut data);
    }
    let ciphertext = Ciphertext::new(&key, &data, "star_encrypt");

    Ok(Message::new(ciphertext, share, &tag))
  }

  pub fn to_bytes(&self) -> Vec<u8> {
    let mut out: Vec<u8> = Vec::new();

    // ciphertext: Ciphertext
    store_bytes(&self.ciphertext.to_bytes(), &mut out);

    // share: Share
    store_bytes(&self.share.to_bytes(), &mut out);

    // tag: Vec<u8>
    store_bytes(&self.tag, &mut out);

    out
  }

  pub fn from_bytes(bytes: &[u8]) -> Option<Message> {
    let mut slice = bytes;

    // ciphertext: Ciphertext
    let cb = load_bytes(slice)?;
    let ciphertext = Ciphertext::from_bytes(cb);
    slice = &slice[4 + cb.len()..];

    // share: Share
    let sb = load_bytes(slice)?;
    let share = Share::from_bytes(sb)?;
    slice = &slice[4 + sb.len()..];

    // tag: Vec<u8>
    let tag = load_bytes(slice)?;

    Some(Message {
      ciphertext,
      share,
      tag: tag.to_vec(),
    })
  }
}

// The `WASMSharingMaterial` consists of all data that is passed to
// higher-level applications using the star-wasm API. This allows
// encrypting and sending the client measurements in higher-level
// implementations of the STAR protocol.
#[derive(Zeroize)]
pub struct WASMSharingMaterial {
  /// 16-byte AES encryption key
  pub key: [u8; 16],
  /// Secret share of key derivation randomness
  pub share: Share,
  /// 32-byte random tag associated with client measurement
  pub tag: [u8; 32],
}

// In the STAR protocol, the `MessageGenerator` is the entity which
// samples and sends `Measurement` to the `AggregationServer`. The
// measurements will only be revealed if a `threshold` number of
// MessageGenerators send the same encoded `Measurement` value.
//
// Note that the `MessageGenerator` struct holds all of the public
// protocol parameters, the secret `Measurement` and `AssociatedData`
// objects, and where randomness should be sampled from.
//
// In the STARLite protocol, the `MessageGenerator` samples randomness
// locally: derived straight from the `Measurement` itself. In the STAR
// protocol, the `MessageGenerator` derives its randomness from an
// exchange with a specifically-defined server that runs a POPRF.
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct MessageGenerator {
  pub x: SingleMeasurement,
  threshold: u32,
  epoch: Vec<u8>,
}
impl MessageGenerator {
  pub fn new(x: SingleMeasurement, threshold: u32, epoch: &[u8]) -> Self {
    Self {
      x,
      threshold,
      epoch: epoch.into(),
    }
  }

  // Share with OPRF randomness (STARLite)
  pub fn share_with_local_randomness(
    &self,
  ) -> Result<WASMSharingMaterial, Box<dyn Error>> {
    let mut rnd = vec![0u8; 32];
    self.sample_local_randomness(&mut rnd);
    let r = self.derive_random_values(&rnd);

    // key is then used for encrypting measurement and associated
    // data
    let key = self.derive_key(&r[0]);
    let share = self.share(&r[0], &r[1])?;
    let tag = r[2];
    Ok(WASMSharingMaterial { key, share, tag })
  }

  #[cfg(feature = "star2")]
  // Share with OPRF randomness (STAR)
  pub fn share_with_oprf_randomness(
    &self,
    oprf_server: &PPOPRFServer,
  ) -> WASMSharingMaterial {
    let mut rnd = vec![0u8; 32];
    self.sample_oprf_randomness(oprf_server, &mut rnd);
    let r = self.derive_random_values(&rnd);

    // key is then used for encrypting measurement and associated
    // data
    let key = self.derive_key(&r[0]);
    let share = self.share(&r[0], &r[1]);
    let tag = r[2].clone();
    WASMSharingMaterial { key, share, tag }
  }

  fn derive_random_values(&self, randomness: &[u8]) -> Vec<[u8; 32]> {
    let mut output = Vec::new();
    for i in 0..3 {
      let mut to_fill = [0u8; 32];
      strobe_digest(
        randomness,
        &[&[i as u8]],
        "star_derive_randoms",
        &mut to_fill,
      );
      output.push(to_fill);
    }
    output
  }

  fn derive_key(&self, r1: &[u8]) -> [u8; 16] {
    let mut enc_key = [0u8; 16];
    derive_ske_key(r1, &self.epoch, &mut enc_key);
    enc_key
  }

  fn share(&self, r1: &[u8], r2: &[u8]) -> Result<Share, Box<dyn Error>> {
    let c = Commune::new(self.threshold, r1.to_vec(), r2.to_vec(), None);
    Ok(Share(c.share()?))
  }

  pub fn sample_local_randomness(&self, out: &mut [u8]) {
    if out.len() != DIGEST_LEN {
      panic!(
        "Output buffer length ({}) does not match randomness length ({})",
        out.len(),
        DIGEST_LEN
      );
    }
    strobe_digest(
      self.x.as_slice(),
      &[&self.epoch, &self.threshold.to_le_bytes()],
      "star_sample_local",
      out,
    );
  }

  #[cfg(feature = "star2")]
  pub fn sample_oprf_randomness(
    &self,
    oprf_server: &PPOPRFServer,
    out: &mut [u8],
  ) {
    let mds = oprf_server.get_valid_metadata_tags();
    let index = mds.iter().position(|r| r == &self.epoch).unwrap();
    end_to_end_evaluation(oprf_server, self.x.as_slice(), index, true, out);
  }
}

// FIXME can we implement collect trait?
pub fn share_recover(shares: &[Share]) -> Result<Commune, Box<dyn Error>> {
  recover(
    &shares
      .iter()
      .map(|share| share.0.clone())
      .collect::<Vec<InternalShare>>(),
  )
}

// The `derive_ske_key` helper function derives symmetric encryption
// keys that are used for encrypting/decrypting `Ciphertext` objects
// during the STAR protocol.
pub fn derive_ske_key(r1: &[u8], epoch: &[u8], key_out: &mut [u8]) {
  let mut to_fill = vec![0u8; 32];
  strobe_digest(r1, &[epoch], "star_derive_ske_key", &mut to_fill);
  key_out.copy_from_slice(&to_fill[..16]);
}

pub fn strobe_digest(key: &[u8], ad: &[&[u8]], label: &str, out: &mut [u8]) {
  if out.len() != DIGEST_LEN {
    panic!(
      "Output buffer length ({}) does not match intended output length ({})",
      out.len(),
      DIGEST_LEN
    );
  } else if ad.is_empty() {
    panic!("No additional data provided");
  }
  let mut t = Strobe::new(label.as_bytes(), SecParam::B128);
  t.key(key, false);
  for x in ad.iter() {
    t.ad(x, false);
  }
  let mut rng: StrobeRng = t.into();
  rng.fill(out);
}
