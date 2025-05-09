//! The `api` module consists of the client- and server-specific
//! functions for producing and aggregating messages, respectively.

pub use crate::internal::{key_recover, recover};
pub use crate::internal::{
  NestedMessage, PartialMeasurement, SerializableNestedMessage,
};

/// Oblivious pseudo-random function used to make submissions
/// unpredictable.
pub use ppoprf;

pub mod client {
  //! The client module wraps all API functions used by clients for
  //! constructing their aggregation messages relative to the
  //! Constellation aggregation protocol.
  //!
  //! The default implementations of each of the functions can be used
  //! for running an example Client. Each of these functions can be
  //! swapped out for alternative implementations.
  use crate::format::*;
  use crate::internal::sample_layer_enc_keys;
  use crate::internal::NestedMeasurement;
  use crate::internal::{NestedMessage, SerializableNestedMessage};
  use crate::randomness::{
    process_randomness_response, RequestState as RandomnessRequestState,
  };
  use crate::Error;
  use ppoprf::ppoprf;

  /// The function `prepare_measurement` takes a vector of measurement
  /// values (serialized as bytes), and an epoch. A randomness request state
  /// containing the formatted measurement and blinded points will be returned.
  ///
  /// The output of the function can be passed as input to `construct_randomness_request()`.
  pub fn prepare_measurement(
    measurement: &[Vec<u8>],
    epoch: u8,
  ) -> Result<RandomnessRequestState, Error> {
    let nm = NestedMeasurement::new(measurement)?;
    let rsf = RandomnessSampling::new(&nm, epoch);
    Ok(RandomnessRequestState::new(rsf))
  }

  /// In `construct_randomness_request`, the client uses the output of
  /// `prepare_measurement` to construct a JSON request to be sent to
  /// the randomness server, to retrieve randomness for each layer of
  /// the nested measurement.
  ///
  /// Each element of the output of the function can base64 encoded, and
  /// sent to the randomness server.
  pub fn construct_randomness_request(
    rrs: &RandomnessRequestState,
  ) -> Vec<Vec<u8>> {
    rrs
      .blinded_points()
      .iter()
      .map(|p| p.as_bytes().to_vec())
      .collect()
  }

  /// In `construct_message` the client uses the base64 decoded output from
  /// the JSON response of the randomness server (the generated randomness) and the
  /// output of `prepare_measurement` (containing the nested measurement/blinded points)
  /// and generates a bincode-formatted aggregation message.
  ///
  /// The client can optionally specify any amount of additional data
  /// to be included with their message in `aux`.
  pub fn construct_message(
    randomness_points: &[&[u8]],
    randomness_proofs: Option<&[&[u8]]>,
    rrs: &RandomnessRequestState,
    verification_key: &Option<ppoprf::ServerPublicKey>,
    aux_bytes: &[u8],
    threshold: u32,
  ) -> Result<Vec<u8>, Error> {
    if (randomness_proofs.is_some() && verification_key.is_none())
      || (randomness_proofs.is_none() && verification_key.is_some())
    {
      return Err(Error::MissingVerificationParams);
    }
    let parsed_response = process_randomness_response(
      rrs.blinded_points(),
      randomness_points,
      randomness_proofs,
    )?;
    let finalized =
      rrs.finalize_response(&parsed_response, verification_key)?;
    let mgf = MessageGeneration::new(rrs.rsf().clone(), finalized)?;

    let nm: NestedMeasurement = mgf.clone().into();
    let mgs = nm.get_message_generators(threshold, mgf.epoch());
    let keys = sample_layer_enc_keys(mgf.input_len());

    let snm = SerializableNestedMessage::from(NestedMessage::new(
      &mgs,
      &mgf.rand(),
      &keys,
      aux_bytes,
      mgf.epoch(),
    )?);
    bincode::serialize(&snm).map_err(|e| Error::Serialization(e.to_string()))
  }
}

pub mod server {
  //! The server module wraps all public API functions used by the
  //! aggregation server.
  use crate::internal::recover_partial_measurements;
  use crate::internal::{NestedMessage, SerializableNestedMessage};
  use crate::{format::*, Error};
  use std::collections::hash_map::Entry;
  use std::collections::HashMap;

