use std::env;

use http_body_util::{BodyExt, Empty};
use hyper::Request;
use hyper_util::client::legacy::{Client, connect::HttpConnector};
use tracing::Instrument;

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let _tracing = tracing_subscriber::fmt()
        .with_max_level(tracing::Level::TRACE)
        .init();
    tracing::info!("tracing subscriber initialized");

    let url = match env::args().nth(1) {
        Some(url) => url,
        None => {
            tracing::error!("Usage: client <url>");
            return Ok(());
        }
    };

    // HTTPS requires picking a TLS implementation, so give a better
    // warning if the user tries to request an 'https' URL.
    let url = url
        .parse::<hyper::Uri>()
        .inspect_err(|err| tracing::error!(%err, "failed to parse url"))?;
    if url.scheme_str() != Some("http") {
        tracing::error!("This example only works with 'http' URLs.");
        return Ok(());
    }
    tracing::info!(%url, "parsed URL");

    let executor = {
        // Propagate spans to spawned tasks.
        use hyper_util::rt::{CurrentSpanExecutor, TokioExecutor};
        let tokio = TokioExecutor::new();
        CurrentSpanExecutor::new(tokio)
    };

    let client = Client::builder(executor).build(HttpConnector::new());

    let req = Request::builder()
        .uri(url.clone())
        .body(Empty::<bytes::Bytes>::new())?;

    let span = tracing::info_span!("sending request", %url);
    let resp = client.request(req).instrument(span).await?;

    let (resp, body) = resp.into_parts();
    let body = body.collect().await.unwrap().to_bytes().to_vec();
    let body = String::from_utf8(body).unwrap();

    tracing::info!(
        ?resp.version,
        %resp.status,
        resp.body = %body,
        "received response",
    );

    Ok(())
}
