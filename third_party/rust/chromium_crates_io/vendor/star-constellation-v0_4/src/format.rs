//! Define various stages in message generation and formatting
use serde::{Deserialize, Serialize};

use crate::consts::*;
use crate::internal::{
  NestedMeasurement, NestedMessage, PartialRecoveredMessage,
};
use crate::Error;
use sta_rs::SingleMeasurement;

/// Serializes a `NestedMeasurement` together with a ppoprf epoch
/// in a format compatible with randomness server interactions.
#[derive(Serialize, Deserialize, Clone)]
pub struct RandomnessSampling {
  input: Vec<Vec<u8>>,
  epoch: u8,
}

impl RandomnessSampling {
  pub fn new(nm: &NestedMeasurement, epoch: u8) -> Self {
    Self {
      input: (0..nm.len()).map(|i| nm.get_layer_as_bytes(i)).collect(),
      epoch,
    }
  }

  pub fn input(&self) -> &[Vec<u8>] {
    &self.input
  }

  pub fn epoch(&self) -> u8 {
    self.epoch
  }

  pub fn input_len(&self) -> usize {
    self.input.len()
  }
}
impl From<&RandomnessSampling> for NestedMeasurement {
  fn from(rsm: &RandomnessSampling) -> NestedMeasurement {
    NestedMeasurement(
      rsm
        .input
        .iter()
        .map(|x| SingleMeasurement::new(x))
        .collect(),
    )
  }
}

/// Holds a `RandomnessSampling` together with a randomness server
/// response, used to construct the final encoded message.
#[derive(Serialize, Deserialize, Clone)]
pub struct MessageGeneration {
  input: Vec<Vec<u8>>,
  rand: Vec<[u8; RANDOMNESS_LEN]>,
  pub epoch: u8,
}
impl MessageGeneration {
  pub fn new(
    rsf: RandomnessSampling,
    input_rand: Vec<[u8; RANDOMNESS_LEN]>,
  ) -> Result<Self, Error> {
    if rsf.input_len() != input_rand.len() {
      return Err(Error::NumMeasurementLayers(
        rsf.input_len(),
        input_rand.len(),
      ));
    }
    Ok(Self {
      input: rsf.input.clone(),
      rand: input_rand,
      epoch: rsf.epoch(),
    })
  }

  pub fn rand(&self) -> Vec<[u8; RANDOMNESS_LEN]> {
    self.rand.clone()
  }

  pub fn epoch(&self) -> u8 {
    self.epoch
  }

  pub fn input_len(&self) -> usize {
    self.input.len()
  }
}
impl From<MessageGeneration> for NestedMeasurement {
  fn from(rsm: MessageGeneration) -> NestedMeasurement {
    NestedMeasurement(
      rsm
        .input
        .iter()
        .map(|x| SingleMeasurement::new(x))
        .collect(),
    )
  }
}

/// The `OutputMeasurement` struct is a serializable data object
/// that holds occurrence counts and auxiliary data for all partial
/// measurements reported by clients.
#[derive(Clone, Debug)]
pub struct OutputMeasurement {
  value: Vec<String>,
  occurrences: usize,
  auxiliary_data: Vec<Vec<u8>>,
  locked_nested_messages: Vec<NestedMessage>,
}
impl OutputMeasurement {
  pub fn increment(&mut self, aux: Vec<u8>, nested_msg: Option<NestedMessage>) {
    self.occurrences += 1;
    self.auxiliary_data.push(aux);
    if let Some(msg) = nested_msg {
      self.locked_nested_messages.push(msg);
    }
  }

  pub fn value(&self) -> Vec<String> {
    self.value.clone()
  }

  pub fn occurrences(&self) -> usize {
    self.occurrences
  }

  pub fn auxiliary_data(&self) -> Vec<Vec<u8>> {
    self.auxiliary_data.clone()
  }
}
impl From<PartialRecoveredMessage> for OutputMeasurement {
  fn from(prm: PartialRecoveredMessage) -> Self {
    let s = String::from_utf8(prm.get_measurement_raw()).unwrap();
    let value = String::from(s.trim_end_matches(char::from(0)));
    Self {
      value: vec![value],
      occurrences: 1,
      auxiliary_data: vec![prm.get_aux_data()],
      locked_nested_messages: match prm.next_message {
        Some(v) => vec![v],
        None => Vec::new(),
      },
    }
  }
}

/// The `AggregationResult` struct wraps recovered output measurements,
/// plus a count of all errors that occurred during the share recovery
/// process.
#[derive(Debug)]
pub struct AggregationResult {
  outputs: Vec<OutputMeasurement>,
  num_serde_errors: usize,
  num_recovery_errors: usize,
}
impl AggregationResult {
  pub fn new(
    outputs: Vec<OutputMeasurement>,
    num_serde_errors: usize,
    num_recovery_errors: usize,
  ) -> Self {
    Self {
      outputs,
      num_serde_errors,
      num_recovery_errors,
    }
  }

  pub fn outputs(&self) -> Vec<OutputMeasurement> {
    self.outputs.clone()
  }

  pub fn num_serde_errors(&self) -> usize {
    self.num_serde_errors
  }

  pub fn num_recovery_errors(&self) -> usize {
    self.num_recovery_errors
  }
}
