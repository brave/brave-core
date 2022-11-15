use std::cmp;
use std::time::Duration;

use async_trait::async_trait;
use futures_retry::{ErrorHandler, RetryPolicy};
use rand::{rngs::OsRng, Rng};
use serde_json::{to_string_pretty, Value};
use tracing::{debug, event, span, Level, instrument};

pub use http;
use http::{Request, Response};

use crate::errors::*;
use crate::models::APIError;
use crate::sdk::SDK;

static BASE_DELAY_MS: u64 = 250;
static MAX_DELAY_MS: u64 = 10000;

/// Default mapping of server response codes to be used after explicitly handling known response codes
impl From<http::Response<Vec<u8>>> for InternalError {
    fn from(resp: http::Response<Vec<u8>>) -> Self {
        event!(Level::DEBUG, "coming from response to internal error");
        let body = resp.body();

        let app_err: APIError = serde_json::from_slice(body).unwrap_or(APIError {
            code: 0,
            message: "unknown".to_string(),
            error_code: "".to_string(),
            data: Value::Null,
        });

        match resp.status() {
            http::StatusCode::TOO_MANY_REQUESTS => {
                InternalError::RetryLater(delay_from_response(&resp))
            }
            status if status.is_client_error() => InternalError::BadRequest(app_err),
            status if status.is_server_error() => InternalError::InternalServer(app_err),
            _ => InternalError::UnhandledStatus(app_err),
        }
    }
}

/// An request handler that counts attempts.
pub(crate) struct HttpHandler<'a, D, U> {
    max_attempts: usize,
    display_name: D,
    client: &'a U,
}

impl<'a, D, U> HttpHandler<'a, D, U> {
    pub fn new(max_attempts: usize, display_name: D, client: &'a U) -> Self {
        HttpHandler { client, max_attempts, display_name }
    }
}

impl<'a, D, U> ErrorHandler<InternalError> for HttpHandler<'a, D, U>
where
    D: ::std::fmt::Display,
    U: HTTPClient,
{
    type OutError = InternalError;

    fn handle(&mut self, current_attempt: usize, e: InternalError) -> RetryPolicy<InternalError> {
        let mut rng = OsRng;

        if current_attempt > self.max_attempts {
            event!(
                Level::DEBUG,
                max_attempts = self.max_attempts,
                "{}, all attempts failed",
                self.display_name,
            );

            return RetryPolicy::ForwardError(e);
        }
        event!(
            Level::DEBUG,
            current_attempt = current_attempt,
            max_attempts = self.max_attempts,
            "{} attempt failed",
            self.display_name,
        );

        let delay_ms = match e {
            InternalError::RetryLater(None)
            | InternalError::RequestFailed
            | InternalError::InternalServer(_)
            | InternalError::InvalidResponse(_) => {
                // Default to an exponential backoff with jitter along the full range
                // https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/
                rng.gen_range(0, cmp::min(MAX_DELAY_MS, BASE_DELAY_MS * (1 << current_attempt)))
            }
            InternalError::RetryLater(Some(after)) => {
                let after_ms = (after.as_millis() as u64) + 1;
                event!(
                    Level::DEBUG,
                    after_ms =?after_ms,
                    "inside RetryLater - waiting for after_ms",
                );

                // If the delay is more than is allowed by our maximum delay, return without retry
                if after_ms > MAX_DELAY_MS {
                    return RetryPolicy::ForwardError(e);
                }

                // If the server instructed us with a specific delay, delay for at least that long
                // while incorporating some random delay based on our current attempt
                cmp::min(
                    after_ms + rng.gen_range(0, BASE_DELAY_MS * (1 << current_attempt)),
                    MAX_DELAY_MS,
                )
            }
            _ => return RetryPolicy::ForwardError(e),
        };

        // Schedule a wakeup if needed
        self.client.schedule_wakeup(delay_ms);

        RetryPolicy::WaitRetry(Duration::from_millis(delay_ms))
    }
}

pub fn clone_resp(resp: &Response<Vec<u8>>) -> Response<Vec<u8>> {
    let mut copy = http::response::Builder::new();
    if let Some(headers) = copy.headers_mut() {
        headers.extend(resp.headers().clone())
    }
    copy.status(resp.status())
        .version(resp.version())
        .body(resp.body().clone())
        .expect("by nature of this result, an invalid http request cannot be created. thus it should be safe to clone an existing request by recursively cloning it's component parts")
}

#[instrument]
pub fn delay_from_response<T: std::fmt::Debug>(resp: &http::Response<T>) -> Option<Duration> {
    resp.headers().get(http::header::RETRY_AFTER).and_then(|value| {
        let parsed_retry_delay = value.to_str().ok().and_then(
            |value| value.trim().parse::<u64>().ok().map(Duration::from_secs));
        event!(
            Level::DEBUG,
            parsed_retry_delay =?parsed_retry_delay,
            "parsed_retry_delay in seconds",
        );
        parsed_retry_delay
    })
}

impl<U> SDK<U> {
    pub async fn fetch(&self, req: Request<Vec<u8>>) -> Result<Response<Vec<u8>>, InternalError>
    where
        U: HTTPClient,
    {
        let method = req.method().clone();
        let uri = req.uri().clone();

        let (parts, body) = req.into_parts();

        let span = span!(
            Level::DEBUG,
            "fetch",
            req.parts = ?parts,
        );
        let _guard = span.enter();

        let v: Value = serde_json::from_slice(&body).unwrap_or(Value::Null);
        event!(
            Level::DEBUG,
            req.body = %to_string_pretty(&v).unwrap(),
            "attempting request",
        );
        let req = Request::from_parts(parts, body);

        let resp = match self.lookup_cached_response(&method, &uri)? {
            Some(resp) => {
                debug!("local cache hit");

                resp
            }
            None => {
                let resp = self.client.execute(req).await?;

                self.cache_request(&method, &uri, &resp)?;

                resp
            }
        };

        let (parts, body) = resp.into_parts();
        let v: Value = serde_json::from_slice(&body).unwrap_or(Value::Null);
        event!(
            Level::DEBUG,
            resp.parts = ?parts,
            resp.body = %to_string_pretty(&v).unwrap(),
            "recieved response",
        );
        Ok(Response::from_parts(parts, body))
    }
}

#[async_trait(?Send)]
pub trait HTTPClient {
    async fn execute(
        &self,
        req: http::Request<Vec<u8>>,
    ) -> Result<Response<Vec<u8>>, InternalError>;
    fn schedule_wakeup(&self, delay_ms: u64);
    fn get_cookie(&self, key: &str) -> Option<String>;
    fn set_cookie(&self, value: &str);
}
