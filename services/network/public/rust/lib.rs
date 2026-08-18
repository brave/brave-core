// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

//! A Rust client for `brave.network.mojom.SimpleUrlLoader`.
//!
//! This gives Rust code access to Chromium's network stack without a bespoke
//! cxx shim per component. The C++ side owns the `URLLoaderFactory`, the
//! traffic annotation, and `network::SimpleURLLoader`; Rust holds a `Remote`
//! and issues buffered round trips over it.
//!
//! Getting a handle: someone has to hand Rust the pipe. Until there are
//! existing C++/Rust Mojo pipes to receive it over, the consuming crate needs
//! a small `#[cxx::bridge]` that accepts a `ScopedMessagePipeHandleWrapper`
//! and calls [`UrlLoader::from_cpp_pipe_handle`].

chromium::import! {
    "//mojo/public/rust/bindings";
    "//mojo/public/rust/system";
    "//brave/services/network/public/mojom:simple_url_loader_rust";
}

use std::collections::HashMap;
use std::future::Future;
use std::pin::Pin;
use std::sync::{Arc, Mutex};
use std::task::{Context, Poll};

use bindings::remote::{PendingRemote, Remote};
use futures_channel::oneshot;
use simple_url_loader_rust::simple_url_loader::{
    DownloadRequest, DownloadResult, HttpHeader, HttpMethod, SimpleUrlLoader,
};
use system::message_pipe::MessageEndpoint;
use system::scoped_handle_interop::ScopedMessagePipeHandleWrapper;

pub use simple_url_loader_rust::simple_url_loader::{
    DownloadRequest as RawDownloadRequest, DownloadResult as RawDownloadResult,
    HttpHeader as RawHttpHeader, HttpMethod as RawHttpMethod,
};

/// The service went away before the request completed.
///
/// The request may or may not have reached the network, so retrying is not
/// necessarily safe for non-idempotent methods.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct Disconnected;

impl std::fmt::Display for Disconnected {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "the SimpleUrlLoader service disconnected")
    }
}

impl std::error::Error for Disconnected {}

/// Senders for in-flight `download_async` calls, plus a sticky disconnect flag.
///
/// The flag matters because the disconnect handler is `FnOnce` and so runs
/// exactly once. Without it, a request issued *after* the pipe already died
/// would register a sender that nothing will ever drop, and its future would
/// hang forever.
#[derive(Default)]
struct PendingState {
    senders: HashMap<u64, oneshot::Sender<DownloadResult>>,
    disconnected: bool,
}

type SharedPendingState = Arc<Mutex<PendingState>>;

/// Panicking on a poisoned mutex is correct here: it means a response callback
/// already panicked, and the map may be inconsistent.
fn lock(state: &SharedPendingState) -> std::sync::MutexGuard<'_, PendingState> {
    state.lock().expect("pending download state mutex poisoned")
}

/// A bound client for the `SimpleUrlLoader` interface.
///
/// Must be constructed and used on a sequence with a running task runner: the
/// underlying `Remote` schedules response processing there.
pub struct UrlLoader {
    remote: Remote<dyn SimpleUrlLoader>,
    state: SharedPendingState,
    next_id: u64,
}

impl UrlLoader {
    /// Binds an already-owned pipe endpoint to the current default sequence.
    pub fn from_message_endpoint(endpoint: MessageEndpoint) -> Self {
        let state = SharedPendingState::default();

        // A disconnect handler is mandatory rather than optional here. The Mojo
        // Rust bindings only remove a pending response callback when a response
        // actually arrives; nothing clears them on disconnect, so they live
        // until the whole `Remote` is dropped. Without this, a future awaiting
        // a request on a dead pipe would hang instead of failing.
        let state_on_disconnect = state.clone();
        let disconnect_handler = Box::new(move || {
            let mut state = lock(&state_on_disconnect);
            state.disconnected = true;
            // Dropping every sender resolves each pending future with
            // `Err(Disconnected)`.
            state.senders.clear();
        });

        let remote = PendingRemote::<dyn SimpleUrlLoader>::new(endpoint)
            .bind_with_options(None, Some(disconnect_handler));

        Self { remote, state, next_id: 0 }
    }

    /// Adopts a message pipe handed over from C++ and binds it to the current
    /// default sequence.
    ///
    /// Returns `None` if the handle was invalid (i.e. already moved from).
    pub fn from_cpp_pipe_handle(
        handle: cxx::UniquePtr<ScopedMessagePipeHandleWrapper>,
    ) -> Option<Self> {
        let endpoint = ScopedMessagePipeHandleWrapper::into_message_endpoint(handle)?;
        Some(Self::from_message_endpoint(endpoint))
    }

    /// Issues a request, resolving to the response.
    ///
    /// This is the API most callers want. The returned future borrows nothing,
    /// so several may be in flight at once; they complete independently and may
    /// complete out of order.
    ///
    /// The future is resolved by a task on the sequence this `UrlLoader` is
    /// bound to, so that sequence must be pumped for it to make progress.
    /// Integrating a Rust executor with a `SequencedTaskRunner` is the caller's
    /// responsibility.
    pub fn download_async(&mut self, request: DownloadRequest) -> PendingDownload {
        let (sender, receiver) = oneshot::channel();

        let id = self.next_id;
        // A u64 will not realistically wrap, but be explicit rather than relying
        // on release-mode wrapping if it ever did.
        self.next_id = self.next_id.wrapping_add(1);

        {
            let mut state = lock(&self.state);
            if state.disconnected {
                // Drop `sender` without registering it, so the returned future
                // resolves to `Err(Disconnected)` rather than hanging. Sending
                // on a dead pipe would otherwise never produce a response.
                return PendingDownload { receiver };
            }
            state.senders.insert(id, sender);
        }

        let state = self.state.clone();
        self.remote.Download(request, move |result| {
            // Absent only if disconnection already cleared the map, in which
            // case the future has already been resolved with an error.
            if let Some(sender) = lock(&state).senders.remove(&id) {
                // Fails only if the caller dropped the future, which is fine.
                let _ = sender.send(result);
            }
        });

        PendingDownload { receiver }
    }

