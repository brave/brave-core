//! The `star-constellation` crate implements the Constellation
//! aggregation mechanism: a modification of the original
//! [STAR](https://github.com/brave/sta-rs) protocol to allow
//! clients to submit ordered, granular data at the highest
//! resolution that is possible, whilst maintaining crowd-based
//! anonymity.
//!
//! Constellation provides both higher utility for data aggregation
//! than STAR alone (revealing partial measurements where possible),
//! and better privacy for fine-grained client data.
//!
//! ## Background
//!
//! Specifically, Constellation 'nests' or 'layers' an ordered vector of
//! measurements into associated STAR messages, such that each message
//! can only be accessed if the STAR message at the previous layer was
//! included in a successful recovery. The privacy of unrevealed layers
//! is provided using symmetric encryption, that can only be decrypted
//! using a key enclosed in the previous STAR message.
//!
//! ## Example API usage
//!
//! ### Client
//!
//! The Client produces a message for threshold aggregation using the
//! Constellation format.
//!
//! ```
//! # use star_constellation::api::*;
//! # use star_constellation::randomness::*;
//! # use star_constellation::randomness::testing::{LocalFetcher as RandomnessFetcher};
//! # use star_constellation::consts::*;
//! # use star_constellation::format::*;
//! #
//! let threshold = 10;
//! let epoch = 0u8;
//! let random_fetcher = RandomnessFetcher::new();
//!
//! // setup randomness server information
//! let example_aux = vec![1u8; 3];
//!
//! let measurements = vec!["hello".as_bytes().to_vec(), "world".as_bytes().to_vec()];
//! let rrs = client::prepare_measurement(&measurements, epoch).unwrap();
//! let req = client::construct_randomness_request(&rrs);
//!
//! let req_slice_vec: Vec<&[u8]> = req.iter().map(|v| v.as_slice()).collect();
//! let resp = random_fetcher.eval(&req_slice_vec, epoch).unwrap();
//!
//! let points_slice_vec: Vec<&[u8]> =
//!   resp.serialized_points.iter().map(|v| v.as_slice()).collect();
//! let proofs_slice_vec: Vec<&[u8]> =
//!   resp.serialized_proofs.iter().map(|v| v.as_slice()).collect();
//! client::construct_message(
//!   &points_slice_vec,
//!   Some(&proofs_slice_vec),
//!   &rrs,
//!   &Some(random_fetcher.get_server().get_public_key()),
//!   &example_aux,
//!   threshold
//! ).unwrap();
//! ```
//!
//! ### Server
//!
//! Server aggregation takes a number of client messages as input, and
//! outputs those measurements that were received from at least
//! `threshold` clients. It also reveals prefixes of full measurements
//! that were received by greater than `threshold` clients.
//!
//! #### Full recovery
//!
//! After receiving at least `threshold` client messages of the same
//! full measurement, the server can run aggregation and reveal the
//! client measurement.
//!
//! ```
//! # use star_constellation::api::*;
//! # use star_constellation::randomness::*;
//! # use star_constellation::randomness::testing::{LocalFetcher as RandomnessFetcher};
//! # use star_constellation::consts::*;
//! # use star_constellation::format::*;
//! #
//! let threshold = 10;
//! let epoch = 0u8;
//! let random_fetcher = RandomnessFetcher::new();
//!
//! // construct at least `threshold` client messages with the same measurement
//! let measurements_1 = vec!["hello".as_bytes().to_vec(), "world".as_bytes().to_vec()];
//! let client_messages_to_reveal: Vec<Vec<u8>> = (0..threshold).into_iter().map(|i| {
//!   let example_aux = vec![i as u8; 3];
//!   let rrs = client::prepare_measurement(&measurements_1, epoch).unwrap();
//!   let req = client::construct_randomness_request(&rrs);
//!
//!   let req_slice_vec: Vec<&[u8]> = req.iter().map(|v| v.as_slice()).collect();
//!   let resp = random_fetcher.eval(&req_slice_vec, epoch).unwrap();
//!
//!   let points_slice_vec: Vec<&[u8]> =
//!     resp.serialized_points.iter().map(|v| v.as_slice()).collect();
//!   let proofs_slice_vec: Vec<&[u8]> =
//!     resp.serialized_proofs.iter().map(|v| v.as_slice()).collect();
//!   client::construct_message(
//!     &points_slice_vec,
//!     Some(&proofs_slice_vec),
//!     &rrs,
//!     &Some(random_fetcher.get_server().get_public_key()),
//!     &example_aux,
//!     threshold
//!   ).unwrap()
//! }).collect();
//!
//! // construct a low number client messages with a different measurement
//! let measurements_2 = vec!["something".as_bytes().to_vec(), "else".as_bytes().to_vec()];
//! let client_messages_to_hide: Vec<Vec<u8>> = (0..2).into_iter().map(|i| {
//!   let example_aux = vec![i as u8; 3];
//!   let rrs = client::prepare_measurement(&measurements_2, epoch).unwrap();
//!   let req = client::construct_randomness_request(&rrs);
//!
//!   let req_slice_vec: Vec<&[u8]> = req.iter().map(|v| v.as_slice()).collect();
//!   let resp = random_fetcher.eval(&req_slice_vec, epoch).unwrap();
//!
//!   let points_slice_vec: Vec<&[u8]> =
//!     resp.serialized_points.iter().map(|v| v.as_slice()).collect();
//!   let proofs_slice_vec: Vec<&[u8]> =
//!     resp.serialized_proofs.iter().map(|v| v.as_slice()).collect();
//!   client::construct_message(
//!     &points_slice_vec,
//!     Some(&proofs_slice_vec),
//!     &rrs,
//!     &Some(random_fetcher.get_server().get_public_key()),
//!     &example_aux,
//!     threshold
//!   ).unwrap()
//! }).collect();
//!
//! // aggregation reveals the client measurement that reaches the
//! // threshold, the other measurement stays hidden
//! let agg_res = server::aggregate(
//!   &[client_messages_to_reveal, client_messages_to_hide].concat(),
//!   threshold,
//!   epoch,
//!   measurements_1.len()
//! );
//! let output = agg_res.outputs();
//! assert_eq!(output.len(), 1);
//! let revealed_output = output.iter().find(|v| v.value() == vec!["world"]).unwrap();
//! assert_eq!(revealed_output.value(), vec!["world"]);
//! assert_eq!(revealed_output.occurrences(), 10);
//! (0..10).into_iter().for_each(|i| {
//!   assert_eq!(revealed_output.auxiliary_data()[i], vec![i as u8; 3]);
//! });
//! ```
//!
//! #### Partial recovery
//!
//! Partial recovery allows revealing prefixes of full measurements that
//! are received by enough clients, even when the full measurements
//! themselves stay hidden.
//!
//! ```
//! # use star_constellation::api::*;
//! # use star_constellation::randomness::*;
//! # use star_constellation::randomness::testing::{LocalFetcher as RandomnessFetcher};
//! # use star_constellation::consts::*;
//! # use star_constellation::format::*;
//! #
//! let threshold = 10;
//! let epoch = 0u8;
//! let random_fetcher = RandomnessFetcher::new();
//!
//! // construct a low number client messages with the same measurement
//! let measurements_1 = vec!["hello".as_bytes().to_vec(), "world".as_bytes().to_vec()];
//! let client_messages_1: Vec<Vec<u8>> = (0..5).into_iter().map(|i| {
//!   let example_aux = vec![i as u8; 3];
//!   let rrs = client::prepare_measurement(&measurements_1, epoch).unwrap();
//!   let req = client::construct_randomness_request(&rrs);
//!
//!   let req_slice_vec: Vec<&[u8]> = req.iter().map(|v| v.as_slice()).collect();
//!   let resp = random_fetcher.eval(&req_slice_vec, epoch).unwrap();
//!
//!   let points_slice_vec: Vec<&[u8]> =
//!     resp.serialized_points.iter().map(|v| v.as_slice()).collect();
//!   let proofs_slice_vec: Vec<&[u8]> =
//!     resp.serialized_proofs.iter().map(|v| v.as_slice()).collect();
//!   client::construct_message(
//!     &points_slice_vec,
//!     Some(&proofs_slice_vec),
//!     &rrs,
//!     &Some(random_fetcher.get_server().get_public_key()),
//!     &example_aux,
//!     threshold
//!   ).unwrap()
//! }).collect();
//!
//! // construct a low number of measurements that also share a prefix
//! let measurements_2 = vec!["hello".as_bytes().to_vec(), "goodbye".as_bytes().to_vec()];
//! let client_messages_2: Vec<Vec<u8>> = (0..5).into_iter().map(|i| {
//!   let example_aux = vec![i as u8; 3];
//!   let rrs = client::prepare_measurement(&measurements_2, epoch).unwrap();
//!   let req = client::construct_randomness_request(&rrs);
//!
//!   let req_slice_vec: Vec<&[u8]> = req.iter().map(|v| v.as_slice()).collect();
//!   let resp = random_fetcher.eval(&req_slice_vec, epoch).unwrap();
//!
//!   let points_slice_vec: Vec<&[u8]> =
//!     resp.serialized_points.iter().map(|v| v.as_slice()).collect();
//!   let proofs_slice_vec: Vec<&[u8]> =
//!     resp.serialized_proofs.iter().map(|v| v.as_slice()).collect();
//!   client::construct_message(
//!     &points_slice_vec,
//!     Some(&proofs_slice_vec),
//!     &rrs,
//!     &Some(random_fetcher.get_server().get_public_key()),
//!     &example_aux,
//!     threshold
//!   ).unwrap()
//! }).collect();
//!
//! // aggregation reveals the partial client measurement `vec!["hello"]`,
//! // but the full measurements stay hidden
//! let agg_res = server::aggregate(
//!   &[client_messages_1, client_messages_2].concat(),
//!   threshold,
//!   epoch,
//!   measurements_1.len()
//! );
//! let output = agg_res.outputs();
//! assert_eq!(output.len(), 1);
//! assert_eq!(output[0].value(), vec!["hello"]);
//! assert_eq!(output[0].occurrences(), 10);
//! (0..10).into_iter().for_each(|i| {
//!   let val = i % 5;
//!   assert_eq!(output[0].auxiliary_data()[i], vec![val as u8; 3]);
//! });
//! ```
mod internal;

