use std::collections::hash_map::Entry;
use std::collections::HashMap;
use std::fmt;

use rand::distributions::Distribution;
use rayon::prelude::*;

use zipf::ZipfDistribution;

#[cfg(feature = "star2")]
pub use ppoprf::ppoprf::Server as PPOPRFServer;

#[cfg(not(feature = "star2"))]
pub struct PPOPRFServer;

use sta_rs::*;

// The `zipf_measurement` function returns a client `Measurement` sampled from
// Zipf power-law distribution with `n` corresponding to the number
// of potential elements, and `s` the exponent.
pub fn measurement_zipf(n: usize, s: f64) -> SingleMeasurement {
  let mut rng = rand::thread_rng();
  let zipf = ZipfDistribution::new(n, s).unwrap();
  let sample = zipf.sample(&mut rng).to_le_bytes();
  let extended = sample.to_vec();
  // essentially we compute a hash here so that we can simulate
  // having a full 32 bytes of data
  let mut to_fill = vec![0u8; 32];
  strobe_digest(&[0u8; 32], &[&extended], "star_zipf_sample", &mut to_fill);
  SingleMeasurement::new(&to_fill)
}

pub fn client_zipf(
  n: usize,
  s: f64,
  threshold: u32,
  epoch: &str,
) -> MessageGenerator {
  let x = measurement_zipf(n, s);
  MessageGenerator::new(x, threshold, epoch)
}

// An `Output` corresponds to a single client `Measurement` sent to the
// `AggregationServer` that satisfied the `threshold` check. Such
// structs contain the `Measurement` value itself, along with a vector
// of all the optional `AssociatedData` values sent by clients.
pub struct Output {
  pub x: SingleMeasurement,
  pub aux: Vec<Option<AssociatedData>>,
}
impl fmt::Debug for Output {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    f.debug_struct("Output")
      .field("tag", &self.x)
      .field("aux", &self.aux)
      .finish()
  }
}

#[derive(Debug)]
enum AggServerError {
  PossibleShareCollision,
}

// The `AggregationServer` is the entity that processes `Client`
// messages and learns `Measurement` values and `AssociatedData` if the
// `threshold` is met. These servers possess no secret data.
pub struct AggregationServer {
  pub threshold: u32,
  pub epoch: String,
}
impl AggregationServer {
  pub fn new(threshold: u32, epoch: &str) -> Self {
    AggregationServer {
      threshold,
      epoch: epoch.to_string(),
    }
  }

  pub fn retrieve_outputs(&self, all_messages: &[Message]) -> Vec<Output> {
    let filtered = self.filter_messages(all_messages);
    filtered
      .into_par_iter()
      .map(|messages| self.recover_measurements(&messages))
      .map(|output| output.unwrap())
      .collect()
  }

  fn recover_measurements(
    &self,
    messages: &[Message],
  ) -> Result<Output, AggServerError> {
    let mut enc_key_buf = vec![0u8; 16];
    self.key_recover(messages, &mut enc_key_buf)?;

    let ciphertexts = messages.iter().map(|t| t.ciphertext.clone());
    let plaintexts =
      ciphertexts.map(|c| c.decrypt(&enc_key_buf, "star_encrypt"));

    let splits: Vec<(Vec<u8>, Option<AssociatedData>)> = plaintexts
      .map(|p| {
        let mut slice = &p[..];

        let measurement_bytes = load_bytes(slice).unwrap();
        slice = &slice[4 + measurement_bytes.len()..];
        if !slice.is_empty() {
          let aux_bytes = load_bytes(slice).unwrap();
          if !aux_bytes.is_empty() {
            return (
              measurement_bytes.to_vec(),
              Some(AssociatedData::new(aux_bytes)),
            );
          }
        }
        (measurement_bytes.to_vec(), None)
      })
      .collect();
    let tag = &splits[0].0;
    for new_tag in splits.iter().skip(1) {
      if &new_tag.0 != tag {
        panic!("tag mismatch ({:?} != {:?})", tag, new_tag.0);
      }
    }
    Ok(Output {
      x: SingleMeasurement::new(tag),
      aux: splits.into_iter().map(|val| val.1).collect(),
    })
  }

  fn key_recover(
    &self,
    messages: &[Message],
    enc_key: &mut [u8],
  ) -> Result<(), AggServerError> {
    let shares: Vec<Share> =
      messages.iter().map(|triple| triple.share.clone()).collect();
    let res = share_recover(&shares);
    if res.is_err() {
      return Err(AggServerError::PossibleShareCollision);
    }
    let message = res.unwrap().get_message();
    derive_ske_key(&message, self.epoch.as_bytes(), enc_key);
    Ok(())
  }

  fn filter_messages(&self, messages: &[Message]) -> Vec<Vec<Message>> {
    let collected = self.collect_messages(messages);
    collected
      .into_iter()
      .filter(|bucket| bucket.len() >= (self.threshold as usize))
      .collect()
  }

  fn collect_messages(&self, messages: &[Message]) -> Vec<Vec<Message>> {
    let mut collected_messages: HashMap<String, Vec<Message>> = HashMap::new();
    for triple in messages {
      let s = format!("{:x?}", triple.tag);
      match collected_messages.entry(s) {
        Entry::Vacant(e) => {
          e.insert(vec![triple.clone()]);
        }
        Entry::Occupied(mut e) => {
          e.get_mut().push(triple.clone());
        }
      }
    }
    collected_messages.values().cloned().collect()
  }
}