  /// The `aggregate` function is a public API function that takes
  /// a list of serialized Constellation messages as input (along
  /// with standard STAR parameters) and outputs a vector of output
  /// measurements using the Constellation recovery mechanism.
  ///
  /// The output measurements include the number of occurrences
  /// that were recorded, along with attached auxiliary data
  /// submitted by each contributing client.
  pub fn aggregate(
    snms_serialized: &[Vec<u8>],
    threshold: u32,
    epoch: u8,
    num_layers: usize,
  ) -> AggregationResult {
    let mut serde_errors = 0;
    let mut recovery_errors = 0;
    let mut nms = Vec::<NestedMessage>::new();
    for snm_ser in snms_serialized.iter() {
      let res = bincode::deserialize::<SerializableNestedMessage>(snm_ser)
        .map_err(|e| Error::Serialization(e.to_string()))
        .and_then(NestedMessage::try_from);
      if let Ok(nm) = res {
        nms.push(nm);
        continue;
      }
      serde_errors += 1;
    }
    let res_fms =
      recover_partial_measurements(&nms, epoch, threshold, num_layers);
    let mut output_map = HashMap::new();
    for res in res_fms {
      if let Ok(prm) = res {
        // ignore empty recoveries in output
        if prm.is_empty() {
          continue;
        }
        match output_map.entry(prm.get_measurement_raw()) {
          Entry::Vacant(e) => {
            let om = OutputMeasurement::from(prm);
            e.insert(om);
          }
          Entry::Occupied(mut e) => {
            e.get_mut().increment(prm.get_aux_data(), prm.next_message);
          }
        }
      } else {
        recovery_errors += 1;
      }
    }
    let out_vec: Vec<OutputMeasurement> = output_map.into_values().collect();
    AggregationResult::new(out_vec, serde_errors, recovery_errors)
  }
}

#[cfg(test)]
mod tests {
  use super::*;
  use crate::randomness::testing::{
    LocalFetcher, LocalFetcherResponse, PPOPRF_SERVER,
  };
  use serde_json::Value;

  #[test]
  fn basic_test() {
    let epoch = 0;
    let threshold = 1;
    let measurement =
      vec!["hello".as_bytes().to_vec(), "world".as_bytes().to_vec()];
    let random_fetcher = LocalFetcher::new();
    let aux = "added_data".as_bytes().to_vec();
    let rrs = client::prepare_measurement(&measurement, epoch).unwrap();
    let req = client::construct_randomness_request(&rrs);

    let req_slice_vec: Vec<&[u8]> = req.iter().map(|v| v.as_slice()).collect();
    let resp = random_fetcher.eval(&req_slice_vec, epoch).unwrap();
    let (points_slice_vec, proofs_slice_vec) =
      get_eval_output_slice_vecs(&resp);

    let msg = client::construct_message(
      &points_slice_vec,
      Some(&proofs_slice_vec),
      &rrs,
      &Some(PPOPRF_SERVER.get_public_key()),
      &aux,
      threshold,
    )
    .unwrap();
    let agg_res = server::aggregate(&[msg], threshold, epoch, 2);
    let outputs = agg_res.outputs();
    assert_eq!(outputs.len(), 1);
    assert_eq!(outputs[0].value(), vec!["world"]);
    assert_eq!(outputs[0].auxiliary_data(), vec![aux]);
    assert_eq!(agg_res.num_recovery_errors(), 0);
    assert_eq!(agg_res.num_serde_errors(), 0);
  }

  #[test]
  fn incompatible_epoch() {
    let c_epoch = 0u8;
    let threshold = 1;
    let measurement =
      vec!["hello".as_bytes().to_vec(), "world".as_bytes().to_vec()];
    let random_fetcher = LocalFetcher::new();
    let rrs = client::prepare_measurement(&measurement, c_epoch).unwrap();
    let req = client::construct_randomness_request(&rrs);

    let req_slice_vec: Vec<&[u8]> = req.iter().map(|v| v.as_slice()).collect();
    let resp = random_fetcher.eval(&req_slice_vec, c_epoch).unwrap();
    let (points_slice_vec, proofs_slice_vec) =
      get_eval_output_slice_vecs(&resp);

    let msg = client::construct_message(
      &points_slice_vec,
      Some(&proofs_slice_vec),
      &rrs,
      &Some(PPOPRF_SERVER.get_public_key()),
      &[],
      threshold,
    )
    .unwrap();
    let agg_res = server::aggregate(&[msg], threshold, 1u8, 2);
    assert_eq!(agg_res.num_recovery_errors(), 1);
    assert_eq!(agg_res.outputs().len(), 0);
  }

