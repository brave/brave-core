// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

use std::io::Write;
use std::panic;
use std::time::Duration;
use std::time::Instant;

use chrono;

use Request;
use Response;

/// Adds a log entry to the given writer for each request.
///
/// Writes a line to the given "writer" after processing each request.
/// Log line info has the format:
/// `"{%Y-%m-%d %H:%M%S%.6f} UTC - {METHOD} {URL} - {ELAPSED_TIME} - {RESP_SATUS}"`
///
/// If you would like to customize the log output or functionality (such as integrating
/// with the [`log`](https://docs.rs/log) crate, see [`rouille::log_custom`](fn.log_custom.html))
///
/// # Example
///
/// ```
/// use std::io;
/// use rouille::{Request, Response};
///
/// fn handle(request: &Request) -> Response {
///     rouille::log(request, io::stdout(), || {
///         Response::text("hello world")
///     })
/// }
/// ```
pub fn log<W, F>(rq: &Request, mut output: W, f: F) -> Response
where
    W: Write,
    F: FnOnce() -> Response,
{
    let start_instant = Instant::now();
    let rq_line = format!(
        "{} UTC - {} {}",
        chrono::Utc::now().format("%Y-%m-%d %H:%M:%S%.6f"),
        rq.method(),
        rq.raw_url()
    );

    // Calling the handler and catching potential panics.
    // Note that this we always resume unwinding afterwards, we can ignore the small panic-safety
    // mechanism of `catch_unwind`.
    let response = panic::catch_unwind(panic::AssertUnwindSafe(f));

    let elapsed_time = format_time(start_instant.elapsed());

    match response {
        Ok(response) => {
            let _ = writeln!(
                output,
                "{} - {} - {}",
                rq_line, elapsed_time, response.status_code
            );
            response
        }
        Err(payload) => {
            // There is probably no point in printing the payload, as this is done by the panic
            // handler.
            let _ = writeln!(output, "{} - {} - PANIC!", rq_line, elapsed_time);
            panic::resume_unwind(payload);
        }
    }
}

/// Calls custom logging functions after processing a request.
///
/// This is nearly identical to the [`rouille::log`](fn.log.html) function except it
/// takes two logging functions that will be called with access to the request/response
/// structs and the total execution duration of the handler.
///
/// # Example
///
/// ```
/// #[macro_use] extern crate log;
/// extern crate chrono;
/// # extern crate rouille;
/// use rouille::{Request, Response};
///
///
/// fn handle(request: &Request) -> Response {
///     let now = chrono::Utc::now().format("%Y-%m-%d %H:%M:%S%.6f");
///     let log_ok = |req: &Request, resp: &Response, _elap: std::time::Duration| {
///         info!("{} {} {}", now, req.method(), req.raw_url());
///     };
///     let log_err = |req: &Request, _elap: std::time::Duration| {
///         error!("{} Handler panicked: {} {}", now, req.method(), req.raw_url());
///     };
///     rouille::log_custom(request, log_ok, log_err, || {
///         Response::text("hello world")
///     })
/// }
/// #
/// # fn main() { }
/// ```
pub fn log_custom<L, E, F>(req: &Request, log_ok_f: L, log_err_f: E, handler: F) -> Response
where
    L: Fn(&Request, &Response, Duration),
    E: Fn(&Request, Duration),
    F: FnOnce() -> Response,
{
    let start_instant = Instant::now();

    // Call the handler and catch panics.
    // Note that we always resume unwinding afterwards.
    // We can ignore the small panic-safety mechanism of `catch_unwind`.
    let response = panic::catch_unwind(panic::AssertUnwindSafe(handler));
    let elapsed = start_instant.elapsed();

    match response {
        Ok(response) => {
            log_ok_f(req, &response, elapsed);
            response
        }
        Err(payload) => {
            log_err_f(req, elapsed);
            // The panic handler will print the payload contents
            panic::resume_unwind(payload);
        }
    }
}

fn format_time(duration: Duration) -> String {
    let secs_part = match duration.as_secs().checked_mul(1_000_000_000) {
        Some(v) => v,
        None => return format!("{}s", duration.as_secs() as f64),
    };

    let duration_in_ns = secs_part + u64::from(duration.subsec_nanos());

    if duration_in_ns < 1_000 {
        format!("{}ns", duration_in_ns)
    } else if duration_in_ns < 1_000_000 {
        format!("{:.1}us", duration_in_ns as f64 / 1_000.0)
    } else if duration_in_ns < 1_000_000_000 {
        format!("{:.1}ms", duration_in_ns as f64 / 1_000_000.0)
    } else {
        format!("{:.1}s", duration_in_ns as f64 / 1_000_000_000.0)
    }
}