pub mod api;
pub mod format;
pub mod randomness;

pub mod consts {
  pub const RANDOMNESS_LEN: usize = 32;
}

#[derive(Debug, Clone, Eq, PartialEq)]
pub enum Error {
  ShareRecovery,
  ClientMeasurementMismatch(String, String),
  LayerEncryptionKeys(usize, usize),
  NumMeasurementLayers(usize, usize),
  Serialization(String),
  RandomnessSampling(String),
  MessageGeneration(String),
  MessageParse,
  ProofMissing,
  MissingVerificationParams,
}

impl std::error::Error for Error {}

impl std::fmt::Display for Error {
  fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
    match self {
      Self::ShareRecovery => write!(f, "Internal share recovery failed"),
      Self::ClientMeasurementMismatch(original, received) => write!(f, "Clients sent differing measurement for identical share sets, original: {original}, received: {received}"),
      Self::LayerEncryptionKeys(nkeys, nlayers) => write!(f, "Number of encryption keys ({nkeys}) provided for nested encryptions is not compatible with number of layers specified ({nlayers})."),
      Self::NumMeasurementLayers(current, expected) => write!(f, "Number of inferred measurement layers is {current}, but expected is {expected}."),
      Self::Serialization(err_string) => write!(f, "An error occurred during serialization/deserialization: {err_string}."),
      Self::RandomnessSampling(err_string) => write!(f, "An error occurred during the sampling of randomness: {err_string}."),
      Self::MessageGeneration(err_string) => write!(f, "An error when attempting to generate the message: {err_string}."),
      Self::MessageParse => write!(f, "An error when attempting to parse the message."),
      Self::ProofMissing => write!(f, "Proof missing for randomness point."),
      Self::MissingVerificationParams => write!(f, "Verification key or proofs missing, must supply both or none.")
    }
  }
}
