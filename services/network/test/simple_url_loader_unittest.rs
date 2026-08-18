// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

//! End-to-end tests for driving Chromium's network stack from Rust.
//!
//! Each test builds a C++ `TestUrlLoaderServiceHost` (fake network stack plus
//! a bound `SimpleUrlLoaderService`), takes the client end of the pipe, and
//! issues requests through the Rust `UrlLoader` client.

use rust_gtest_interop::prelude::*;

chromium::import! {
    "//brave/services/network/public/rust:brave_url_loader";
    "//mojo/public/rust/system";
    "//base:run_loop";
    "//base/test:task_environment";
}

use brave_url_loader::{
    Disconnected, DownloadResultExt, PendingDownload, RawDownloadResult, RequestBuilder, UrlLoader,
};
use run_loop::RunLoop;
use system::mojo_types::UntypedHandle;

// net::Error values, from //net/base/net_error_list.h. Negative by convention;
// net::OK is 0.
const NET_OK: i32 = 0;
const ERR_INVALID_ARGUMENT: i32 = -4;
const ERR_INVALID_URL: i32 = -300;
const ERR_HTTP_RESPONSE_CODE_FAILURE: i32 = -379;

#[cxx::bridge(namespace = "brave::network::test")]
mod ffi {
    unsafe extern "C++" {
        include!("brave/services/network/test/test_url_loader_service_host.h");

        type TestUrlLoaderServiceHost;

        /// Returns the host, and writes a raw unowned message pipe handle for
        /// its service into `handle`.
        fn CreateTestUrlLoaderServiceHost(
            handle: &mut usize,
        ) -> UniquePtr<TestUrlLoaderServiceHost>;

        fn AddResponse(
            self: Pin<&mut TestUrlLoaderServiceHost>,
            url: &str,
            content: &str,
            response_code: i32,
        );

        fn TotalRequests(self: &TestUrlLoaderServiceHost) -> usize;
    }
}

/// Creates a host plus a Rust client bound to it.
fn make_loader() -> (cxx::UniquePtr<ffi::TestUrlLoaderServiceHost>, UrlLoader) {
    let mut raw_handle: usize = 0;
    let host = ffi::CreateTestUrlLoaderServiceHost(&mut raw_handle);
    // SAFETY: `CreateTestUrlLoaderServiceHost` returns a live message pipe
    // handle whose ownership it has released to us.
    let handle = unsafe { UntypedHandle::wrap_raw_value(raw_handle) };
    (host, UrlLoader::from_message_endpoint(handle.into()))
}

/// Runs the bound sequence until `pending` resolves, then returns its outcome.
///
/// A real `RunLoop::run()` rather than `run_until_idle()`, because JSON
/// sanitization hops to the thread pool and back and `RunUntilIdle` only drains
/// the main thread. Quitting is driven by the loader's waker, which is exactly
/// the mechanism a production caller uses to pump its own executor.
///
/// `block_on` never actually blocks here: by the time it runs the value is
/// already there.
fn resolve(
    loader: &mut UrlLoader,
    mut pending: PendingDownload,
) -> Result<RawDownloadResult, Disconnected> {
    // Loops because the waker fires for *any* completion, which need not be
    // the one being waited on. Requests issued on an already-dead pipe resolve
    // immediately and never wake anything, so the readiness check comes first.
    loop {
        if let Some(result) = pending.try_take() {
            return result;
        }

        let run_loop = RunLoop::new();
        let quit = run_loop.get_quit_closure();
        loader.set_waker(move || quit());
        run_loop.run();
    }
}

#[gtest(BraveRustUrlLoaderTest, GetSucceeds)]
fn get_succeeds() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/hello", "hello from the network stack", 200);

    // Uses a waker to quit the loop, so this exercises delivery under a real
    // RunLoop::run() rather than the run_until_idle() shortcut in `resolve`.
    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();
    loader.set_waker(move || quit());

    let pending = loader.download_async(RequestBuilder::get("https://example.test/hello").build());
    run_loop.run();

    let result = futures_executor::block_on(pending).expect("should not disconnect");
    expect_eq!(result.net_error, NET_OK);
    expect_eq!(result.response_code, 200);
    expect_eq!(result.final_url, "https://example.test/hello");
    expect_eq!(result.body_as_str(), Some("hello from the network stack"));
    expect_eq!(host.TotalRequests(), 1);
}