    /// Issues a request, invoking `on_complete` on the sequence this
    /// `UrlLoader` was bound to.
    ///
    /// Prefer [`UrlLoader::download_async`]. This lower-level form cannot
    /// report disconnection: if the pipe drops, `on_complete` is silently never
    /// called.
    pub fn download(
        &mut self,
        request: DownloadRequest,
        on_complete: impl FnOnce(DownloadResult) + Send + 'static,
    ) {
        self.remote.Download(request, on_complete);
    }
}

/// An in-flight [`UrlLoader::download_async`] call.
///
/// A named type rather than `impl Future` so that it captures no lifetime from
/// the `UrlLoader` that produced it.
pub struct PendingDownload {
    receiver: oneshot::Receiver<DownloadResult>,
}

impl Future for PendingDownload {
    type Output = Result<DownloadResult, Disconnected>;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        Pin::new(&mut self.receiver).poll(cx).map(|result| result.map_err(|_| Disconnected))
    }
}

/// Builds a [`DownloadRequest`].
///
/// This exists because the Rust Mojom generator ignores struct field defaults,
/// so the generated struct has no constructor and every field must be filled
/// in by hand at each call site. Adding a field to the `.mojom` would
/// otherwise break every caller.
pub struct RequestBuilder {
    request: DownloadRequest,
}

impl RequestBuilder {
    fn new(url: impl Into<String>, method: HttpMethod) -> Self {
        Self {
            request: DownloadRequest {
                url: url.into(),
                method,
                headers: Vec::new(),
                body: None,
                body_content_type: None,
                max_response_bytes: 0,
                allow_http_error_results: false,
            },
        }
    }

    pub fn get(url: impl Into<String>) -> Self {
        Self::new(url, HttpMethod::kGet)
    }

    pub fn head(url: impl Into<String>) -> Self {
        Self::new(url, HttpMethod::kHead)
    }

    pub fn delete(url: impl Into<String>) -> Self {
        Self::new(url, HttpMethod::kDelete)
    }

    /// Creates a request with a body. The content type is mandatory because
    /// the service rejects a body without one.
    pub fn with_body(
        url: impl Into<String>,
        method: HttpMethod,
        body: Vec<u8>,
        content_type: impl Into<String>,
    ) -> Self {
        let mut builder = Self::new(url, method);
        builder.request.body = Some(body);
        builder.request.body_content_type = Some(content_type.into());
        builder
    }

    pub fn post(url: impl Into<String>, body: Vec<u8>, content_type: impl Into<String>) -> Self {
        Self::with_body(url, HttpMethod::kPost, body, content_type)
    }

    pub fn put(url: impl Into<String>, body: Vec<u8>, content_type: impl Into<String>) -> Self {
        Self::with_body(url, HttpMethod::kPut, body, content_type)
    }

    /// Adds a header. Names and values are validated service-side; an invalid
    /// one fails the whole request with `net::ERR_INVALID_ARGUMENT`.
    pub fn header(mut self, name: impl Into<String>, value: impl Into<Vec<u8>>) -> Self {
        self.request.headers.push(HttpHeader { name: name.into(), value: value.into() });
        self
    }

    /// Caps the buffered response body. Zero (the default) means the service
    /// default of `SimpleURLLoader::kMaxBoundedStringDownloadSize`.
    pub fn max_response_bytes(mut self, max: u64) -> Self {
        self.request.max_response_bytes = max;
        self
    }

    /// When set, non-2xx responses are returned with their body and status
    /// instead of being collapsed into `net::ERR_FAILED`.
    pub fn allow_http_error_results(mut self, allow: bool) -> Self {
        self.request.allow_http_error_results = allow;
        self
    }

    pub fn build(self) -> DownloadRequest {
        self.request
    }
}

/// Convenience accessors for [`DownloadResult`], which is a generated type and
/// so cannot have inherent methods added here.
pub trait DownloadResultExt {
    /// True if the network stack reported `net::OK`.
    fn is_ok(&self) -> bool;

    /// The response body as UTF-8, or `None` if it is not valid UTF-8.
    fn body_as_str(&self) -> Option<&str>;

    /// First value for `name`, matched case-insensitively per RFC 9110.
    fn first_header(&self, name: &str) -> Option<&[u8]>;
}

impl DownloadResultExt for DownloadResult {
    fn is_ok(&self) -> bool {
        // net::OK is 0.
        self.net_error == 0
    }

    fn body_as_str(&self) -> Option<&str> {
        std::str::from_utf8(&self.body).ok()
    }

    fn first_header(&self, name: &str) -> Option<&[u8]> {
        self.headers
            .iter()
            .find(|header| header.name.eq_ignore_ascii_case(name))
            .map(|header| header.value.as_slice())
    }
}