  #[test]
  fn incompatible_threshold() {
    let epoch = 0u8;
    let threshold = 3;
    let measurement =
      vec!["hello".as_bytes().to_vec(), "world".as_bytes().to_vec()];
    let random_fetcher = LocalFetcher::new();
    let messages: Vec<Vec<u8>> = (0..threshold - 1)
      .map(|_| {
        let rrs = client::prepare_measurement(&measurement, epoch).unwrap();
        let req = client::construct_randomness_request(&rrs);

        let req_slice_vec: Vec<&[u8]> =
          req.iter().map(|v| v.as_slice()).collect();
        let resp = random_fetcher.eval(&req_slice_vec, epoch).unwrap();
        let (points_slice_vec, proofs_slice_vec) =
          get_eval_output_slice_vecs(&resp);

        client::construct_message(
          &points_slice_vec,
          Some(&proofs_slice_vec),
          &rrs,
          &Some(PPOPRF_SERVER.get_public_key()),
          &[],
          threshold,
        )
        .unwrap()
      })
      .collect();
    let agg_res = server::aggregate(&messages, threshold - 1, epoch, 2);
    assert_eq!(agg_res.num_recovery_errors(), 2);
    assert_eq!(agg_res.outputs().len(), 0);
  }

  fn get_eval_output_slice_vecs(
    resp: &LocalFetcherResponse,
  ) -> (Vec<&[u8]>, Vec<&[u8]>) {
    (
      resp
        .serialized_points
        .iter()
        .map(|v| v.as_slice())
        .collect(),
      resp
        .serialized_proofs
        .iter()
        .map(|v| v.as_slice())
        .collect(),
    )
  }

  #[test]
  fn end_to_end_public_api_no_aux() {
    end_to_end_public_api(false, false);
  }

  #[test]
  fn end_to_end_public_api_with_aux() {
    end_to_end_public_api(true, false);
  }

  #[test]
  fn end_to_end_public_api_no_aux_with_errors() {
    end_to_end_public_api(false, true);
  }

  #[test]
  fn end_to_end_public_api_with_aux_with_errors() {
    end_to_end_public_api(true, true);
  }

