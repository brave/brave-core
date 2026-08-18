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

/// Resolves a [`PendingDownload`] without integrating a Rust executor.
///
/// `PendingDownload` is resolved by a task on the Chromium sequence, so drain
/// that sequence first; `block_on` then completes without ever actually
/// blocking. Production callers must instead drive a real executor from their
/// sequence (see the `LocalPool` shim in //brave/components/skus) — this
/// shortcut only works because the fake network stack answers promptly.
fn resolve(
    run_loop: &RunLoop,
    pending: PendingDownload,
) -> Result<RawDownloadResult, Disconnected> {
    run_loop.run_until_idle();
    futures_executor::block_on(pending)
}

#[gtest(BraveRustUrlLoaderTest, GetSucceeds)]
fn get_succeeds() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/hello", "hello from the network stack", 200);

    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();

    loader.download(RequestBuilder::get("https://example.test/hello").build(), move |result| {
        expect_eq!(result.net_error, NET_OK);
        expect_eq!(result.response_code, 200);
        expect_eq!(result.final_url, "https://example.test/hello");
        expect_eq!(result.body_as_str(), Some("hello from the network stack"));
        quit();
    });

    run_loop.run();
    expect_eq!(host.TotalRequests(), 1);
}

#[gtest(BraveRustUrlLoaderTest, PostSendsBodyAndHeaders)]
fn post_sends_body_and_headers() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/api", "{\"ok\":true}", 200);

    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();

    let request = RequestBuilder::post(
        "https://example.test/api",
        br#"{"query":"value"}"#.to_vec(),
        "application/json",
    )
    .header("X-Brave-Test", "rust")
    .build();

    loader.download(request, move |result| {
        expect_eq!(result.net_error, NET_OK);
        expect_eq!(result.response_code, 200);
        expect_eq!(result.body_as_str(), Some("{\"ok\":true}"));
        // TestURLLoaderFactory synthesises these, so this also proves response
        // headers survive the round trip as raw bytes.
        expect_eq!(result.first_header("Content-Type"), Some(&b"text/html"[..]));
        quit();
    });

    run_loop.run();
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

    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();

    loader.download(RequestBuilder::get("https://example.test/missing").build(), move |result| {
        expect_eq!(result.net_error, ERR_HTTP_RESPONSE_CODE_FAILURE);
        expect_eq!(result.response_code, 404);
        expect_true!(result.body.is_empty());
        quit();
    });

    run_loop.run();
}

#[gtest(BraveRustUrlLoaderTest, HttpErrorBodyAvailableWhenAllowed)]
fn http_error_body_available_when_allowed() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/missing", "not found", 404);

    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();

    let request =
        RequestBuilder::get("https://example.test/missing").allow_http_error_results(true).build();

    loader.download(request, move |result| {
        expect_eq!(result.net_error, NET_OK);
        expect_eq!(result.response_code, 404);
        expect_eq!(result.body_as_str(), Some("not found"));
        quit();
    });

    run_loop.run();
}

/// URL validation happens service-side, since Rust has no GURL. A bad URL must
/// fail cleanly rather than reaching the network or crashing the service.
#[gtest(BraveRustUrlLoaderTest, InvalidUrlIsRejected)]
fn invalid_url_is_rejected() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (host, mut loader) = make_loader();

    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();

    loader.download(RequestBuilder::get("not a url at all").build(), move |result| {
        expect_eq!(result.net_error, ERR_INVALID_URL);
        expect_eq!(result.response_code, -1);
        quit();
    });

    run_loop.run();
    expect_eq!(host.TotalRequests(), 0);
}

/// Header values crossing the boundary are untrusted. A value containing a
/// newline would be a request-splitting vector, so the service rejects it
/// instead of feeding it to `HttpRequestHeaders::SetHeader`, which DCHECKs.
#[gtest(BraveRustUrlLoaderTest, MalformedHeaderIsRejected)]
fn malformed_header_is_rejected() {
    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (host, mut loader) = make_loader();

    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();

    let request = RequestBuilder::get("https://example.test/hello")
        .header("X-Evil", "value\r\nInjected: yes")
        .build();

    loader.download(request, move |result| {
        expect_eq!(result.net_error, ERR_INVALID_ARGUMENT);
        quit();
    });

    run_loop.run();
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

    let run_loop = RunLoop::new();
    let quit = run_loop.get_quit_closure();

    loader.download(RequestBuilder::get("https://example.test/bin").build(), move |result| {
        expect_eq!(result.net_error, NET_OK);
        expect_false!(result.body.is_empty());
        quit();
    });

    run_loop.run();
}

/// Multiple in-flight requests on one pipe must complete independently.
#[gtest(BraveRustUrlLoaderTest, ConcurrentRequests)]
fn concurrent_requests() {
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::Arc;

    let _task_env = task_environment::ffi::CreateTaskEnvironment();
    let (mut host, mut loader) = make_loader();
    host.pin_mut().AddResponse("https://example.test/a", "response-a", 200);
    host.pin_mut().AddResponse("https://example.test/b", "response-b", 200);

    let run_loop = RunLoop::new();
    let completed = Arc::new(AtomicUsize::new(0));

    for (url, expected) in
        [("https://example.test/a", "response-a"), ("https://example.test/b", "response-b")]
    {
        let quit = run_loop.get_quit_closure();
        let completed = completed.clone();
        loader.download(RequestBuilder::get(url).build(), move |result| {
            expect_eq!(result.body_as_str(), Some(expected));
            if completed.fetch_add(1, Ordering::SeqCst) == 1 {
                quit();
            }
        });
    }

    run_loop.run();
    expect_eq!(completed.load(Ordering::SeqCst), 2);
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

    let run_loop = RunLoop::new();
    let pending = loader.download_async(RequestBuilder::get("https://example.test/hello").build());

    let result = resolve(&run_loop, pending).expect("should not disconnect");
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

    let run_loop = RunLoop::new();
    expect_eq!(resolve(&run_loop, pending), Err(Disconnected));
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
    let run_loop2 = RunLoop::new();
    expect_eq!(resolve(&run_loop2, pending), Err(Disconnected));
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
    let run_loop = RunLoop::new();
    let result = resolve(&run_loop, pending).expect("should not disconnect");
    expect_eq!(result.body_as_str(), Some("response-b"));
    expect_eq!(host.TotalRequests(), 2);
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

    let run_loop = RunLoop::new();
    let result = resolve(&run_loop, loader.download_async(request)).expect("should not disconnect");

    expect_eq!(result.net_error, NET_OK);
    expect_eq!(result.body_as_str(), Some("ok"));
    // The request was accepted by validation rather than dropped.
    expect_eq!(host.TotalRequests(), 1);
}
