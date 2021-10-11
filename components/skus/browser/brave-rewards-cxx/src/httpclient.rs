use async_trait::async_trait;
use brave_rewards::{errors, http::http, HTTPClient};
use futures::channel::oneshot;

use tracing::debug;

use crate::{ffi, NativeClient, LOCAL_POOL};

pub struct HttpRoundtripContext(
    oneshot::Sender<Result<http::Response<Vec<u8>>, errors::InternalError>>,
);

pub async fn execute_request(
    req: ffi::HttpRequest,
) -> Result<http::Response<Vec<u8>>, errors::InternalError> {
    let (tx, rx) = oneshot::channel();
    let context = Box::new(HttpRoundtripContext(tx));

    ffi::shim_executeRequest(
        &req,
        |context, resp| {
            let resp = match resp.result {
                ffi::RewardsResult::Ok => {
                    // FIXME implement from

                    let mut response = http::Response::builder();

                    response.status(resp.return_code);
                    for header in resp.headers {
                        let header = header.to_string();
                        // header: value
                        let idx = header.find(':').unwrap();
                        let (key, value) = header.split_at(idx);
                        let value = value.get(1..).unwrap();

                        response.header(key, value);
                    }

                    Ok(response.body(resp.body.iter().cloned().collect()).unwrap())
                }
                _ => Err(errors::InternalError::RequestFailed),
            };

            let _ = context.0.send(resp);

            LOCAL_POOL.with(|pool| {
                if let Ok(mut pool) = pool.try_borrow_mut() {
                    pool.run_until_stalled();
                }
            });
        },
        context,
    );

    rx.await.unwrap()
}

#[async_trait(?Send)]
impl HTTPClient for NativeClient {
    async fn execute(
        req: http::Request<Vec<u8>>,
    ) -> Result<http::Response<Vec<u8>>, errors::InternalError> {
        // FIXME implement from

        let url = req.uri().to_string();
        let method = req.method().to_string();
        let mut headers: Vec<String> = Vec::new();

        for (key, value) in req.headers().iter() {
            let header = format!("{}: {}", key.as_str(), value.to_str().unwrap());
            headers.push(header);
        }

        let body = req.body().to_vec();
        execute_request(ffi::HttpRequest {
            url,
            method,
            headers,
            body,
        })
        .await
    }

    fn schedule_wakeup(delay_ms: u64) {
        ffi::shim_scheduleWakeup(delay_ms, || {
            debug!("woke up!");
            LOCAL_POOL.with(|pool| {
                if let Ok(mut pool) = pool.try_borrow_mut() {
                    pool.run_until_stalled();
                }
            });
        })
    }

    fn get_cookie(_key: &str) -> Option<String> {
        unimplemented!();
    }

    fn set_cookie(_value: &str) {
        unimplemented!();
    }
}
