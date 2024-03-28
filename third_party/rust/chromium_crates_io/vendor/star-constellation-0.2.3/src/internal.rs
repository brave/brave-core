use std::collections::hash_map::Entry;
use std::collections::HashMap;

use serde::{Deserialize, Serialize};
use sta_rs::{
  derive_ske_key, load_bytes, share_recover, AssociatedData, Ciphertext,
  Message, MessageGenerator, Share, SingleMeasurement,
};

use rand::{rngs::OsRng, RngCore};

use crate::consts::*;
use crate::Error;

// Internal consts
const LAYER_ENC_KEY_LEN: usize = 16;
const NESTED_STAR_ENCRYPTION_LABEL: &str = "constellation_layer_encrypt";

/// The `NestedMeasurement` struct provides a mechanism for submitting
/// measurements as vectors.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct NestedMeasurement(pub Vec<SingleMeasurement>);
impl NestedMeasurement {
  pub fn new(x_list: &[Vec<u8>]) -> Result<Self, Error> {
    let mut measurements = Vec::with_capacity(x_list.len());
    // create partial measurements
    for x in x_list {
      measurements.push(SingleMeasurement::new(x));
    }
    Ok(Self(measurements))
  }

  pub fn get_message_generators(
    &self,
    threshold: u32,
    epoch: u8,
  ) -> Vec<MessageGenerator> {
    self
      .0
      .iter()
      .map(|x| MessageGenerator::new(x.clone(), threshold, &[epoch]))
      .collect()
  }

  pub fn get_layer_as_bytes(&self, i: usize) -> Vec<u8> {
    self.0[i].as_vec()
  }

  pub fn len(&self) -> usize {
    self.0.len()
  }
}

/// An internal struct that allows serializing and deserializing client
/// STAR messages during encryption/decryption.
#[derive(Serialize, Deserialize, Clone, Debug)]
struct SerializableMessage {
  ciphertext: Vec<u8>,
  share: Vec<u8>,
  tag: Vec<u8>,
}

impl TryFrom<SerializableMessage> for Message {
  type Error = Error;

  fn try_from(sm: SerializableMessage) -> Result<Message, Error> {
    Ok(Message {
      ciphertext: Ciphertext::from(sm.ciphertext),
      share: Share::from_bytes(&sm.share).ok_or_else(|| {
        Error::Serialization("invalid serialized share".to_string())
      })?,
      tag: sm.tag,
    })
  }
}

impl From<Message> for SerializableMessage {
  fn from(message: Message) -> Self {
    Self {
      ciphertext: message.ciphertext.to_bytes(),
      share: message.share.to_bytes(),
      tag: message.tag,
    }
  }
}

/// A `NestedMessage` consists of an unencrypted STAR message, plus
/// layers of encrypted STAR messages that can be decrypted only if the
/// previous message layer is decrypted and opened via the standard STAR
/// recovery mechanism.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct NestedMessage {
  pub epoch: u8,
  pub unencrypted_layer: Message,
  pub encrypted_layers: Vec<Ciphertext>,
}
impl NestedMessage {
  pub fn new(
    gens: &[MessageGenerator],
    rand_bytes: &[[u8; RANDOMNESS_LEN]],
    keys: &[Vec<u8>],
    aux_data: &[u8],
    epoch: u8,
  ) -> Result<Self, Error> {
    if gens.len() - 1 != keys.len() {
      return Err(Error::LayerEncryptionKeys(keys.len(), gens.len() - 1));
    } else if rand_bytes.len() != gens.len() {
      return Err(Error::NumMeasurementLayers(gens.len(), rand_bytes.len()));
    }

    let mut unencrypted_layer: Option<Message> = None;
    let mut encrypted_layers: Vec<Ciphertext> = Vec::with_capacity(keys.len());

    // construct nested star message
    for i in 0..gens.len() {
      let mg = &gens[i];
      let rnd = &rand_bytes[i];

      // set ith encryption key as auxiliary data in layer i
      // (except for last layer)
      let mut key_aux = None;
      if i < gens.len() - 1 {
        key_aux = Some(keys[i].clone());
      }

      // add any extra associated data that has been specified
      let nested_aux = NestedAssociatedData {
        key: key_aux,
        data: aux_data.to_vec(),
      };
      let message_aux = Some(AssociatedData::new(
        &bincode::serialize(&nested_aux).unwrap(),
      ));

      // generate message
      let message = Message::generate(mg, rnd, message_aux)
        .map_err(|msg| Error::MessageGeneration(msg.to_string()))?;

      // encrypt ith layer with (i-1)th key (except for first
      // layer)
      if i > 0 {
        // serialize message
        let bytes_to_encrypt =
          bincode::serialize(&SerializableMessage::from(message)).unwrap();
        let encrypted_layer = Ciphertext::new(
          &keys[i - 1],
          &bytes_to_encrypt,
          NESTED_STAR_ENCRYPTION_LABEL,
        );
        encrypted_layers.push(encrypted_layer);
      } else {
        unencrypted_layer = Some(message);
      }
    }
    Ok(Self {
      epoch,
      unencrypted_layer: unencrypted_layer.unwrap(),
      encrypted_layers,
    })
  }

