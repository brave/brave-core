//! Define interations with the randomness server OPRF
use crate::consts::RANDOMNESS_LEN;
use crate::format::RandomnessSampling;
use crate::Error;
use ppoprf::ppoprf;

pub fn process_randomness_response(
  points: &[ppoprf::Point],
  resp_points: &[&[u8]],
  resp_proofs: Option<&[&[u8]]>,
) -> Result<Vec<ppoprf::Evaluation>, Error> {
  let results = resp_points
    .iter()
    .enumerate()
    .map(|(i, v)| {
      let proof = match resp_proofs {
        Some(proofs) => {
          let data = proofs.get(i).ok_or(Error::ProofMissing)?;
          Some(
            ppoprf::ProofDLEQ::load_from_bincode(data)
              .map_err(|e| Error::Serialization(e.to_string()))?,
          )
        }
        None => None,
      };
      Ok(ppoprf::Evaluation {
        output: ppoprf::Point::from(*v),
        proof,
      })
    })
    .collect::<Result<Vec<ppoprf::Evaluation>, Error>>()?;
  if resp_points.len() != points.len() {
    return Err(Error::RandomnessSampling(format!(
      "Server returned {} results, but expected {}.",
      resp_points.len(),
      points.len(),
    )));
  }
  Ok(results)
}

/// `RequestState` for building and building all state associated with
/// randomness requests
pub struct RequestState {
  rsf: RandomnessSampling,
  req_points: Vec<ppoprf::Point>,
  blinds: Vec<ppoprf::CurveScalar>,
}
impl RequestState {
  pub fn new(rsf: RandomnessSampling) -> Self {
    let measurements = rsf.input();
    let mut req_points = Vec::with_capacity(measurements.len());
    let mut blinds = Vec::with_capacity(measurements.len());
    for x in measurements {
      let (p, r) = ppoprf::Client::blind(x);
      req_points.push(p);
      blinds.push(r);
    }

    Self {
      rsf,
      req_points,
      blinds,
    }
  }

  // Finalize randomness outputs
  pub fn finalize_response(
    &self,
    results: &[ppoprf::Evaluation],
    public_key: &Option<ppoprf::ServerPublicKey>,
  ) -> Result<Vec<[u8; RANDOMNESS_LEN]>, Error> {
    let mut buf = [0u8; RANDOMNESS_LEN];
    let mut rand_out = Vec::with_capacity(results.len());
    for (i, result) in results.iter().enumerate() {
      let blinded_point = &self.blinded_points()[i];

      // if a server public key was specified, attempt to verify the
      // result of the randomness
      if let Some(pk) = public_key {
        if result.proof.is_some()
          && !ppoprf::Client::verify(pk, blinded_point, result, self.epoch())
        {
          return Err(Error::RandomnessSampling(
            "Client ZK proof verification failed".into(),
          ));
        }
      }

      // unblind and finalize randomness output
      let unblinded = ppoprf::Client::unblind(&result.output, &self.blinds[i]);
      ppoprf::Client::finalize(
        &self.measurement(i),
        self.epoch(),
        &unblinded,
        &mut buf,
      );
      rand_out.push(buf);
    }
    Ok(rand_out)
  }

  pub fn blinded_points(&self) -> &[ppoprf::Point] {
    &self.req_points
  }

  fn measurement(&self, idx: usize) -> Vec<u8> {
    let mut result = Vec::new();
    for m in &self.rsf.input()[..(idx + 1)] {
      result.extend(m);
    }
    result
  }

  fn epoch(&self) -> u8 {
    self.rsf.epoch()
  }

  pub fn rsf(&self) -> &RandomnessSampling {
    &self.rsf
  }
}

pub mod testing {
  //! This module provides a mock `LocalFetcher` for locally fetching
  //! randomness during tests.
  //!
  //! IMPORTANT: the local fetching method should only be used for
  //! tests!
  use super::ppoprf::Server as PPOPRFServer;
  use super::*;

  // This is a hack to make sure that we always use the same key for
  // PPOPRF evaluations
  lazy_static::lazy_static! {
    pub static ref PPOPRF_SERVER: PPOPRFServer = PPOPRFServer::new((0u8..=7).collect()).unwrap();
  }

  /// The `LocalFetcher` provides a test implementation of the fetching
  /// interface, using a local instantiation of the PPOPRF.
  ///
  /// NOT TO BE USED IN PRODUCTION
  pub struct LocalFetcher {
    ppoprf_server: PPOPRFServer,
  }

  #[derive(Default)]
  pub struct LocalFetcherResponse {
    pub serialized_points: Vec<Vec<u8>>,
    pub serialized_proofs: Vec<Vec<u8>>,
  }

  impl LocalFetcher {
    pub fn new() -> Self {
      Self {
        ppoprf_server: PPOPRF_SERVER.clone(),
      }
    }

    pub fn eval(
      &self,
      serialized_points: &[&[u8]],
      epoch: u8,
    ) -> Result<LocalFetcherResponse, Error> {
      // Create a mock response based on expected PPOPRF functionality
      let mut result: LocalFetcherResponse = Default::default();
      for serialized_point in serialized_points.iter() {
        let point = ppoprf::Point::from(*serialized_point);
        match self.ppoprf_server.eval(&point, epoch, true) {
          Err(e) => return Err(Error::RandomnessSampling(e.to_string())),
          Ok(eval) => {
            result
              .serialized_points
              .push(eval.output.as_bytes().to_vec());
            result.serialized_proofs.push(
              eval
                .proof
                .unwrap()
                .serialize_to_bincode()
                .map_err(|e| Error::Serialization(e.to_string()))?,
            );
          }
        };
      }
      Ok(result)
    }

    pub fn get_server(&self) -> &PPOPRFServer {
      &self.ppoprf_server
    }
  }
  impl Default for LocalFetcher {
    fn default() -> Self {
      LocalFetcher::new()
    }
  }
}
