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
    DownloadHandle, DownloadRequest, DownloadResult, HttpHeader, HttpMethod, RetryOptions,
    SimpleUrlLoader,
};
use system::message_pipe::MessageEndpoint;
use system::scoped_handle_interop::ScopedMessagePipeHandleWrapper;

pub use simple_url_loader_rust::simple_url_loader::{
    DownloadRequest as RawDownloadRequest, DownloadResult as RawDownloadResult,
    HttpHeader as RawHttpHeader, HttpMethod as RawHttpMethod, RetryOptions as RawRetryOptions,
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
/// See [`UrlLoader::set_waker`].
///
/// `Send + Sync` is not gratuitous: Mojo response callbacks must be `Send`, and
/// `Sync` lets the waker be cloned out of the mutex so it can be invoked once
/// the lock is released.
type Waker = Arc<dyn Fn() + Send + Sync + 'static>;

#[derive(Default)]
struct PendingState {
    senders: HashMap<u64, oneshot::Sender<DownloadResult>>,
    disconnected: bool,
    waker: Option<Waker>,
}

type SharedPendingState = Arc<Mutex<PendingState>>;

/// Panicking on a poisoned mutex is correct here: it means a response callback
/// already panicked, and the map may be inconsistent.
fn lock(state: &SharedPendingState) -> std::sync::MutexGuard<'_, PendingState> {
    state.lock().expect("pending download state mutex poisoned")
}

/// Mutates the shared state, then invokes the waker *after* releasing the lock.
///
/// Waking outside the lock is required, not merely tidy: the waker polls the
/// caller's executor, and a future woken by it may immediately issue another
/// request, which re-enters this type and would deadlock on a held lock.
fn update_then_wake(state: &SharedPendingState, update: impl FnOnce(&mut PendingState)) {
    let waker = {
        let mut guard = lock(state);
        update(&mut guard);
        guard.waker.clone()
    };
    if let Some(waker) = waker {
        waker();
    }
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
            update_then_wake(&state_on_disconnect, |state| {
                state.disconnected = true;
                // Dropping every sender resolves each pending future with
                // `Err(Disconnected)`.
                state.senders.clear();
            });
        });

        let remote = PendingRemote::<dyn SimpleUrlLoader>::new(endpoint)
            .bind_with_options(None, Some(disconnect_handler));

        Self { remote, state, next_id: 0 }
    }

    /// Registers a callback invoked on the bound sequence whenever a
    /// [`PendingDownload`] becomes ready, and once on disconnection.
    ///
    /// Responses arrive as Chromium tasks, which a Rust executor has no way to
    /// wait on. Callers driving futures with a manually-pumped executor (for
    /// example `futures::executor::LocalPool`) must use this to poll it,
    /// otherwise a ready future is simply never observed.
    ///
    /// The `Send + Sync` bound is inherited from Mojo, whose response callbacks
    /// must be `Send`. An executor that is neither (an `Rc`-based `LocalPool`,
    /// say) cannot be captured here directly; reach it through a thread-local
    /// instead, which is sound because the waker always runs on the sequence
    /// this `UrlLoader` was bound to.
    pub fn set_waker(&mut self, waker: impl Fn() + Send + Sync + 'static) {
        lock(&self.state).waker = Some(Arc::new(waker));
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
    /// bound to, so that sequence must be pumped for it to make progress. See
    /// [`UrlLoader::set_waker`] if your executor is not driven by that
    /// sequence.
    ///
    /// Dropping the returned future cancels the request; see
    /// [`PendingDownload`].
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
                return PendingDownload { receiver, _cancellation: None };
            }
            state.senders.insert(id, sender);
        }

        let (cancellation, cancellation_receiver) = PendingRemote::<dyn DownloadHandle>::new_pipe()
            .expect("out of Mojo message pipe handles");

        let state = self.state.clone();
        self.remote.Download(request, cancellation_receiver, move |result| {
            update_then_wake(&state, |state| {
                // Absent only if disconnection already cleared the map, in
                // which case the future has already been resolved with an
                // error.
                if let Some(sender) = state.senders.remove(&id) {
                    // Fails only if the caller dropped the future, which is
                    // fine.
                    let _ = sender.send(result);
                }
            });
        });

        PendingDownload { receiver, _cancellation: Some(cancellation) }
    }
}

/// An in-flight [`UrlLoader::download_async`] call.
///
/// A named type rather than `impl Future` so that it captures no lifetime from
/// the `UrlLoader` that produced it.
///
/// Dropping this cancels the request: the held `DownloadHandle` pipe closes,
/// the service notices the disconnect, destroys the underlying
/// `network::SimpleURLLoader` and answers the outstanding reply with
/// `net::ERR_ABORTED`.
pub struct PendingDownload {
    receiver: oneshot::Receiver<DownloadResult>,
    // Held solely for its `Drop`, which closes the pipe and so cancels the
    // request. Deliberately never bound: `DownloadHandle` has no methods.
    _cancellation: Option<PendingRemote<dyn DownloadHandle>>,
}

impl PendingDownload {
    /// Returns the outcome if it is already available, without awaiting and
    /// without needing an executor.
    ///
    /// Useful for callers that are not futures-based, and to check whether a
    /// request failed immediately (issuing one on an already-dead pipe resolves
    /// straight away).
    pub fn try_take(&mut self) -> Option<Result<DownloadResult, Disconnected>> {
        match self.receiver.try_recv() {
            Ok(Some(result)) => Some(Ok(result)),
            Ok(None) => None,
            Err(oneshot::Canceled) => Some(Err(Disconnected)),
        }
    }
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
                retry_options: None,
                sanitize_json_response: false,
            },
        }
    }

    /// A request with no body, for callers whose method is only known at
    /// runtime.
    pub fn with_method(url: impl Into<String>, method: HttpMethod) -> Self {
        Self::new(url, method)
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
    /// instead of being collapsed into `net::ERR_HTTP_RESPONSE_CODE_FAILURE`.
    pub fn allow_http_error_results(mut self, allow: bool) -> Self {
        self.request.allow_http_error_results = allow;
        self
    }

    /// Retries the request according to `options`.
    ///
    /// Only safe for idempotent requests; nothing checks that for you.
    pub fn retry(mut self, options: RetryOptions) -> Self {
        self.request.retry_options = Some(options);
        self
    }

    /// Convenience for the common "retry once if the network changed under us"
    /// policy, which is safe even for non-idempotent requests because the
    /// request could not have been delivered.
    pub fn retry_on_network_change(self, max_retries: u32) -> Self {
        self.retry(RetryOptions {
            max_retries,
            retry_on_5xx: false,
            retry_on_network_change: true,
            retry_on_name_not_resolved: false,
        })
    }

    /// Parses the response body as strict RFC 8259 JSON and re-serializes it
    /// canonically, so this crate's caller never sees raw server bytes.
    ///
    /// A body that fails to parse, or whose top level is neither an object nor
    /// an array, arrives *empty* with the status untouched. That silence is
    /// deliberate: it reproduces the behaviour of
    /// //brave/components/api_request_helper, which existing callers rely on.
    pub fn sanitize_json_response(mut self, sanitize: bool) -> Self {
        self.request.sanitize_json_response = sanitize;
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