  /// The `decrypt_next_layer` function decrypts the next layer of
  /// encrypted messages and sets the unencrypted layer to be equal to
  /// the decrypted message
  pub fn decrypt_next_layer(&mut self, key: &[u8]) -> Result<(), Error> {
    if self.encrypted_layers.is_empty() {
      panic!("No more layers to decrypt");
    }
    let decrypted =
      self.encrypted_layers[0].decrypt(key, NESTED_STAR_ENCRYPTION_LABEL);
    let sm: SerializableMessage = bincode::deserialize(&decrypted)
      .map_err(|e| Error::Serialization(e.to_string()))?;
    self.unencrypted_layer = sm.try_into()?;
    self.encrypted_layers = self.encrypted_layers[1..].to_vec();
    Ok(())
  }
}

/// Serialization wrapper for the `NestedMessage` struct
#[derive(Serialize, Deserialize)]
pub struct SerializableNestedMessage {
  epoch: u8,
  unencrypted_layer: SerializableMessage,
  encrypted_layers: Vec<Vec<u8>>,
}

impl TryFrom<SerializableNestedMessage> for NestedMessage {
  type Error = Error;

  fn try_from(sm: SerializableNestedMessage) -> Result<NestedMessage, Error> {
    Ok(NestedMessage {
      epoch: sm.epoch,
      unencrypted_layer: sm.unencrypted_layer.try_into()?,
      encrypted_layers: sm
        .encrypted_layers
        .into_iter()
        .map(Ciphertext::from)
        .collect(),
    })
  }
}

impl TryFrom<&SerializableNestedMessage> for NestedMessage {
  type Error = Error;

  fn try_from(sm: &SerializableNestedMessage) -> Result<NestedMessage, Error> {
    Ok(NestedMessage {
      epoch: sm.epoch,
      unencrypted_layer: sm.unencrypted_layer.clone().try_into()?,
      encrypted_layers: sm
        .encrypted_layers
        .iter()
        .map(|c| Ciphertext::from(c.to_vec()))
        .collect(),
    })
  }
}

impl From<NestedMessage> for SerializableNestedMessage {
  fn from(nm: NestedMessage) -> Self {
    Self {
      epoch: nm.epoch,
      unencrypted_layer: nm.unencrypted_layer.into(),
      encrypted_layers: nm
        .encrypted_layers
        .iter()
        .map(|c| c.to_bytes())
        .collect(),
    }
  }
}

/// The `NestedAssociatedData` struct is a serializable struct that
/// accompanies a layer of a `NestedMessage`. The `key` field should
/// contain a symmetric encryption key that decrypts the message at the
/// next layer in the `NestedMessage`. It is optional to allow
/// specifying `None`for the final message layer. The `data` field
/// contains arbitrary encoded data that accompanies the associated
/// measurement, it is empty if none is specified.
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct NestedAssociatedData {
  key: Option<Vec<u8>>,
  data: Vec<u8>,
}

/// Internal struct that contains the partial measurement at a given
/// layer, plus the encryption key for decrypting the next layer.
#[derive(Clone, Debug)]
pub struct PartialMeasurement {
  pub measurement: NestedMeasurement,
  pub aux: NestedAssociatedData,
}
impl PartialMeasurement {
  pub fn get_next_layer_key(&self) -> &Option<Vec<u8>> {
    &self.aux.key
  }
}

