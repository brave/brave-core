// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

//! HTTP for the SKU SDK, on top of `brave.network.mojom.SimpleUrlLoader`.
//!
//! Requests go over Mojo to a `brave::network::SimpleUrlLoaderService` owned by
//! `SkusContextImpl`, which drives `network::SimpleURLLoader`. This replaced a
//! bespoke cxx shim (`shim_executeRequest`) around
//! `api_request_helper::APIRequestHelper`; the request options below are chosen
//! to reproduce what that helper did, so behaviour is unchanged.

chromium::import! {
    "//brave/services/network/public/rust:brave_url_loader";
}

use std::cell::RefCell;
use std::collections::HashMap;
use std::convert::TryFrom;
use std::rc::Rc;

use async_trait::async_trait;
use brave_url_loader::{RequestBuilder, UrlLoader};
use skus::{
    errors::{DebugUnwrap, InternalError},
    http::http,
    HTTPClient,
};

use log::debug;

use crate::{ffi, NativeClient, NativeClientExecutor};

/// `api_request_helper` retried exactly once, and only on a network change.
/// Retrying on a network change is safe even for non-idempotent requests
/// because the original could not have been delivered.
const MAX_RETRIES_ON_NETWORK_CHANGE: u32 = 1;

/// Every SKU SDK request body is JSON.
const REQUEST_CONTENT_TYPE: &str = "application/json";

pub struct WakeupContext {
    client: NativeClient,
}

thread_local! {
    /// Executors for the SDKs living on this thread, keyed by loader id.
    ///
    /// Mojo response callbacks must be `Send`, but `NativeClientExecutor` is
    /// `Rc`-based and therefore is not. The waker closure captures only a `u64`
    /// and looks the executor up here, which is sound because a `UrlLoader`
    /// wakes only on the sequence it was bound to, and that is the same
    /// single-threaded sequence the SDK runs on.
    ///
    /// A map rather than a single slot because `SkusServiceImpl` may run one
    /// SDK per environment, all on the same thread.
    static EXECUTORS: RefCell<HashMap<u64, Rc<RefCell<NativeClientExecutor>>>> =
        RefCell::new(HashMap::new());
}

thread_local! {
    static NEXT_LOADER_ID: RefCell<u64> = const { RefCell::new(0) };
}

/// Binds a Mojo pipe from `ctx` and registers `executor` to be polled whenever
/// a response arrives on it.
pub fn create_url_loader(
    ctx: &mut cxx::UniquePtr<ffi::SkusContext>,
    executor: Rc<RefCell<NativeClientExecutor>>,
) -> Option<UrlLoader> {
    let pipe = ffi::shim_createUrlLoader(ctx.pin_mut());
    let mut loader = UrlLoader::from_cpp_pipe_handle(pipe)?;

    let id = NEXT_LOADER_ID.with(|next| {
        let mut next = next.borrow_mut();
        let id = *next;
        *next += 1;
        id
    });
    EXECUTORS.with(|executors| executors.borrow_mut().insert(id, executor));

    loader.set_waker(move || wake_executor(id));
    Some(loader)
}

/// Advances the SDK's future pool after a response lands.
///
/// Nothing else polls it: the pool is only ever driven by explicit
/// `run_until_stalled` calls.
fn wake_executor(id: u64) {
    let executor = EXECUTORS.with(|executors| executors.borrow().get(&id).cloned());
    let Some(executor) = executor else {
        // The SDK for this loader has already gone away.
        return;
    };
    // `try_borrow_mut` because the waker can fire while the pool is already
    // being run, in which case the in-progress run will observe the result.
    if let Ok(mut executor) = executor.try_borrow_mut() {
        executor.try_run_until_stalled();
    }
}

fn method_from(method: &http::Method) -> Result<brave_url_loader::RawHttpMethod, InternalError> {
    use brave_url_loader::RawHttpMethod;
    Ok(match *method {
        http::Method::GET => RawHttpMethod::kGet,
        http::Method::HEAD => RawHttpMethod::kHead,
        http::Method::POST => RawHttpMethod::kPost,
        http::Method::PUT => RawHttpMethod::kPut,
        http::Method::PATCH => RawHttpMethod::kPatch,
        http::Method::DELETE => RawHttpMethod::kDelete,
        _ => return Err(InternalError::UnhandledVariant),
    })
}