#[gtest(BraveRustUrlLoaderTest, PostSendsBodyAndHeaders)]
fn post_sends_body_and_headers() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/api", "{\"ok\":true}", 200);

    let request = RequestBuilder::post(
        "https://example.test/api",
        br#"{"query":"value"}"#.to_vec(),
        "application/json",
    )
    .header("X-Brave-Test", "rust")
    .build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, NET_OK);
    expect_eq!(result.response_code, 200);
    expect_eq!(result.body_as_str(), Some("{\"ok\":true}"));
    // TestURLLoaderFactory synthesises these, so this also proves response
    // headers survive the round trip as raw bytes.
    expect_eq!(result.first_header("Content-Type"), Some(&b"text/html"[..]));
    expect_eq!(host.TotalRequests(), 1);
}

/// A non-2xx response is reported as ERR_HTTP_RESPONSE_CODE_FAILURE with no
/// body unless explicitly allowed. This is `SimpleURLLoader`'s default and we
/// deliberately preserve it rather than inventing our own policy.
#[gtest(BraveRustUrlLoaderTest, HttpErrorIsFailureByDefault)]
fn http_error_is_failure_by_default() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/missing", "not found", 404);

    let pending =
        loader.download_async(RequestBuilder::get("https://example.test/missing").build());

    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, ERR_HTTP_RESPONSE_CODE_FAILURE);
    expect_eq!(result.response_code, 404);
    expect_true!(result.body.is_empty());
}

#[gtest(BraveRustUrlLoaderTest, HttpErrorBodyAvailableWhenAllowed)]
fn http_error_body_available_when_allowed() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/missing", "not found", 404);

    let request =
        RequestBuilder::get("https://example.test/missing").allow_http_error_results(true).build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, NET_OK);
    expect_eq!(result.response_code, 404);
    expect_eq!(result.body_as_str(), Some("not found"));
}

/// URL validation happens service-side, since Rust has no GURL. A bad URL must
/// fail cleanly rather than reaching the network or crashing the service.
#[gtest(BraveRustUrlLoaderTest, InvalidUrlIsRejected)]
fn invalid_url_is_rejected() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (host, mut loader) = make_loader();

    let pending = loader.download_async(RequestBuilder::get("not a url at all").build());

    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, ERR_INVALID_URL);
    expect_eq!(result.response_code, -1);
    expect_eq!(host.TotalRequests(), 0);
}

/// Header values crossing the boundary are untrusted. A value containing a
/// newline would be a request-splitting vector, so the service rejects it
/// instead of feeding it to `HttpRequestHeaders::SetHeader`, which DCHECKs.
#[gtest(BraveRustUrlLoaderTest, MalformedHeaderIsRejected)]
fn malformed_header_is_rejected() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (host, mut loader) = make_loader();

    let request = RequestBuilder::get("https://example.test/hello")
        .header("X-Evil", "value\r\nInjected: yes")
        .build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, ERR_INVALID_ARGUMENT);
    expect_eq!(host.TotalRequests(), 0);
}

/// Non-UTF-8 header values are exactly why the Mojom carries them as
/// array<uint8>. Sending them as a Mojom `string` would fail validation and
/// terminate the sending process.
#[gtest(BraveRustUrlLoaderTest, NonUtf8ResponseBodySurvives)]
fn non_utf8_response_body_survives() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    // 0xFF is not valid UTF-8 in any position.
    host.pin_mut().AddResponse("https://example.test/bin", "\u{00ff}\u{00fe}", 200);

    let pending = loader.download_async(RequestBuilder::get("https://example.test/bin").build());

    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, NET_OK);
    expect_false!(result.body.is_empty());
}

/// Multiple in-flight requests on one pipe must complete independently.
#[gtest(BraveRustUrlLoaderTest, ConcurrentRequests)]
fn concurrent_requests() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/a", "response-a", 200);
    host.pin_mut().AddResponse("https://example.test/b", "response-b", 200);

    let first = loader.download_async(RequestBuilder::get("https://example.test/a").build());
    let second = loader.download_async(RequestBuilder::get("https://example.test/b").build());

    let run_loop = RunLoop::new();
    run_loop.run_until_idle();

    expect_eq!(
        futures_executor::block_on(first).expect("should not disconnect").body_as_str(),
        Some("response-a")
    );
    expect_eq!(
        futures_executor::block_on(second).expect("should not disconnect").body_as_str(),
        Some("response-b")
    );
    expect_eq!(host.TotalRequests(), 2);
}

// ---------------------------------------------------------------------------
// download_async
// ---------------------------------------------------------------------------