/// `FinalMeasurement` contains the measurement data output from a
/// single message, including any additional data included at each
/// layer. Important: The auxiliary data that is included is only taken
/// from the last layer that was opened.
#[derive(Clone, Debug)]
pub struct FinalMeasurement {
  measurement: NestedMeasurement,
  data: Vec<u8>,
}
impl FinalMeasurement {
  pub fn get_measurement_raw(&self) -> Vec<u8> {
    self.get_partial_measurement_raw(self.measurement.len() - 1)
  }

  pub fn get_aux_data(&self) -> &[u8] {
    &self.data
  }

  fn get_partial_measurement_raw(&self, i: usize) -> Vec<u8> {
    self.measurement.get_layer_as_bytes(i)
  }
}
impl From<&PartialMeasurement> for FinalMeasurement {
  fn from(pm: &PartialMeasurement) -> Self {
    let pm_to_set = pm.measurement.clone();
    Self {
      measurement: pm_to_set,
      data: pm.aux.data.clone(),
    }
  }
}
impl PartialEq for FinalMeasurement {
  fn eq(&self, other: &Self) -> bool {
    self.get_measurement_raw() == other.get_measurement_raw()
  }
}

#[derive(Clone, Debug)]
pub struct PartialRecoveredMessage {
  pub measurement: Option<FinalMeasurement>,
  pub next_message: Option<NestedMessage>,
}
impl PartialRecoveredMessage {
  pub fn get_measurement_raw(&self) -> Vec<u8> {
    match self.measurement.as_ref() {
      Some(m) => m.get_measurement_raw(),
      None => Vec::new(),
    }
  }

  pub fn get_aux_data(&self) -> Vec<u8> {
    match self.measurement.as_ref() {
      Some(m) => m.get_aux_data().to_vec(),
      None => Vec::new(),
    }
  }

  pub fn is_empty(&self) -> bool {
    self.measurement.is_none() && self.next_message.is_none()
  }
}

/// Internal struct that adds a long-lived identifier to a
/// `NestedMessage` instantiation, for allowing easy traversal of the
/// nesting tree.
#[derive(Clone, Debug)]
struct IdentNestedMessage {
  message: NestedMessage,
  ident: usize,
}
impl IdentNestedMessage {
  fn get_next_layer(&self) -> IdentMessage {
    IdentMessage {
      message: self.message.unencrypted_layer.clone(),
      ident: self.ident,
    }
  }

  fn get_next_layer_message(&self) -> &Message {
    &self.message.unencrypted_layer
  }

  fn decrypt_next_layer(&mut self, key: &[u8]) -> Result<(), Error> {
    self.message.decrypt_next_layer(key)
  }
}

/// Internal struct that wraps the unencrypted layer of an
/// `IdentNestedMessage` instantiation.
#[derive(Clone, Debug)]
struct IdentMessage {
  message: Message,
  ident: usize,
}
impl IdentMessage {
  fn get_tag(&self) -> &[u8] {
    &self.message.tag
  }
}