fn build_request(
    req: http::Request<Vec<u8>>,
) -> Result<brave_url_loader::RawDownloadRequest, InternalError> {
    let method = method_from(req.method())?;
    let url = req.uri().to_string();

    let body = req.body().to_vec();
    // Matching api_request_helper, which attached an upload only for a
    // non-empty payload. Attaching an empty body would set a Content-Type and
    // Content-Length on requests that previously had neither.
    let mut builder = if body.is_empty() {
        RequestBuilder::with_method(url, method)
    } else {
        RequestBuilder::with_body(url, method, body, REQUEST_CONTENT_TYPE)
    };

    for (name, value) in req.headers().iter() {
        builder = builder.header(name.as_str(), value.as_bytes());
    }

    Ok(builder
        // api_request_helper always allowed HTTP error results and left the
        // decision to the caller; `success` below reproduces its test.
        .allow_http_error_results(true)
        .retry_on_network_change(MAX_RETRIES_ON_NETWORK_CHANGE)
        // The SDK parses these bodies as JSON. Sanitizing service-side keeps
        // the previous guarantee that a malformed body never reaches here.
        .sanitize_json_response(true)
        .build())
}

fn build_response(
    result: brave_url_loader::RawDownloadResult,
) -> Result<http::Response<Vec<u8>>, InternalError> {
    // api_request_helper treated "we got any HTTP response at all" as success
    // and let the SDK interpret the status; anything else was a failed request.
    let status = u16::try_from(result.response_code).map_err(|_| InternalError::RequestFailed)?;
    if !(100..=599).contains(&status) {
        return Err(InternalError::RequestFailed);
    }

    let mut response = http::Response::builder().status(status);
    for header in &result.headers {
        let name = http::HeaderName::try_from(header.name.as_str()).map_err(|_| {
            InternalError::InvalidCall(
                concat!(
                    "must pass a valid HTTP header name,",
                    " i.e. ASCII charaters with no whitespace",
                    " or separator punctuation.",
                    "\nSee RFC 9110 section 5.6.2 for details.",
                )
                .to_string(),
            )
        })?;
        let value = http::HeaderValue::from_bytes(&header.value).map_err(|_| {
            InternalError::InvalidCall(
                "must pass a valid (ASCII-printable) HTTP header value".to_string(),
            )
        })?;
        if let Some(headers) = response.headers_mut() {
            headers.insert(name, value);
        }
    }

    response.body(result.body).map_err(|e| InternalError::InvalidCall(e.to_string())).debug_unwrap()
}

impl NativeClient {
    pub async fn execute_request(
        &self,
        req: http::Request<Vec<u8>>,
    ) -> Result<http::Response<Vec<u8>>, InternalError> {
        let request = build_request(req)?;

        let pending = {
            let inner = self.inner.lock().await;
            let mut url_loader =
                inner.url_loader.try_borrow_mut().map_err(|_| InternalError::BorrowFailed)?;
            let url_loader = url_loader.as_mut().ok_or(InternalError::RequestFailed)?;
            url_loader.download_async(request)
        };

        // Dropping `pending` here (e.g. if this future is cancelled) closes the
        // request's Mojo cancellation token, which aborts the load. That
        // matches the previous shim, where dropping the fetcher destroyed its
        // APIRequestHelper.
        match pending.await {
            Ok(result) => build_response(result),
            Err(_) => {
                debug!("url loader disconnected before the response arrived");
                Err(InternalError::FutureCancelled)
            }
        }
    }
}

#[async_trait(?Send)]
impl HTTPClient for NativeClient {
    async fn execute(
        &self,
        req: http::Request<Vec<u8>>,
    ) -> Result<http::Response<Vec<u8>>, InternalError> {
        self.execute_request(req).await
    }

    fn schedule_wakeup(&self, delay_ms: u64) {
        ffi::shim_scheduleWakeup(
            delay_ms,
            |context| {
                debug!("woke up!");
                context.client.try_run_until_stalled();
            },
            Box::new(WakeupContext { client: self.clone() }),
        )
    }

    fn get_cookie(&self, _key: &str) -> Option<String> {
        unimplemented!();
    }

    fn set_cookie(&self, _value: &str) {
        unimplemented!();
    }
}
