//! Example ppoprf randomness web service.
//!
//! This wraps the ppoprf evaluation function in an Actix-Web
//! service application so it can be accessed over https.
//!
//! To verify the example works, start the server in one terminal:
//! ```sh
//! cargo run --example server
//! ```
//!
//! In another terminal, verify the GET method returns a service
//! identification:
//! ```sh
//! curl --silent localhost:8080
//! ```
//!
//! Finally verify the POST method returns an altered point:
//! ```sh
//! curl --silent localhost:8080 \
//!     --header 'Content-Type: application/json' \
//!     --data '{"name":"STAR", "points": [
//!         [ 226, 242, 174, 10, 106, 188, 78, 113,
//!           168, 132, 169, 97, 197, 0, 81, 95,
//!           88, 227, 11, 106, 165, 130, 221, 141,
//!           182, 166, 89, 69, 224, 141, 45, 118 ]
//!       ]}'
//! ```

use dotenvy::dotenv;
use env_logger::Env;
use log::{info, warn};
use serde::{Deserialize, Serialize};
use warp::http::StatusCode;
use warp::Filter;

use std::collections::VecDeque;
use std::convert::Infallible;
use std::env;
use std::sync::{Arc, RwLock};

use ppoprf::ppoprf;

const DEFAULT_EPOCH_DURATION: u64 = 5;
const DEFAULT_MDS: &str = "116;117;118;119;120";
const EPOCH_DURATION_ENV_KEY: &str = "EPOCH_DURATION";
const MDS_ENV_KEY: &str = "METADATA_TAGS";

/// Shared randomness server state
struct ServerState {
  prf_server: ppoprf::Server,
  active_md: u8,
  future_mds: VecDeque<u8>,
}
/// Wrapped state for access within service tasks
/// We use an RWLock to handle the infrequent puncture events.
/// Only read access is necessary to answer queries.
type State = Arc<RwLock<ServerState>>;

/// Decorator to clone state into a warp::Filter.
fn with_state(
  state: State,
) -> impl Filter<Extract = (State,), Error = Infallible> + Clone {
  warp::any().map(move || Arc::clone(&state))
}

/// PPOPRF evaluation request from the client
#[derive(Deserialize)]
struct EvalRequest {
  name: String,
  points: Vec<ppoprf::Point>,
}

/// PPOPRF evaluation result returned by the server
#[derive(Serialize)]
struct EvalResponse {
  name: String,
  results: Vec<ppoprf::Evaluation>,
}

#[derive(Serialize)]
struct ServerErrorResponse {
  error: String,
}

/// Simple string to identify the server.
fn help() -> &'static str {
  concat!(
    "STAR protocol randomness server.\n",
    "See https://arxiv.org/abs/2109.10074 for more information.\n"
  )
}

/// Process a PPOPRF evaluation request from the client.
async fn eval(
  data: EvalRequest,
  state: State,
) -> Result<impl warp::Reply, Infallible> {
  let state = state.read().unwrap();

  // Pass each point from the client through the ppoprf.
  let result: Result<Vec<ppoprf::Evaluation>, ppoprf::PPRFError> = data
    .points
    .iter()
    .map(|p| state.prf_server.eval(p, state.active_md, false))
    .collect();

  // Format the results.
  match result {
    Ok(results) => Ok(warp::reply::with_status(
      warp::reply::json(&EvalResponse {
        name: data.name,
        results,
      }),
      StatusCode::OK,
    )),
    Err(error) => Ok(warp::reply::with_status(
      warp::reply::json(&ServerErrorResponse {
        error: format!("{error}"),
      }),
      StatusCode::INTERNAL_SERVER_ERROR,
    )),
  }
}

#[tokio::main]
async fn main() {
  dotenv().ok();

  let host = "localhost";
  let port = 8080;

  env_logger::init_from_env(Env::default().default_filter_or("info"));
  info!("Server configured on {host} port {port}");

  // Metadata tags marking each randomness epoch.
  let mds_str = match env::var(MDS_ENV_KEY) {
    Ok(val) => val,
    Err(_) => {
      info!(
        "{} env var not defined, using default: {}",
        MDS_ENV_KEY, DEFAULT_MDS
      );
      DEFAULT_MDS.to_string()
    }
  };
  let mds: Vec<u8> = mds_str
    .split(';')
    .map(|y| {
      y.parse().expect(
        "Could not parse metadata tags. Must contain 8-bit unsigned values!",
      )
    })
    .collect();

  // Time interval between puncturing each successive md.
  let epoch =
    std::time::Duration::from_secs(match env::var(EPOCH_DURATION_ENV_KEY) {
      Ok(val) => val.parse().expect(
        "Could not parse epoch duration. It must be a positive number!",
      ),
      Err(_) => {
        info!(
          "{} env var not defined, using default: {} seconds",
          EPOCH_DURATION_ENV_KEY, DEFAULT_EPOCH_DURATION
        );
        DEFAULT_EPOCH_DURATION
      }
    });

  // Initialize shared server state.
  let state = Arc::new(RwLock::new(ServerState {
    prf_server: ppoprf::Server::new(mds.clone()).unwrap(),
    active_md: mds[0],
    future_mds: VecDeque::from(mds[1..].to_vec()),
  }));
  info!("PPOPRF initialized with epoch metadata tags {:?}", &mds);

  // Spawn a background task.
  let background_state = state.clone();
  tokio::spawn(async move {
    info!(
      "Background task will rotate epoch every {} seconds",
      epoch.as_secs()
    );
    // Loop over the list of configured epoch metadata tags.
    for &md in &mds {
      info!(
        "Epoch tag now '{:?}'; next rotation in {} seconds",
        md,
        epoch.as_secs()
      );
      // Wait for the end of an epoch.
      tokio::time::sleep(epoch).await;
      if let Ok(mut state) = background_state.write() {
        info!("Epoch rotation: puncturing '{:?}'", md);
        state.prf_server.puncture(md).unwrap();
        let new_md = state.future_mds.pop_front().unwrap();
        state.active_md = new_md;
      }
    }
    warn!("All epoch tags punctured! No further evaluations possible.");
  });

  // Warp web server framework routes.
  let info = warp::get().map(help);
  let rand = warp::post()
    .and(warp::body::content_length_limit(8 * 1024))
    .and(warp::body::json())
    .and(with_state(state))
    .and_then(eval);
  let routes = rand.or(info);

  // Run server until exit.
  warp::serve(routes).run(([127, 0, 0, 1], 8080)).await;
}