/// `recover_partial_measurement` returns all partial output
/// measurements according to the list of messages that it is provided.
pub fn recover_partial_measurements(
  nested_messages: &[NestedMessage],
  epoch: u8,
  threshold: u32,
  num_layers: usize,
) -> Vec<Result<PartialRecoveredMessage, Error>> {
  // Identify each message with a long-term identifier
  let mut ident_nested_messages = Vec::new();
  for (i, nm) in nested_messages.iter().enumerate() {
    ident_nested_messages.push(Some(IdentNestedMessage {
      message: nm.clone(),
      ident: i,
    }));
  }

  // The `current layer` holds all subsets of messages associated with
  // the current layer of nodes in the navigated tree
  let mut current_layer: Vec<Vec<IdentMessage>> = vec![ident_nested_messages
    .iter()
    .map(|m| m.as_ref().unwrap().get_next_layer())
    .collect()];

  // The `measurements` variable will eventually hold the most fine-grained
  // partial measurement sent by each client
  let mut measurements = vec![Ok(None); ident_nested_messages.len()];

  // refers to the layer that is currently being processed
  let mut layer_idx = 0;

  // loop through each layer in the tree and reveal partial measurment
  while layer_idx < num_layers {
    let mut next_layers = Vec::new();

    // loop through each node within the current layer of the tree
    for node in current_layer {
      // filter the subsets of the current node by all possible
      // leaves in the next layer
      let subsets = filter_node(&node, threshold);

      // loop through each subset of long-term identifiers
      // associated with the next layer of leaves
      for indices in subsets {
        // retrieve indexed messages to perform recovery
        let messages: Vec<&Message> = indices
          .iter()
          .map(|&ident| {
            ident_nested_messages[ident]
              .as_ref()
              .unwrap()
              .get_next_layer_message()
          })
          .collect();

        let key = match key_recover(&messages, epoch) {
          Err(e) => {
            indices
              .iter()
              .for_each(|&idx| measurements[idx] = Err(e.clone()));
            continue;
          }
          Ok(k) => k,
        };
        // returns an ordered vector of partial measurements for
        // the current leaf
        let pms = recover(&messages, &key);
        if let Some(res) = pms.iter().find(|res| res.is_err()) {
          indices
            .iter()
            .for_each(|&idx| measurements[idx] = Err(res.clone().unwrap_err()));
          continue;
        };

        // We may need to decrypt the next layer of STAR
        // messages for those partial measurments which have
        // been recovered
        if layer_idx + 1 < num_layers {
          let decrypted_messages = (0..indices.len())
            .map(|i| {
              let ident = indices[i];
              let key = pms[i]
                .as_ref()
                .unwrap()
                .get_next_layer_key()
                .as_ref()
                .unwrap();
              let msg = ident_nested_messages[ident].as_mut().unwrap();
              msg.decrypt_next_layer(key).unwrap();
              msg.get_next_layer()
            })
            .collect();

          // store the decrypted messages for the next layer
          next_layers.push(decrypted_messages);
        } else {
          (0..indices.len())
            .for_each(|i| ident_nested_messages[indices[i]] = None);
        }

        // set the current partial outputs
        (0..indices.len()).for_each(|j| {
          let idx = indices[j];
          measurements[idx] =
            Ok(Some(FinalMeasurement::from(pms[j].as_ref().unwrap())));
        });
      }
    }
    current_layer = next_layers;
    layer_idx += 1;
  }

  ident_nested_messages
    .into_iter()
    .zip(measurements)
    .map(|(ident_nested_msg, measurement)| match measurement {
      Err(e) => Err(e),
      Ok(msmt) => {
        // The measurement is empty in cases where no layers are decrypted, so these should be ignored.
        if msmt.is_none() {
          return Ok(PartialRecoveredMessage {
            measurement: None,
            next_message: None,
          });
        }

        // When the measurement is recovered then we output it
        let msg = match ident_nested_msg {
          None => None,
          Some(nm) => Some(nm.message),
        };
        Ok(PartialRecoveredMessage {
          measurement: msmt,
          next_message: msg,
        })
      }
    })
    .collect()
}

pub fn sample_layer_enc_keys(num_layers: usize) -> Vec<Vec<u8>> {
  let num_keys = num_layers - 1;
  let mut keys = Vec::with_capacity(num_keys);
  for _ in 0..num_keys {
    let mut enc_key = vec![0u8; LAYER_ENC_KEY_LEN];
    OsRng.fill_bytes(&mut enc_key);
    keys.push(enc_key);
  }
  keys
}

fn decode_message_plaintext(
  plaintext: &[u8],
) -> Result<(Vec<u8>, NestedAssociatedData), Error> {
  // parse all measurement bytes
  if let Some(buf) = load_bytes(plaintext) {
    // parse remaining bytes of auxiliary data
    let rem = &plaintext[4 + buf.len()..];

    if let Some(aux_bytes) = load_bytes(rem) {
      let measurement_bytes = buf.to_vec();

      return match bincode::deserialize(aux_bytes) {
        Ok(aux) => Ok((measurement_bytes, aux)),
        Err(e) => Err(Error::Serialization(format!(
          "bincode::deserialize failed recovering aux data: {e}"
        ))),
      };
    }
  }
  Err(Error::MessageParse)
}

