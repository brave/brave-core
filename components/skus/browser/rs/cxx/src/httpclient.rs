use std::convert::{TryFrom, TryInto};
use std::ops::Deref;

use async_trait::async_trait;
use futures::channel::oneshot;
use skus::{errors, http::http, HTTPClient};

use tracing::debug;

use crate::{ffi, NativeClient};

pub struct HttpRoundtripContext {
    tx: oneshot::Sender<Result<http::Response<Vec<u8>>, errors::InternalError>>,
    client: NativeClient,
}

pub struct WakeupContext {
    client: NativeClient,
}

impl TryFrom<http::Request<Vec<u8>>> for ffi::HttpRequest {
    type Error = errors::InternalError;

    fn try_from(req: http::Request<Vec<u8>>) -> Result<Self, Self::Error> {
        let url = req.uri().to_string();
        let method = req.method().to_string();
        let mut headers: Vec<String> = Vec::new();

        for (key, value) in req.headers().iter() {
            let value = value
                .to_str()
                .map_err(|_| errors::InternalError::UnhandledVariant)?;
            let header = format!("{}: {}", key.as_str(), value);
            headers.push(header);
        }

        let body = req.body().to_vec();

        Ok(ffi::HttpRequest {
            url,
            method,
            headers,
            body,
        })
    }
}

impl From<ffi::HttpResponse<'_>> for Result<http::Response<Vec<u8>>, errors::InternalError> {
    fn from(resp: ffi::HttpResponse<'_>) -> Self {
        match resp.result {
            ffi::SkusResult::Ok => {
                let mut response = http::Response::builder();

                response.status(resp.return_code);
                for header in resp.headers {
                    let header = header.to_string();
                    // header: value
                    let idx = header
                        .find(':')
                        .expect("caller must pass headers as `KEY: VALUE`");
                    let (key, value) = header.split_at(idx);
                    let value = value
                        .get(1..)
                        .expect("caller must pass headers as `KEY: VALUE`");

                    response.header(key, value);
                }

                Ok(response
                    .body(resp.body.iter().cloned().collect())
                    .expect("caller must pass valid body even if zero length"))
            }
            _ => Err(errors::InternalError::RequestFailed),
        }
    }
}

impl NativeClient {
    pub async fn execute_request(
        &self,
        req: ffi::HttpRequest,
    ) -> Result<http::Response<Vec<u8>>, errors::InternalError> {
        let (tx, rx) = oneshot::channel();
        let context = Box::new(HttpRoundtripContext {
            tx,
            client: self.clone(),
        });

        let fetcher = ffi::shim_executeRequest(
            &self
                .ctx
                .try_borrow()
                .map_err(|_| errors::InternalError::BorrowFailed)?
                .deref()
                .deref()
                .ctx,
            &req,
            |context, resp| {
                let _ = context.tx.send(resp.into());

                context.client.try_run_until_stalled();
            },
            context,
        );

        let ret = rx
            .await
            .map_err(|_| errors::InternalError::FutureCancelled)?;
        drop(fetcher);
        ret
    }
}

#[async_trait(?Send)]
impl HTTPClient for NativeClient {
    async fn execute(
        &self,
        req: http::Request<Vec<u8>>,
    ) -> Result<http::Response<Vec<u8>>, errors::InternalError> {
        self.execute_request(req.try_into()?).await
    }

    fn schedule_wakeup(&self, delay_ms: u64) {
        ffi::shim_scheduleWakeup(
            delay_ms,
            |context| {
                debug!("woke up!");
                context.client.try_run_until_stalled();
            },
            Box::new(WakeupContext {
                client: self.clone(),
            }),
        )
    }

    fn get_cookie(&self, _key: &str) -> Option<String> {
        unimplemented!();
    }

    fn set_cookie(&self, _value: &str) {
        unimplemented!();
    }
}