  fn end_to_end_public_api(include_aux: bool, incl_failures: bool) {
    let threshold: u32 = 10;
    let num_layers = 3;
    let epoch = 0u8;
    let random_fetcher = LocalFetcher::new();

    // Sampling client measurements
    let total_num_measurements = 7;
    let mut all_measurements = Vec::new();
    let mut counts = Vec::<u32>::new();

    // add complete measurements
    let hello = "hello".as_bytes().to_vec();
    let goodbye = "goodbye".as_bytes().to_vec();
    let dog = "dog".as_bytes().to_vec();
    let cat = "cat".as_bytes().to_vec();
    let germany = "germany".as_bytes().to_vec();
    let france = "france".as_bytes().to_vec();
    all_measurements.push(vec![hello.clone(), dog.clone(), germany.clone()]);
    counts.push(threshold + 2);
    all_measurements.push(vec![hello.clone(), hello.clone(), germany.clone()]);
    counts.push(threshold + 5);

    // add partial measurements
    all_measurements.push(vec![hello.clone(), cat.clone(), france.clone()]);
    counts.push(threshold - 2);
    all_measurements.push(vec![hello.clone(), germany, france.clone()]);
    counts.push(threshold - 1);
    all_measurements.push(vec![hello.clone(), hello.clone(), dog.clone()]);
    counts.push(threshold - 7);
    all_measurements.push(vec![hello.clone(), hello, cat]);
    counts.push(threshold - 8);

    // Add some input that does not satisfy threshold
    all_measurements.push(vec![goodbye, dog, france]);
    counts.push(threshold - 1);
    assert_eq!(all_measurements.len(), total_num_measurements);

    // combine all measurements together
    let measurements: Vec<Vec<Vec<u8>>> = (0..total_num_measurements)
      .map(|i| {
        (0..counts[i])
          .map(|_| all_measurements[i].to_vec())
          .collect()
      })
      .fold(Vec::new(), |acc, r: Vec<Vec<Vec<u8>>>| {
        [acc, r.to_vec()].concat()
      });

    // generate client_messages
    let mut aux = vec![];
    if include_aux {
      let json_data = r#"
          {
            "score": 98.7,
            "tagline": "some word",
            "other_data": [
            "something",
            "else"
            ]
          }"#;
      aux = json_data.as_bytes().to_vec();
    }
    let mut client_messages: Vec<Vec<u8>> = measurements
      .iter()
      .map(|m| {
        let rrs = client::prepare_measurement(m, epoch).unwrap();
        let req = client::construct_randomness_request(&rrs);

        let req_slice_vec: Vec<&[u8]> =
          req.iter().map(|v| v.as_slice()).collect();
        let resp = random_fetcher.eval(&req_slice_vec, epoch).unwrap();
        let (points_slice_vec, proofs_slice_vec) =
          get_eval_output_slice_vecs(&resp);

        client::construct_message(
          &points_slice_vec,
          Some(&proofs_slice_vec),
          &rrs,
          &Some(PPOPRF_SERVER.get_public_key()),
          &aux,
          threshold,
        )
        .unwrap()
      })
      .collect();

    if incl_failures {
      // Include a single message threshold times. This will cause
      // the server to think that a value should be revealed, but
      // because the shares are identical a failure should occur.
      let rrs = client::prepare_measurement(
        &[
          "some".as_bytes().to_vec(),
          "bad".as_bytes().to_vec(),
          "input".as_bytes().to_vec(),
        ],
        epoch,
      )
      .unwrap();
      let req = client::construct_randomness_request(&rrs);

      let req_slice_vec: Vec<&[u8]> =
        req.iter().map(|v| v.as_slice()).collect();
      let resp = random_fetcher.eval(&req_slice_vec, epoch).unwrap();
      let (points_slice_vec, proofs_slice_vec) =
        get_eval_output_slice_vecs(&resp);

      let msg = client::construct_message(
        &points_slice_vec,
        Some(&proofs_slice_vec),
        &rrs,
        &Some(PPOPRF_SERVER.get_public_key()),
        &aux,
        threshold,
      )
      .unwrap();
      for _ in 0..threshold {
        client_messages.push(msg.clone());
      }

      // Include a client message that cannot be deserialized
      client_messages.push("some_bad_message".as_bytes().to_vec());
    }

    // server retrieve outputs
    let agg_res =
      server::aggregate(&client_messages, threshold, epoch, num_layers);

    // check outputs
    let outputs = agg_res.outputs();
    assert_eq!(outputs.len(), 2);
    if incl_failures {
      assert_eq!(agg_res.num_recovery_errors(), threshold as usize);
      assert_eq!(agg_res.num_serde_errors(), 1);
    } else {
      assert_eq!(agg_res.num_recovery_errors(), 0);
      assert_eq!(agg_res.num_serde_errors(), 0);
    }
    for output in outputs.iter() {
      let values: Vec<String> = output.value();
      let occurrences = output.occurrences();
      let mut expected_occurrences = None;
      if values.is_empty() {
        expected_occurrences = Some((threshold - 1) as usize);
      }
      if let Some(expected_occurrences) = expected_occurrences {
        assert_eq!(occurrences, expected_occurrences);
      }

      if include_aux && !values.is_empty() {
        if let Some(expected_occurrences) = expected_occurrences {
          assert_eq!(output.auxiliary_data().len(), expected_occurrences);
        }
        // all aux is the same for now
        let decoded = output.auxiliary_data()[0].clone();
        let object: Value = serde_json::from_slice(&decoded).unwrap();
        assert!(
          (object["score"].as_f64().unwrap() - 98.7).abs() < f64::EPSILON
        );
        assert_eq!(object["tagline"], "some word");
        assert_eq!(object["other_data"][0], "something");
        assert_eq!(object["other_data"][1], "else");
      }
    }
  }
}