/// Run the standard star recovery procedure for an array of STAR
/// messages
pub fn recover(
  subset: &[&Message],
  key: &[u8],
) -> Vec<Result<PartialMeasurement, Error>> {
  let mut measurement_for_validation = None;

  let plaintexts = subset
    .iter()
    .map(|t| t.ciphertext.decrypt(key, "star_encrypt"));

  plaintexts
    .map(|plaintext| {
      decode_message_plaintext(&plaintext).and_then(
        |(measurement_bytes, aux)| {
          // check that decrypted measurements all have the same value.
          match measurement_for_validation.as_ref() {
            Some(measurement_for_validation) => {
              // Compare current measurement to the first successful measurement
              if measurement_for_validation != &measurement_bytes {
                return Err(Error::ClientMeasurementMismatch(
                  serde_json::to_string(measurement_for_validation).unwrap(),
                  serde_json::to_string(&measurement_bytes).unwrap(),
                ));
              }
            }
            None => {
              // Store the first successful measurement so we can
              // compare subsequent measurements to this one.
              measurement_for_validation = Some(measurement_bytes.clone());
            }
          }

          let y = vec![measurement_bytes];
          Ok(PartialMeasurement {
            measurement: NestedMeasurement::new(&y).unwrap(),
            aux,
          })
        },
      )
    })
    .collect()
}

/// Runs the standard STAR key recovery mechanism
pub fn key_recover(layer: &[&Message], epoch: u8) -> Result<Vec<u8>, Error> {
  let mut result = vec![0u8; LAYER_ENC_KEY_LEN];
  let shares: Vec<Share> = layer.iter().map(|m| m.share.clone()).collect();
  let res = share_recover(&shares);
  if res.is_err() {
    return Err(Error::ShareRecovery);
  }
  let message = res.unwrap().get_message();
  derive_ske_key(&message, &[epoch], &mut result);
  Ok(result)
}

/// Filters out subsets of matching STAR messages that are smaller than
/// the threshold
fn filter_node(node: &[IdentMessage], threshold: u32) -> Vec<Vec<usize>> {
  let collected = group_messages(node);
  collected
    .into_iter()
    .filter(|bucket| bucket.len() >= threshold as usize)
    .collect()
}

/// Groups STAR messages by tag
fn group_messages(node: &[IdentMessage]) -> Vec<Vec<usize>> {
  let mut grouped: HashMap<String, Vec<usize>> = HashMap::new();
  for im in node {
    let s = format!("{:x?}", im.get_tag());
    match grouped.entry(s) {
      Entry::Vacant(e) => {
        e.insert(vec![im.ident]);
      }
      Entry::Occupied(mut e) => {
        e.get_mut().push(im.ident);
      }
    }
  }
  grouped.values().cloned().collect()
}

#[cfg(test)]
mod tests {
  use super::*;
  use base64::prelude::{Engine as _, BASE64_STANDARD as BASE64};
  use insta::assert_snapshot;
  use sta_rs::share_recover;

  /// Example sta_rs::Share value for testing
  ///
  /// The other Message fields are unchecked, but the share member
  /// needs to parse properly to instantiate. A valid test value
  /// can be generated as follows:
  ///
  /// ```
  /// # let threshold = 2;
  /// # let epoch = "t";
  /// let measurement = sta_rs::SingleMeasurement::new("test example".as_bytes());
  /// let mg = sta_rs::MessageGenerator::new(measurement, threshold, epoch);
  /// let mut rnd = [0u8; sta_rs::DIGEST_LEN];
  /// mg.sample_local_randomness(&mut rnd);
  /// let message = sta_rs::Message::generate(&mg, &mut rnd, None).unwrap();
  /// println!("EXAMPLE_SHARE: {}", BASE64.encode(message.share.to_bytes()));
  /// ```
  const EXAMPLE_SHARE: &str = "AgAAADAAAAAjbxRyzvFKynnzP/l2NQ8MAAAAAAAAAAAZBEVLkZXDJHvki6l75wXFAAAAAAAAAAAgAAAA4rynd5v1ane0FkR8aVvfDFwP+Y+mHhpZFWujfWErvvAgAAAAdasndZJ68kHipgIaudx6x9J0dzv9RSTuPprqAZOjk/2VTqVc+zQwXCNSCt9oUwZgA9M7ELpeyeYoaZHk6W0GbRaW7ptj73/Zrtg26acmwi1+EDb3ObNKNojcHuOQjAWY";

  #[test]
  fn construct_measurement() {
    let measurement = vec![vec![1u8; 16], vec![2u8; 32], vec![3u8; 48]];
    let nm = NestedMeasurement::new(measurement.as_slice()).unwrap();
    assert_eq!(nm.0[0].as_vec(), measurement[0].to_vec());
    assert_eq!(nm.0[1].as_vec(), measurement[1].to_vec());
    assert_eq!(nm.0[2].as_vec(), measurement[2].to_vec());
  }