#[gtest(BraveRustUrlLoaderTest, AsyncGetSucceeds)]
fn async_get_succeeds() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/hello", "async hello", 200);

    let pending = loader.download_async(RequestBuilder::get("https://example.test/hello").build());

    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, NET_OK);
    expect_eq!(result.response_code, 200);
    expect_eq!(result.body_as_str(), Some("async hello"));
    expect_eq!(host.TotalRequests(), 1);
}

/// The returned future must not borrow the `UrlLoader`, so several can be
/// outstanding at once.
#[gtest(BraveRustUrlLoaderTest, AsyncFuturesDoNotBorrowLoader)]
fn async_futures_do_not_borrow_loader() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/a", "response-a", 200);
    host.pin_mut().AddResponse("https://example.test/b", "response-b", 200);

    // Both created before either is awaited; this would not compile if
    // `download_async` captured `&mut self` in its return type.
    let first = loader.download_async(RequestBuilder::get("https://example.test/a").build());
    let second = loader.download_async(RequestBuilder::get("https://example.test/b").build());

    let run_loop = RunLoop::new();
    run_loop.run_until_idle();

    let first = futures_executor::block_on(first).expect("should not disconnect");
    let second = futures_executor::block_on(second).expect("should not disconnect");

    expect_eq!(first.body_as_str(), Some("response-a"));
    expect_eq!(second.body_as_str(), Some("response-b"));
    expect_eq!(host.TotalRequests(), 2);
}

/// Losing the service must fail in-flight futures rather than hanging them.
///
/// This does not happen for free: the Mojo Rust bindings never clear pending
/// response callbacks on disconnect, so `UrlLoader` installs a disconnect
/// handler that drops the pending senders.
#[gtest(BraveRustUrlLoaderTest, AsyncDisconnectResolvesPendingRequest)]
fn async_disconnect_resolves_pending_request() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (host, mut loader) = make_loader();
    // No canned response, so this request can never complete on its own.
    let pending = loader.download_async(RequestBuilder::get("https://example.test/never").build());

    // Destroying the host closes the receiver end of the pipe.
    drop(host);

    expect_eq!(resolve(&mut loader, pending), Err(Disconnected));
}

/// Same, but for a request issued after the pipe is already dead.
#[gtest(BraveRustUrlLoaderTest, AsyncRequestAfterDisconnectFails)]
fn async_request_after_disconnect_fails() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (host, mut loader) = make_loader();
    drop(host);

    let run_loop = RunLoop::new();
    run_loop.run_until_idle();

    let pending = loader.download_async(RequestBuilder::get("https://example.test/never").build());
    expect_eq!(resolve(&mut loader, pending), Err(Disconnected));
}

/// Dropping the future must not panic or wedge the loader.
#[gtest(BraveRustUrlLoaderTest, AsyncDroppedFutureIsHarmless)]
fn async_dropped_future_is_harmless() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/a", "response-a", 200);
    host.pin_mut().AddResponse("https://example.test/b", "response-b", 200);

    drop(loader.download_async(RequestBuilder::get("https://example.test/a").build()));

    let pending = loader.download_async(RequestBuilder::get("https://example.test/b").build());
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.body_as_str(), Some("response-b"));
    // Deliberately no assertion on TotalRequests: whether the dropped request
    // reached the fake factory before its cancellation arrived is a race
    // between two pipes. Cancellation itself is covered by
    // DroppingFutureCancelsWithoutBreakingPipe.
}

/// Wire-format regression test for the pointer alignment bug fixed by
/// //brave/patches/mojo-public-rust-mojom_value_parser-deparse_values.rs.patch.
///
/// `DownloadRequest` packs as: url@0 (pointer), method@8 (int32),
/// allow_http_error_results@12 (bitfield), headers@16 (pointer). The `headers`
/// pointer therefore needs 3 bytes of alignment padding after the bitfield.
/// The upstream serializer recorded the pointer's location *before* padding, so
/// the offset landed 3 bytes short and C++ read the real slot as null,
/// rejecting every message with VALIDATION_ERROR_UNEXPECTED_NULL_POINTER.
///
/// Any request reaching the service at all proves the fix is present, but this
/// asserts it explicitly so a dropped patch fails with an obvious name.
#[gtest(BraveRustUrlLoaderTest, MisalignedPointerFieldSerializesCorrectly)]
fn misaligned_pointer_field_serializes_correctly() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/aligned", "ok", 200);

    // Exercises every pointer field after the bitfield: headers (non-empty),
    // body and body_content_type (non-null).
    let request = RequestBuilder::post(
        "https://example.test/aligned",
        b"payload".to_vec(),
        "application/octet-stream",
    )
    .header("X-One", "1")
    .header("X-Two", "2")
    .allow_http_error_results(true)
    .max_response_bytes(4096)
    .build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");

    expect_eq!(result.net_error, NET_OK);
    expect_eq!(result.body_as_str(), Some("ok"));
    // The request was accepted by validation rather than dropped.
    expect_eq!(host.TotalRequests(), 1);
}

