//! Example clieant for the ppoprf randomness web service.
//!
//! This tests and demonstrates the ppoprf evaluation function
//! in an Actix-Web service application by making example queries.
//!
//! To verify the example works, start the server in one terminal:
//! ```sh
//! cargo run --example server
//! ```
//!
//! In another terminal, launch the client:
//! ```sh
//! cargo run --example client
//! ```

use env_logger::Env;
use log::info;
use ppoprf::ppoprf;
use reqwest::blocking::Client as HttpClient;
use serde::{Deserialize, Serialize};

/// Fetch the server identification string.
///
/// Acts as a basic availability ping.
fn fetch_ident(url: &str) -> reqwest::Result<()> {
  let res = HttpClient::new().get(url).send()?;
  let status = res.status();
  let text = res.text()?;

  info!("{} - {}", status, text);

  Ok(())
}

/// Explicit query body.
#[derive(Serialize)]
struct Query {
  name: String,
  points: Vec<ppoprf::Point>,
}

/// Explicit response body.
#[derive(Deserialize)]
struct Response {
  name: String,
  results: Vec<ppoprf::Evaluation>,
}

/// Fetch randomness from the server.
///
/// Acts as a basic round-trip test.
fn fetch_randomness(url: &str) -> reqwest::Result<()> {
  // Construct a blinded message hash.
  let message = "ppoprf test client";
  let (blinded_message, r) = ppoprf::Client::blind(message.as_bytes());
  // Submit it to the server.
  let query = Query {
    name: "example client".into(),
    points: vec![blinded_message],
  };
  let res = HttpClient::new().post(url).json(&query).send()?;
  let status = res.status();
  let result = res.json::<Response>()?;
  let results = result.results;

  // We only submit one hash; confirm the server returned the same.
  info!(
    "{} {} - {} points returned",
    status,
    result.name,
    results.len()
  );
  assert_eq!(
    query.points.len(),
    results.len(),
    "Server returned a different number of points!"
  );
  assert_eq!(results.len(), 1, "Expected one point!");
  if let Some(result) = results.first() {
    // Unblind the message hash.
    let unblinded = ppoprf::Client::unblind(&result.output, &r);
    // Next we would finalize the unblinded point to produce
    // value needed for the next step of the Star protocol.
    //
    // This requires knowning the epoch metadata tag the server
    // used, which we don't currently coordinate, so skip this step.
    //let mut out = vec![0u8; ppoprf::COMPRESSED_POINT_LEN];
    //ppoprf::Client::finalize(message.as_bytes(), &md, &unblinded, &mut out);
    let point = base64::encode(unblinded.as_bytes());
    let proof = result.proof.is_some();
    let meta = if proof { " proof" } else { "" };
    info!("  {}{}", &point, &meta);
  }

  Ok(())
}

fn main() {
  let url = "http://localhost:8080";

  env_logger::init_from_env(Env::default().default_filter_or("info"));

  info!("Contacting server at {}", url);
  fetch_ident(url).unwrap();
  fetch_randomness(url).unwrap();
}