  #[test]
  fn bad_number_of_layer_enc_keys() {
    let threshold = 1;
    let epoch = 0u8;
    let (_, mgs, mut keys) =
      sample_client_measurement(&[1u8, 2u8, 3u8], threshold, epoch);
    keys.pop();
    let rand = sample_randomness(&mgs);
    let nmsg = NestedMessage::new(&mgs, &rand, &keys, &[], epoch);
    assert!(nmsg.is_err());
    assert_eq!(nmsg, Err(Error::LayerEncryptionKeys(keys.len(), 2)));
  }

  #[test]
  fn bad_number_of_randomness_byte_arrays() {
    let threshold = 1;
    let epoch = 0u8;
    let (_, mgs, keys) =
      sample_client_measurement(&[1u8, 2u8, 3u8], threshold, epoch);
    let mut rand = sample_randomness(&mgs);
    rand.pop();
    let nmsg = NestedMessage::new(&mgs, &rand, &keys, &[], epoch);
    assert!(nmsg.is_err());
    assert_eq!(
      nmsg,
      Err(Error::NumMeasurementLayers(mgs.len(), rand.len()))
    );
  }

  #[test]
  fn construct_and_check_message_without_aux() {
    construct_message(None);
  }

  #[test]
  fn construct_and_check_message_with_aux_1() {
    construct_message(Some(vec![1u8; 3]));
  }

  #[test]
  fn construct_and_check_message_with_aux_2() {
    construct_message(Some(vec![2u8; 15]));
  }

  #[test]
  fn nested_message_serialization() {
    let nm = NestedMessage {
      epoch: 1u8,
      unencrypted_layer: Message {
        ciphertext: Ciphertext::from_bytes(&[7u8; 40]),
        share: Share::from_bytes(&BASE64.decode(EXAMPLE_SHARE).unwrap())
          .unwrap(),
        tag: vec![12u8; 32],
      },
      encrypted_layers: vec![
        Ciphertext::from_bytes(&[2u8; 16]),
        Ciphertext::from_bytes(&[2u8; 25]),
        Ciphertext::from_bytes(&[2u8; 33]),
      ],
    };
    let snm = SerializableNestedMessage::from(nm);

    let snm_bincode =
      bincode::serialize(&snm).expect("Should serialize to bincode");

    assert_snapshot!(BASE64.encode(&snm_bincode));

    bincode::deserialize::<SerializableNestedMessage>(&snm_bincode)
      .expect("Should load bincode");
  }

  #[test]
  fn nested_message_bad_data_load() {
    assert!(
      bincode::deserialize::<SerializableNestedMessage>(&[7u8; 58]).is_err()
    );
    assert!(
      bincode::deserialize::<SerializableNestedMessage>(&[21u8; 9000]).is_err()
    );
  }

  #[test]
  fn end_to_end_basic_no_aux() {
    end_to_end_basic(None);
  }

  #[test]
  fn end_to_end_basic_with_aux() {
    let aux = Some(vec![1u8; 2]);
    end_to_end_basic(aux);
  }

  #[test]
  fn end_to_end_basic_with_aux_2() {
    let aux = Some(vec![4u8; 5]);
    end_to_end_basic(aux);
  }

  #[test]
  fn end_to_end_advanced_no_aux() {
    end_to_end_advanced(None);
  }

  #[test]
  fn end_to_end_advanced_with_aux() {
    let aux = Some(vec![1u8; 3]);
    end_to_end_advanced(aux);
  }

  #[test]
  fn end_to_end_advanced_with_aux_2() {
    let aux = Some(vec![3u8; 7]);
    end_to_end_advanced(aux);
  }

  fn end_to_end_basic(aux: Option<Vec<u8>>) {
    let threshold: usize = 3;
    let measurement_len = 5;
    let num_clients = 3;
    let (outputs, measurements) = end_to_end(
      (0..num_clients)
        .map(|_| vec![1u8, 2u8, 3u8, 4u8, 5u8])
        .collect(),
      &aux,
      threshold,
      measurement_len,
    );
    for i in 0..num_clients {
      let output_layer = &outputs[i];
      assert_measurement_output_equality(
        output_layer,
        &measurements[i],
        true,
        Some(5),
      );
      assert_aux_equality(&aux, output_layer);
    }
  }