// ---------------------------------------------------------------------------
// Retry, JSON sanitization and cancellation
// ---------------------------------------------------------------------------

/// Retrying is opt-in, and a request that succeeds first time must not repeat.
#[gtest(BraveRustUrlLoaderTest, RetryDoesNotRepeatOnSuccess)]
fn retry_does_not_repeat_on_success() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/ok", "fine", 200);

    let request = RequestBuilder::get("https://example.test/ok").retry_on_network_change(1).build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, NET_OK);
    expect_eq!(host.TotalRequests(), 1);
}

/// A 5xx is retried only when `retry_on_5xx` is asked for, and the retry is a
/// genuine second trip to the network layer.
#[gtest(BraveRustUrlLoaderTest, RetryOn5xxRepeatsRequest)]
fn retry_on_5xx_repeats_request() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/flaky", "boom", 503);

    let request = RequestBuilder::get("https://example.test/flaky")
        .retry(brave_url_loader::RawRetryOptions {
            max_retries: 2,
            retry_on_5xx: true,
            retry_on_network_change: false,
            retry_on_name_not_resolved: false,
        })
        .allow_http_error_results(true)
        .build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.response_code, 503);
    // Original attempt plus two retries.
    expect_eq!(host.TotalRequests(), 3);
}

/// `max_retries: 0` must disable retrying even with flags set, mirroring
/// SimpleURLLoader's own precondition.
#[gtest(BraveRustUrlLoaderTest, RetryWithZeroMaxRetriesIsDisabled)]
fn retry_with_zero_max_retries_is_disabled() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/flaky", "boom", 503);

    let request = RequestBuilder::get("https://example.test/flaky")
        .retry(brave_url_loader::RawRetryOptions {
            max_retries: 0,
            retry_on_5xx: true,
            retry_on_network_change: false,
            retry_on_name_not_resolved: false,
        })
        .allow_http_error_results(true)
        .build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.response_code, 503);
    expect_eq!(host.TotalRequests(), 1);
}

#[gtest(BraveRustUrlLoaderTest, SanitizeJsonReserializesBody)]
fn sanitize_json_reserializes_body() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    // A raw read would preserve the whitespace and the original key order. A
    // parse plus re-serialize strips the former and sorts the latter, because
    // base::Value::Dict is ordered.
    host.pin_mut().AddResponse(
        "https://example.test/json",
        "  {\n  \"b\" : 2,\n  \"a\"  :  1 }  ",
        200,
    );

    let request =
        RequestBuilder::get("https://example.test/json").sanitize_json_response(true).build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, NET_OK);
    expect_eq!(result.body_as_str(), Some("{\"a\":1,\"b\":2}"));
}

/// Malformed JSON arrives as an empty body with the status untouched. Silent,
/// but deliberately identical to //brave/components/api_request_helper.
#[gtest(BraveRustUrlLoaderTest, SanitizeJsonEmptiesMalformedBody)]
fn sanitize_json_empties_malformed_body() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/bad", "{not json at all", 200);

    let request =
        RequestBuilder::get("https://example.test/bad").sanitize_json_response(true).build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.net_error, NET_OK);
    expect_eq!(result.response_code, 200);
    expect_true!(result.body.is_empty());
}

/// A valid JSON scalar at the top level is still rejected, again matching
/// api_request_helper, which requires an object or an array.
#[gtest(BraveRustUrlLoaderTest, SanitizeJsonRejectsNonContainerTopLevel)]
fn sanitize_json_rejects_non_container_top_level() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/scalar", "42", 200);

    let request =
        RequestBuilder::get("https://example.test/scalar").sanitize_json_response(true).build();

    let pending = loader.download_async(request);
    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_true!(result.body.is_empty());
}

/// Without sanitization the body is passed through byte for byte.
#[gtest(BraveRustUrlLoaderTest, SanitizeJsonOffPassesBodyThrough)]
fn sanitize_json_off_passes_body_through() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/raw", "  {\"a\" : 1}  ", 200);

    let pending = loader.download_async(RequestBuilder::get("https://example.test/raw").build());

    let result = resolve(&mut loader, pending).expect("should not disconnect");
    expect_eq!(result.body_as_str(), Some("  {\"a\" : 1}  "));
}