  fn end_to_end_advanced(aux: Option<Vec<u8>>) {
    let threshold: usize = 3;
    let measurement_len = 5;
    let mut full_input: Vec<Vec<u8>> =
      (0..5).map(|_| vec![1u8, 2u8, 3u8, 4u8, 5u8]).collect();
    full_input.extend(
      (0..3)
        .map(|_| vec![1u8, 2u8, 3u8, 5u8, 6u8])
        .collect::<Vec<Vec<u8>>>(),
    );
    full_input.extend(
      (0..1)
        .map(|_| vec![1u8, 2u8, 5u8, 6u8, 7u8])
        .collect::<Vec<Vec<u8>>>(),
    );
    full_input.extend(
      (0..2)
        .map(|_| vec![2u8, 3u8, 4u8, 5u8, 6u8])
        .collect::<Vec<Vec<u8>>>(),
    );
    full_input.extend(
      (0..3)
        .map(|_| vec![3u8, 4u8, 5u8, 6u8, 7u8])
        .collect::<Vec<Vec<u8>>>(),
    );
    full_input.extend(
      (0..1)
        .map(|_| vec![3u8, 4u8, 5u8, 7u8, 8u8])
        .collect::<Vec<Vec<u8>>>(),
    );
    let (outputs, measurements) =
      end_to_end(full_input, &aux, threshold, measurement_len);
    for i in 0..outputs.len() {
      let mut revealed_len = measurement_len;
      let output = &outputs[i];
      let measurement = &measurements[i];
      if i > 8 && i < 11 {
        assert_measurement_output_equality(output, measurement, false, None);
        continue;
      } else if i == 8 {
        revealed_len = 2;
      } else if i == 14 {
        revealed_len = 3;
      }
      assert_measurement_output_equality(
        output,
        measurement,
        true,
        Some(revealed_len),
      );
      assert_aux_equality(&aux, output);
    }
  }

  fn end_to_end(
    inputs: Vec<Vec<u8>>,
    aux: &Option<Vec<u8>>,
    threshold: usize,
    measurement_len: usize,
  ) -> (Vec<Option<FinalMeasurement>>, Vec<NestedMeasurement>) {
    let epoch = 0u8;
    let num_clients = inputs.len();
    let mut messages: Vec<NestedMessage> = Vec::new();
    let mut measurements = Vec::new();
    for input in inputs.iter().take(num_clients) {
      let (nested_m, mgs, keys) =
        sample_client_measurement(input, threshold as u32, epoch);
      let mut added_data = &vec![];
      if aux.is_some() {
        added_data = aux.as_ref().unwrap();
      }
      let rand = sample_randomness(&mgs);
      let nested_message =
        NestedMessage::new(&mgs, &rand, &keys, added_data, epoch).unwrap();
      messages.push(nested_message);
      measurements.push(nested_m);
    }
    let output_results = recover_partial_measurements(
      &messages,
      epoch,
      threshold as u32,
      measurement_len,
    );
    let outputs = output_results
      .iter()
      .map(|x| x.as_ref().unwrap().measurement.clone())
      .collect();
    (outputs, measurements)
  }

  fn assert_measurement_output_equality(
    output: &Option<FinalMeasurement>,
    measurement: &NestedMeasurement,
    revealed: bool,
    revealed_len: Option<usize>,
  ) {
    if !revealed {
      assert!(output.is_none());
    } else {
      let revealed_output = output.as_ref().unwrap();
      assert_eq!(revealed_output.measurement.len(), 1);
      assert_eq!(
        revealed_output.get_partial_measurement_raw(0),
        measurement.get_layer_as_bytes(revealed_len.unwrap() - 1)
      );
    }
  }

  fn assert_aux_equality(
    aux: &Option<Vec<u8>>,
    output: &Option<FinalMeasurement>,
  ) {
    if output.is_some() {
      let no_aux = aux.is_none();
      if !no_aux {
        let aux_check = aux.as_ref().unwrap();
        assert_eq!(&output.as_ref().unwrap().data, aux_check);
      }
    }
  }

  fn sample_client_measurement(
    vals: &[u8],
    threshold: u32,
    epoch: u8,
  ) -> (NestedMeasurement, Vec<MessageGenerator>, Vec<Vec<u8>>) {
    let mut measurement: Vec<Vec<u8>> = Vec::new();
    // There is no limit on measurement lengths, we just fix these for
    // making it possible to establish test vectors
    let check_lens = [32, 91, 400];
    for (i, &x) in vals.iter().enumerate() {
      measurement.push(vec![x; check_lens[i % check_lens.len()]]);
    }
    let nm = NestedMeasurement::new(&measurement).unwrap();
    let mgs = nm.get_message_generators(threshold, epoch);
    let keys = sample_layer_enc_keys(nm.len());
    (nm, mgs, keys)
  }

  fn construct_message(aux: Option<Vec<u8>>) {
    let threshold = 1;
    let epoch = 0u8;
    let (nm, mgs, keys) =
      sample_client_measurement(&[1u8, 2u8, 3u8], threshold, epoch);

    // check tags and measurement in each layer
    let mut added_data = &vec![];
    if aux.is_some() {
      added_data = aux.as_ref().unwrap();
    }
    let rand = sample_randomness(&mgs);
    let mut nested_message =
      NestedMessage::new(&mgs, &rand, &keys, added_data, epoch).unwrap();
    let checks = vec![
      vec![
        171, 38, 129, 158, 77, 71, 82, 131, 243, 52, 6, 92, 214, 67, 67, 126,
        65, 245, 244, 10, 227, 83, 71, 88, 151, 34, 13, 132, 202, 224, 160,
        119,
      ],
      vec![
        0, 194, 167, 145, 145, 232, 142, 69, 186, 245, 187, 201, 46, 220, 222,
        58, 71, 157, 253, 5, 198, 73, 244, 146, 64, 194, 149, 20, 228, 217,
        201, 140,
      ],
      vec![
        67, 188, 75, 167, 174, 162, 3, 192, 54, 197, 53, 85, 139, 165, 228, 65,
        221, 98, 95, 103, 27, 244, 179, 115, 130, 210, 247, 22, 143, 113, 171,
        185,
      ],
    ];
    let aux_ref = aux.as_ref();
    for i in 0..3 {
      assert_eq!(checks[i], nested_message.unencrypted_layer.tag);
      let value =
        share_recover(&[nested_message.unencrypted_layer.share.clone()])
          .unwrap()
          .get_message();
      let mut star_key = vec![0u8; LAYER_ENC_KEY_LEN];
      derive_ske_key(&value, &[epoch], &mut star_key);
      let res = nested_message
        .unencrypted_layer
        .ciphertext
        .decrypt(&star_key, "star_encrypt");

      // check measurement value first 4 bytes are just for length
      let res_measurement = load_bytes(&res).unwrap().to_vec();
      assert_eq!(res_measurement, nm.get_layer_as_bytes(i));

      // check aux data
      let aux_check_bytes = load_bytes(&res[4 + res_measurement.len()..])
        .unwrap()
        .to_vec();
      let mut add_data = NestedAssociatedData {
        key: None,
        data: vec![],
      };
      if i < 2 {
        add_data.key = Some(keys[i].clone());
      }
      if aux.is_some() {
        add_data.data = aux_ref.unwrap().to_vec();
      } else {
        add_data.data = vec![];
      }
      let serialized_aux = bincode::serialize(&add_data).unwrap();
      assert_eq!(aux_check_bytes.len(), serialized_aux.len());
      assert_eq!(aux_check_bytes, serialized_aux);
      let add_data_unserialized: NestedAssociatedData =
        bincode::deserialize(&serialized_aux).unwrap();
      assert_eq!(add_data_unserialized.key, add_data.key);
      assert_eq!(add_data_unserialized.data, add_data.data);

      // decrypt next layer
      if i < 2 {
        nested_message.decrypt_next_layer(&keys[i]).unwrap();
      }
    }
  }

  fn sample_randomness(mgs: &[MessageGenerator]) -> Vec<[u8; RANDOMNESS_LEN]> {
    let mut rnd_buf = [0u8; 32];
    let mut rand = Vec::new();
    for mg in mgs.iter() {
      mg.sample_local_randomness(&mut rnd_buf);
      rand.push(rnd_buf);
    }
    rand
  }
}