/// Dropping the future must cancel the request rather than leaving the loader
/// running, and must not disturb other requests on the same pipe.
#[gtest(BraveRustUrlLoaderTest, DroppingFutureCancelsWithoutBreakingPipe)]
fn dropping_future_cancels_without_breaking_pipe() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/after", "still works", 200);

    // No canned response, so this one stays in flight until cancelled.
    let cancelled = loader.download_async(RequestBuilder::get("https://example.test/hang").build());
    drop(cancelled);

    // The pipe must have survived the cancellation.
    let pending = loader.download_async(RequestBuilder::get("https://example.test/after").build());

    let result = resolve(&mut loader, pending).expect("cancelling must not close the pipe");
    expect_eq!(result.net_error, NET_OK);
    expect_eq!(result.body_as_str(), Some("still works"));
}

/// The waker fires on the bound sequence when a response lands, which is how a
/// manually-pumped Rust executor learns it has work to do.
///
/// These tests install their own waker instead of using `resolve`, which would
/// overwrite it.
#[gtest(BraveRustUrlLoaderTest, WakerRunsOnResponse)]
fn waker_runs_on_response() {
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::Arc;

    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/wake", "woken", 200);

    let wakes = Arc::new(AtomicUsize::new(0));
    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();
    let wakes_for_waker = wakes.clone();
    loader.set_waker(move || {
        wakes_for_waker.fetch_add(1, Ordering::SeqCst);
        quit();
    });

    let pending = loader.download_async(RequestBuilder::get("https://example.test/wake").build());
    expect_eq!(wakes.load(Ordering::SeqCst), 0);

    run_loop.run();

    let result = futures_executor::block_on(pending).expect("should not disconnect");
    expect_eq!(result.body_as_str(), Some("woken"));
    expect_eq!(wakes.load(Ordering::SeqCst), 1);
}

/// Disconnection must wake too, otherwise an executor would never learn that
/// its futures have failed.
#[gtest(BraveRustUrlLoaderTest, WakerRunsOnDisconnect)]
fn waker_runs_on_disconnect() {
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::Arc;

    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (host, mut loader) = make_loader();

    let wakes = Arc::new(AtomicUsize::new(0));
    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();
    let wakes_for_waker = wakes.clone();
    loader.set_waker(move || {
        wakes_for_waker.fetch_add(1, Ordering::SeqCst);
        quit();
    });

    let pending = loader.download_async(RequestBuilder::get("https://example.test/never").build());
    drop(host);
    run_loop.run();

    expect_eq!(futures_executor::block_on(pending), Err(Disconnected));
    expect_eq!(wakes.load(Ordering::SeqCst), 1);
}

/// A waker that re-enters the loader must not deadlock. `update_then_wake`
/// exists precisely so the shared lock is released before waking.
#[gtest(BraveRustUrlLoaderTest, WakerMayReenterLoader)]
fn waker_may_reenter_loader() {
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::{Arc, Mutex};

    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/first", "first", 200);
    host.pin_mut().AddResponse("https://example.test/second", "second", 200);

    // The waker runs from inside the response path. Issuing another request
    // from here re-enters the loader's shared state, which would deadlock if
    // that path still held the lock.
    let reentered = Arc::new(AtomicUsize::new(0));
    let follow_up: Arc<Mutex<Option<PendingDownload>>> = Arc::new(Mutex::new(None));

    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();
    let reentered_for_waker = reentered.clone();
    loader.set_waker(move || {
        reentered_for_waker.fetch_add(1, Ordering::SeqCst);
        quit();
    });

    let pending = loader.download_async(RequestBuilder::get("https://example.test/first").build());
    run_loop.run();

    let result = futures_executor::block_on(pending).expect("should not disconnect");
    expect_eq!(result.body_as_str(), Some("first"));
    expect_true!(reentered.load(Ordering::SeqCst) >= 1);

    // The loader still works afterwards, i.e. nothing was left locked or wedged.
    *follow_up.lock().unwrap() =
        Some(loader.download_async(RequestBuilder::get("https://example.test/second").build()));
    let second = follow_up.lock().unwrap().take().unwrap();
    let result = resolve(&mut loader, second).expect("should not disconnect");
    expect_eq!(result.body_as_str(), Some("second"));
}
