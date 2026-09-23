// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

//! The rouille library is very easy to get started with.
//!
//! Listening to a port is done by calling the [`start_server`](fn.start_server.html) function:
//!
//! ```no_run
//! use rouille::Request;
//! use rouille::Response;
//!
//! rouille::start_server("0.0.0.0:80", move |request| {
//!     Response::text("hello world")
//! });
//! ```
//!
//! Whenever an HTTP request is received on the address passed as first parameter, the closure
//! passed as second parameter is called. This closure must then return a
//! [`Response`](struct.Response.html) that will be sent back to the client.
//!
//! See the documentation of [`start_server`](fn.start_server.html) for more details.
//!
//! # Analyzing the request
//!
//! The parameter that the closure receives is a [`Request`](struct.Request.html) object that
//! represents the request made by the client.
//!
//! The `Request` object itself provides some getters, but most advanced functionalities are
//! provided by other modules of this crate.
//!
//! - In order to dispatch between various code depending on the URL, you can use the
//!   [`router!`](macro.router.html) macro.
//! - In order to analyze the body of the request, like handling JSON input, form input, etc. you
//!   can take a look at [the `input` module](input/index.html).
//!
//! # Returning a response
//!
//! Once you analyzed the request, it is time to return a response by returning a
//! [`Response`](struct.Response.html) object.
//!
//! All the members of `Response` are public, so you can customize it as you want. There are also
//! several constructors that you build a basic `Response` which can then modify.
//!
//! In order to serve static files, take a look at
//! [the `match_assets` function](fn.match_assets.html).
//!
//! In order to apply content encodings (including compression such as gzip or deflate), see
//! the [content_encoding module](content_encoding/index.html), and specifically the
//! [content_encoding::apply](content_encoding/fn.apply.html) function.

#![deny(unsafe_code)]

extern crate base64;
#[cfg(feature = "brotli")]
extern crate brotli;
extern crate chrono;
#[cfg(feature = "gzip")]
extern crate deflate;
extern crate filetime;
extern crate multipart;
extern crate rand;
extern crate serde;
#[macro_use]
extern crate serde_derive;
pub extern crate percent_encoding;
extern crate serde_json;
extern crate sha1_smol;
extern crate threadpool;
extern crate time;
extern crate tiny_http;
pub extern crate url;

// https://github.com/servo/rust-url/blob/e121d8d0aafd50247de5f5310a227ecb1efe6ffe/percent_encoding/lib.rs#L126
pub const DEFAULT_ENCODE_SET: &percent_encoding::AsciiSet = &percent_encoding::CONTROLS
    .add(b' ')
    .add(b'"')
    .add(b'#')
    .add(b'<')
    .add(b'>')
    .add(b'`')
    .add(b'?')
    .add(b'{')
    .add(b'}');

pub use assets::extension_to_mime;
pub use assets::match_assets;
pub use log::{log, log_custom};
pub use response::{Response, ResponseBody};
pub use tiny_http::ReadWrite;

use std::error::Error;
use std::fmt;
use std::io::Cursor;
use std::io::Read;
use std::io::Result as IoResult;
use std::marker::PhantomData;
use std::net::SocketAddr;
use std::net::ToSocketAddrs;
use std::panic;
use std::panic::AssertUnwindSafe;
use std::slice::Iter as SliceIter;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::mpsc;
use std::sync::Arc;
use std::sync::Mutex;
use std::thread;
use std::time::Duration;

pub mod cgi;
pub mod content_encoding;
pub mod input;
pub mod proxy;
pub mod session;
pub mod websocket;

mod assets;
mod find_route;
mod log;
mod response;
mod router;
#[doc(hidden)]
pub mod try_or_400;

/// This macro assumes that the current function returns a `Response` and takes a `Result`.
/// If the expression you pass to the macro is an error, then a 404 response is returned.
#[macro_export]
macro_rules! try_or_404 {
    ($result:expr) => {
        match $result {
            Ok(r) => r,
            Err(_) => return $crate::Response::empty_404(),
        }
    };
}

/// This macro assumes that the current function returns a `Response`. If the condition you pass
/// to the macro is false, then a 400 response is returned.
///
/// # Example
///
/// ```
/// # #[macro_use] extern crate rouille;
/// # fn main() {
/// use rouille::Request;
/// use rouille::Response;
///
/// fn handle_something(request: &Request) -> Response {
///     let data = try_or_400!(post_input!(request, {
///         field1: u32,
///         field2: String,
///     }));
///
///     assert_or_400!(data.field1 >= 2);
///     Response::text("hello")
/// }
/// # }
/// ```
#[macro_export]
macro_rules! assert_or_400 {
    ($cond:expr) => {
        if !$cond {
            return $crate::Response::empty_400();
        }
    };
}

/// Starts a server and uses the given requests handler.
///
/// The request handler takes a `&Request` and must return a `Response` to send to the user.
///
/// > **Note**: `start_server` is meant to be an easy-to-use function. If you want more control,
/// > see [the `Server` struct](struct.Server.html).
///
/// # Common mistakes
///
/// The handler must capture its environment by value and not by reference (`'static`). If you
/// use closure, don't forget to put `move` in front of the closure.
///
/// The handler must also be thread-safe (`Send` and `Sync`).
/// For example this handler isn't thread-safe:
///
/// ```should_fail
/// let mut requests_counter = 0;
///
/// rouille::start_server("localhost:80", move |request| {
///     requests_counter += 1;
///
///     // ... rest of the handler ...
/// # panic!()
/// })
/// ```
///
/// Multiple requests can be processed simultaneously, therefore you can't mutably access
/// variables from the outside.
///
/// Instead you must use a `Mutex`:
///
/// ```no_run
/// use std::sync::Mutex;
/// let requests_counter = Mutex::new(0);
///
/// rouille::start_server("localhost:80", move |request| {
///     *requests_counter.lock().unwrap() += 1;
///
///     // rest of the handler
/// # panic!()
/// })
/// ```
///
/// # Panic handling in the handler
///
/// If your request handler panics, a 500 error will automatically be sent to the client.
///
/// # Panic
///
/// This function will panic if the server starts to fail (for example if you use a port that is
/// already occupied) or if the socket is force-closed by the operating system.
///
/// If you need to handle these situations, please see `Server`.
pub fn start_server<A, F>(addr: A, handler: F) -> !
where
    A: ToSocketAddrs,
    F: Send + Sync + 'static + Fn(&Request) -> Response,
{
    Server::new(addr, handler)
        .expect("Failed to start server")
        .run();
    panic!("The server socket closed unexpectedly")
}

/// Identical to `start_server` but uses a `ThreadPool` of the given size.
///
/// When `pool_size` is `None`, the thread pool size will default to `8 * num-cpus`.
/// `pool_size` must be greater than zero or this function will panic.
pub fn start_server_with_pool<A, F>(addr: A, pool_size: Option<usize>, handler: F) -> !
where
    A: ToSocketAddrs,
    F: Send + Sync + 'static + Fn(&Request) -> Response,
{
    let pool_size = pool_size.unwrap_or_else(|| {
        8 * thread::available_parallelism()
            .map(|n| n.get())
            .unwrap_or(1)
    });

    Server::new(addr, handler)
        .expect("Failed to start server")
        .pool_size(pool_size)
        .run();
    panic!("The server socket closed unexpectedly")
}

struct AtomicCounter(Arc<AtomicUsize>);

impl AtomicCounter {
    fn new(count: &Arc<AtomicUsize>) -> Self {
        count.fetch_add(1, Ordering::Relaxed);
        AtomicCounter(Arc::clone(count))
    }
}

impl Drop for AtomicCounter {
    fn drop(&mut self) {
        self.0.fetch_sub(1, Ordering::Release);
    }
}

/// Executes a function in either a thread of a thread pool
enum Executor {
    Threaded { count: Arc<AtomicUsize> },
    Pooled { pool: threadpool::ThreadPool },
}
impl Executor {
    /// `size` must be greater than zero or the call to `ThreadPool::new` will panic.
    fn with_size(size: usize) -> Self {
        let pool = threadpool::ThreadPool::new(size);
        Executor::Pooled { pool }
    }

    #[inline]
    fn execute<F: FnOnce() + Send + 'static>(&self, f: F) {
        match *self {
            Executor::Threaded { ref count } => {
                let counter = AtomicCounter::new(count);
                thread::spawn(move || {
                    let _counter = counter;
                    f()
                });
            }
            Executor::Pooled { ref pool } => {
                pool.execute(f);
            }
        }
    }

    fn join(&self) {
        match *self {
            Executor::Threaded { ref count } => {
                while count.load(Ordering::Acquire) > 0 {
                    thread::sleep(Duration::from_millis(100));
                }
            }
            Executor::Pooled { ref pool } => {
                pool.join();
            }
        }
    }
}

impl Default for Executor {
    fn default() -> Self {
        Executor::Threaded {
            count: Arc::new(AtomicUsize::new(0)),
        }
    }
}

/// A listening server.
///
/// This struct is the more manual server creation API of rouille and can be used as an alternative
/// to the `start_server` function.
///
/// The `start_server` function is just a shortcut for `Server::new` followed with `run`. See the
/// documentation of the `start_server` function for more details about the handler.
///
/// # Example
///
/// ```no_run
/// use rouille::Server;
/// use rouille::Response;
///
/// let server = Server::new("localhost:0", |request| {
///     Response::text("hello world")
/// }).unwrap();
/// println!("Listening on {:?}", server.server_addr());
/// server.run();
/// ```
pub struct Server<F> {
    server: tiny_http::Server,
    handler: Arc<AssertUnwindSafe<F>>,
    executor: Executor,
}

impl<F> Server<F>
where
    F: Send + Sync + 'static + Fn(&Request) -> Response,
{
    /// Builds a new `Server` object.
    ///
    /// After this function returns, the HTTP server is listening.
    ///
    /// Returns an error if there was an error while creating the listening socket, for example if
    /// the port is already in use.
    pub fn new<A>(addr: A, handler: F) -> Result<Server<F>, Box<dyn Error + Send + Sync + 'static>>
    where
        A: ToSocketAddrs,
    {
        let server = tiny_http::Server::http(addr)?;
        Ok(Server {
            server,
            executor: Executor::default(),
            handler: Arc::new(AssertUnwindSafe(handler)), // TODO: using AssertUnwindSafe here is wrong, but unwind safety has some usability problems in Rust in general
        })
    }

    /// Builds a new `Server` object with SSL support.
    ///
    /// After this function returns, the HTTPS server is listening.
    ///
    /// Returns an error if there was an error while creating the listening socket, for example if
    /// the port is already in use.
    #[cfg(any(feature = "ssl", feature = "rustls"))]
    pub fn new_ssl<A>(
        addr: A,
        handler: F,
        certificate: Vec<u8>,
        private_key: Vec<u8>,
    ) -> Result<Server<F>, Box<dyn Error + Send + Sync + 'static>>
    where
        A: ToSocketAddrs,
    {
        let ssl_config = tiny_http::SslConfig {
            certificate,
            private_key,
        };
        let server = tiny_http::Server::https(addr, ssl_config)?;
        Ok(Server {
            server,
            executor: Executor::default(),
            handler: Arc::new(AssertUnwindSafe(handler)), // TODO: using AssertUnwindSafe here is wrong, but unwind safety has some usability problems in Rust in general
        })
    }

    /// Use a `ThreadPool` of the given size to process requests
    ///
    /// `pool_size` must be greater than zero or this function will panic.
    pub fn pool_size(mut self, pool_size: usize) -> Self {
        self.executor = Executor::with_size(pool_size);
        self
    }

    /// Returns the address of the listening socket.
    #[inline]
    pub fn server_addr(&self) -> SocketAddr {
        self.server
            .server_addr()
            .to_ip()
            .expect("Unexpected Unix socket listener")
    }

    /// Runs the server forever, or until the listening socket is somehow force-closed by the
    /// operating system.
    #[inline]
    pub fn run(self) {
        for request in self.server.incoming_requests() {
            self.process(request);
        }
    }

    /// Processes all the client requests waiting to be processed, then returns.
    ///
    /// This function executes very quickly, as each client requests that needs to be processed
    /// is processed in a separate thread.
    #[inline]
    pub fn poll(&self) {
        while let Ok(Some(request)) = self.server.try_recv() {
            self.process(request);
        }
    }

    /// Creates a new thread for the server that can be gracefully stopped later.
    ///
    /// This function returns a tuple of a `JoinHandle` and a `Sender`.
    /// You must call `JoinHandle::join()` otherwise the server will not run until completion.
    /// The server can be stopped at will by sending it an empty `()` message from another thread.
    /// There may be a maximum of a 1 second delay between sending the stop message and the server
    /// stopping. This delay may be shortened in future.
    ///
    /// ```no_run
    /// use std::thread;
    /// use std::time::Duration;
    /// use rouille::Server;
    /// use rouille::Response;
    ///
    /// let server = Server::new("localhost:0", |request| {
    ///     Response::text("hello world")
    /// }).unwrap();
    /// println!("Listening on {:?}", server.server_addr());
    /// let (handle, sender) = server.stoppable();
    ///
    /// // Stop the server in 3 seconds
    /// thread::spawn(move || {
    ///     thread::sleep(Duration::from_secs(3));
    ///     sender.send(()).unwrap();
    /// });
    ///
    /// // Block the main thread until the server is stopped
    /// handle.join().unwrap();
    /// ```
    #[inline]
    pub fn stoppable(self) -> (thread::JoinHandle<()>, mpsc::Sender<()>) {
        let (tx, rx) = mpsc::channel();
        let handle = thread::spawn(move || {
            while rx.try_recv().is_err() {
                // In order to reduce CPU load wait 1s for a recv before looping again
                while let Ok(Some(request)) = self.server.recv_timeout(Duration::from_secs(1)) {
                    self.process(request);
                }
            }
        });

        (handle, tx)
    }

    /// Same as `poll()` but blocks for at most `duration` before returning.
    ///
    /// This function can be used implement a custom server loop in a more CPU-efficient manner
    /// than calling `poll`.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use rouille::Server;
    /// use rouille::Response;
    ///
    /// let server = Server::new("localhost:0", |request| {
    ///     Response::text("hello world")
    /// }).unwrap();
    /// println!("Listening on {:?}", server.server_addr());
    ///
    /// loop {
    ///     server.poll_timeout(std::time::Duration::from_millis(100));
    /// }
    /// ```
    #[inline]
    pub fn poll_timeout(&self, dur: std::time::Duration) {
        while let Ok(Some(request)) = self.server.recv_timeout(dur) {
            self.process(request);
        }
    }

    /// Waits for all in-flight requests to be processed. This is useful for implementing a graceful
    /// shutdown.
    ///
    /// Note: new connections may still be accepted while we wait, and this function does not guarantee
    /// to wait for those new requests. To implement a graceful shutdown or a clean rolling-update,
    /// the following approach should be used:
    ///
    /// 1) Stop routing requests to this server. For a rolling update, requests should be routed
    ///    to the new instance. This logic typically sits outside of your application.
    ///
    /// 2) Drain the queue of incoming connections by calling `poll_timeout` with a short timeout.
    ///
    /// 3) Wait for in-flight requests to be processed by using this method.
    ///
    /// # Example
    /// ```no_run
    /// # use std::time::Duration;
    /// # use rouille::Server;
    /// #
    /// # let server = Server::new("", |_| unimplemented!()).unwrap();
    /// # fn is_stopping() -> bool { unimplemented!() }
    ///
    /// // Accept connections until we receive a SIGTERM
    /// while !is_stopping() {
    ///     server.poll_timeout(Duration::from_millis(100));
    /// }
    ///
    /// // We received a SIGTERM, but there still may be some queued connections,
    /// // so wait for them to be accepted.
    /// println!("Shutting down gracefully...");
    /// server.poll_timeout(Duration::from_millis(100));
    ///
    /// // We can expect there to be no more queued connections now, but slow requests
    /// // may still be in-flight, so wait for them to finish.
    /// server.join();
    /// ```
    pub fn join(&self) {
        self.executor.join();
    }

    // Internal function, called when we got a request from tiny-http that needs to be processed.
    fn process(&self, request: tiny_http::Request) {
        // We spawn a thread so that requests are processed in parallel.
        let handler = self.handler.clone();
        self.executor.execute(|| {
            // Small helper struct that makes it possible to put
            // a `tiny_http::Request` inside a `Box<Read>`.
            struct RequestRead(Arc<Mutex<Option<tiny_http::Request>>>);
            impl Read for RequestRead {
                #[inline]
                fn read(&mut self, buf: &mut [u8]) -> IoResult<usize> {
                    self.0
                        .lock()
                        .unwrap()
                        .as_mut()
                        .unwrap()
                        .as_reader()
                        .read(buf)
                }
            }

            // Building the `Request` object.
            let tiny_http_request;
            let rouille_request = {
                let url = request.url().to_owned();
                let method = request.method().as_str().to_owned();
                let headers = request
                    .headers()
                    .iter()
                    .map(|h| (h.field.to_string(), h.value.clone().into()))
                    .collect();
                let remote_addr = request.remote_addr().copied();

                tiny_http_request = Arc::new(Mutex::new(Some(request)));
                let data = Arc::new(Mutex::new(Some(
                    Box::new(RequestRead(tiny_http_request.clone())) as Box<_>,
                )));

                Request {
                    url,
                    method,
                    headers,
                    https: false,
                    data,
                    remote_addr,
                }
            };

            // Calling the handler ; this most likely takes a lot of time.
            // If the handler panics, we build a dummy response.
            let mut rouille_response = {
                // We don't use the `rouille_request` anymore after the panic, so it's ok to assert
                // it's unwind safe.
                let rouille_request = AssertUnwindSafe(rouille_request);
                let res = panic::catch_unwind(move || {
                    let rouille_request = rouille_request;
                    handler(&rouille_request)
                });

                match res {
                    Ok(r) => r,
                    Err(_) => Response::html(
                        "<h1>Internal Server Error</h1>\
                                        <p>An internal error has occurred on the server.</p>",
                    )
                    .with_status_code(500),
                }
            };

            // writing the response
            let (res_data, res_len) = rouille_response.data.into_reader_and_size();
            let mut response = tiny_http::Response::empty(rouille_response.status_code)
                .with_data(res_data, res_len);

            let mut upgrade_header = "".into();

            for (key, value) in rouille_response.headers {
                if key.eq_ignore_ascii_case("Content-Length") {
                    continue;
                }

                if key.eq_ignore_ascii_case("Upgrade") {
                    upgrade_header = value;
                    continue;
                }

                if let Ok(header) = tiny_http::Header::from_bytes(key.as_bytes(), value.as_bytes())
                {
                    response.add_header(header);
                } else {
                    // TODO: ?
                }
            }

            if let Some(ref mut upgrade) = rouille_response.upgrade {
                let trq = tiny_http_request.lock().unwrap().take().unwrap();
                let socket = trq.upgrade(&upgrade_header, response);
                upgrade.build(socket);
            } else {
                // We don't really care if we fail to send the response to the client, as there's
                // nothing we can do anyway.
                let _ = tiny_http_request
                    .lock()
                    .unwrap()
                    .take()
                    .unwrap()
                    .respond(response);
            }
        });
    }
}

/// Trait for objects that can take ownership of a raw connection to the client data.
///
/// The purpose of this trait is to be used with the `Connection: Upgrade` header, hence its name.
pub trait Upgrade {
    /// Initializes the object with the given socket.
    fn build(&mut self, socket: Box<dyn ReadWrite + Send>);
}

/// Represents a request that your handler must answer to.
///
/// This can be either a real request (received by the HTTP server) or a mock object created with
/// one of the `fake_*` constructors.
pub struct Request {
    method: String,
    url: String,
    headers: Vec<(String, String)>,
    https: bool,
    data: Arc<Mutex<Option<Box<dyn Read + Send>>>>,
    remote_addr: Option<SocketAddr>,
}

impl fmt::Debug for Request {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("Request")
            .field("method", &self.method)
            .field("url", &self.url)
            .field("headers", &self.headers)
            .field("https", &self.https)
            .field("remote_addr", &self.remote_addr)
            .finish()
    }
}

impl Request {
    /// Builds a fake HTTP request to be used during tests.
    ///
    /// The remote address of the client will be `127.0.0.1:12345`. Use `fake_http_from` to
    /// specify what the client's address should be.
    pub fn fake_http<U, M>(
        method: M,
        url: U,
        headers: Vec<(String, String)>,
        data: Vec<u8>,
    ) -> Request
    where
        U: Into<String>,
        M: Into<String>,
    {
        let data = Arc::new(Mutex::new(Some(Box::new(Cursor::new(data)) as Box<_>)));
        let remote_addr = Some("127.0.0.1:12345".parse().unwrap());

        Request {
            url: url.into(),
            method: method.into(),
            https: false,
            data,
            headers,
            remote_addr,
        }
    }

    /// Builds a fake HTTP request to be used during tests.
    pub fn fake_http_from<U, M>(
        from: SocketAddr,
        method: M,
        url: U,
        headers: Vec<(String, String)>,
        data: Vec<u8>,
    ) -> Request
    where
        U: Into<String>,
        M: Into<String>,
    {
        let data = Arc::new(Mutex::new(Some(Box::new(Cursor::new(data)) as Box<_>)));

        Request {
            url: url.into(),
            method: method.into(),
            https: false,
            data,
            headers,
            remote_addr: Some(from),
        }
    }

    /// Builds a fake HTTPS request to be used during tests.
    ///
    /// The remote address of the client will be `127.0.0.1:12345`. Use `fake_https_from` to
    /// specify what the client's address should be.
    pub fn fake_https<U, M>(
        method: M,
        url: U,
        headers: Vec<(String, String)>,
        data: Vec<u8>,
    ) -> Request
    where
        U: Into<String>,
        M: Into<String>,
    {
        let data = Arc::new(Mutex::new(Some(Box::new(Cursor::new(data)) as Box<_>)));
        let remote_addr = Some("127.0.0.1:12345".parse().unwrap());

        Request {
            url: url.into(),
            method: method.into(),
            https: true,
            data,
            headers,
            remote_addr,
        }
    }

    /// Builds a fake HTTPS request to be used during tests.
    pub fn fake_https_from<U, M>(
        from: SocketAddr,
        method: M,
        url: U,
        headers: Vec<(String, String)>,
        data: Vec<u8>,
    ) -> Request
    where
        U: Into<String>,
        M: Into<String>,
    {
        let data = Arc::new(Mutex::new(Some(Box::new(Cursor::new(data)) as Box<_>)));

        Request {
            url: url.into(),
            method: method.into(),
            https: true,
            data,
            headers,
            remote_addr: Some(from),
        }
    }

    /// If the decoded URL of the request starts with `prefix`, builds a new `Request` that is
    /// the same as the original but without that prefix.
    ///
    /// # Example
    ///
    /// ```
    /// # use rouille::Request;
    /// # use rouille::Response;
    /// fn handle(request: &Request) -> Response {
    ///     if let Some(request) = request.remove_prefix("/static") {
    ///         return rouille::match_assets(&request, "/static");
    ///     }
    ///
    ///     // ...
    ///     # panic!()
    /// }
    /// ```
    pub fn remove_prefix(&self, prefix: &str) -> Option<Request> {
        if !self.url().starts_with(prefix) {
            return None;
        }

        // TODO: url-encoded characters in the prefix are not implemented
        assert!(self.url.starts_with(prefix));
        Some(Request {
            method: self.method.clone(),
            url: self.url[prefix.len()..].to_owned(),
            headers: self.headers.clone(), // TODO: expensive
            https: self.https,
            data: self.data.clone(),
            remote_addr: self.remote_addr,
        })
    }

    /// Returns `true` if the request uses HTTPS, and `false` if it uses HTTP.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::{Request, Response};
    ///
    /// fn handle(request: &Request) -> Response {
    ///     if !request.is_secure() {
    ///         return Response::redirect_303(format!("https://example.com"));
    ///     }
    ///
    ///     // ...
    /// # panic!()
    /// }
    /// ```
    #[inline]
    pub fn is_secure(&self) -> bool {
        self.https
    }

    /// Returns the method of the request (`GET`, `POST`, etc.).
    #[inline]
    pub fn method(&self) -> &str {
        &self.method
    }

    /// Returns the raw URL requested by the client. It is not decoded and thus can contain strings
    /// such as `%20`, and the query parameters such as `?p=hello`.
    ///
    /// See also `url()`.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Request;
    ///
    /// let request = Request::fake_http("GET", "/hello%20world?foo=bar", vec![], vec![]);
    /// assert_eq!(request.raw_url(), "/hello%20world?foo=bar");
    /// ```
    #[inline]
    pub fn raw_url(&self) -> &str {
        &self.url
    }

    /// Returns the raw query string requested by the client. In other words, everything after the
    /// first `?` in the raw url.
    ///
    /// Returns the empty string if no query string.
    #[inline]
    pub fn raw_query_string(&self) -> &str {
        if let Some(pos) = self.url.bytes().position(|c| c == b'?') {
            self.url.split_at(pos + 1).1
        } else {
            ""
        }
    }

    /// Returns the URL requested by the client.
    ///
    /// Contrary to `raw_url`, special characters have been decoded and the query string
    /// (eg `?p=hello`) has been removed.
    ///
    /// If there is any non-unicode character in the URL, it will be replaced with `U+FFFD`.
    ///
    /// > **Note**: This function will decode the token `%2F` will be decoded as `/`. However the
    /// > official specifications say that such a token must not count as a delimiter for URL paths.
    /// > In other words, `/hello/world` is not the same as `/hello%2Fworld`.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Request;
    ///
    /// let request = Request::fake_http("GET", "/hello%20world?foo=bar", vec![], vec![]);
    /// assert_eq!(request.url(), "/hello world");
    /// ```
    pub fn url(&self) -> String {
        let url = self.url.as_bytes();
        let url = if let Some(pos) = url.iter().position(|&c| c == b'?') {
            &url[..pos]
        } else {
            url
        };

        percent_encoding::percent_decode(url)
            .decode_utf8_lossy()
            .into_owned()
    }

    /// Returns the value of a GET parameter or None if it doesn't exist.
    pub fn get_param(&self, param_name: &str) -> Option<String> {
        let name_pattern = &format!("{}=", param_name);
        let param_pairs = self.raw_query_string().split('&');
        param_pairs
            .filter(|pair| pair.starts_with(name_pattern) || pair == &param_name)
            .map(|pair| pair.split('=').nth(1).unwrap_or(""))
            .next()
            .map(|value| {
                percent_encoding::percent_decode(value.replace('+', " ").as_bytes())
                    .decode_utf8_lossy()
                    .into_owned()
            })
    }

    /// Returns the value of a header of the request.
    ///
    /// Returns `None` if no such header could be found.
    #[inline]
    pub fn header(&self, key: &str) -> Option<&str> {
        self.headers
            .iter()
            .find(|&&(ref k, _)| k.eq_ignore_ascii_case(key))
            .map(|&(_, ref v)| &v[..])
    }

    /// Returns a list of all the headers of the request.
    #[inline]
    pub fn headers(&self) -> HeadersIter {
        HeadersIter {
            iter: self.headers.iter(),
        }
    }

    /// Returns the state of the `DNT` (Do Not Track) header.
    ///
    /// If the header is missing or is malformed, `None` is returned. If the header exists,
    /// `Some(true)` is returned if `DNT` is `1` and `Some(false)` is returned if `DNT` is `0`.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::{Request, Response};
    ///
    /// # fn track_user(request: &Request) {}
    /// fn handle(request: &Request) -> Response {
    ///     if !request.do_not_track().unwrap_or(false) {
    ///         track_user(&request);
    ///     }
    ///
    ///     // ...
    /// # panic!()
    /// }
    /// ```
    pub fn do_not_track(&self) -> Option<bool> {
        match self.header("DNT") {
            Some(h) if h == "1" => Some(true),
            Some(h) if h == "0" => Some(false),
            _ => None,
        }
    }

    /// Returns the body of the request.
    ///
    /// The body can only be retrieved once. Returns `None` is the body has already been retrieved
    /// before.
    ///
    /// # Example
    ///
    /// ```
    /// use std::io::Read;
    /// use rouille::{Request, Response, ResponseBody};
    ///
    /// fn echo(request: &Request) -> Response {
    ///     let mut data = request.data().expect("Oops, body already retrieved, problem \
    ///                                           in the server");
    ///
    ///     let mut buf = Vec::new();
    ///     match data.read_to_end(&mut buf) {
    ///         Ok(_) => (),
    ///         Err(_) => return Response::text("Failed to read body")
    ///     };
    ///
    ///     Response {
    ///         data: ResponseBody::from_data(buf),
    ///         .. Response::text("")
    ///     }
    /// }
    /// ```
    pub fn data(&self) -> Option<RequestBody> {
        let reader = self.data.lock().unwrap().take();
        reader.map(|r| RequestBody {
            body: r,
            marker: PhantomData,
        })
    }

    /// Returns the address of the client that made this request.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::{Request, Response};
    ///
    /// fn handle(request: &Request) -> Response {
    ///     Response::text(format!("Your IP is: {:?}", request.remote_addr()))
    /// }
    /// ```
    #[inline]
    pub fn remote_addr(&self) -> &SocketAddr {
        self.remote_addr
            .as_ref()
            .expect("Unexpected Unix socket for request")
    }
}

/// Iterator to the list of headers in a request.
#[derive(Debug, Clone)]
pub struct HeadersIter<'a> {
    iter: SliceIter<'a, (String, String)>,
}

impl<'a> Iterator for HeadersIter<'a> {
    type Item = (&'a str, &'a str);

    #[inline]
    fn next(&mut self) -> Option<Self::Item> {
        self.iter.next().map(|&(ref k, ref v)| (&k[..], &v[..]))
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        self.iter.size_hint()
    }
}

impl<'a> ExactSizeIterator for HeadersIter<'a> {}

/// Gives access to the body of a request.
///
/// In order to obtain this object, call `request.data()`.
pub struct RequestBody<'a> {
    body: Box<dyn Read + Send>,
    marker: PhantomData<&'a ()>,
}

impl<'a> Read for RequestBody<'a> {
    #[inline]
    fn read(&mut self, buf: &mut [u8]) -> IoResult<usize> {
        self.body.read(buf)
    }
}

#[cfg(test)]
mod tests {
    use Request;

    #[test]
    fn header() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Host".to_owned(), "localhost".to_owned())],
            vec![],
        );
        assert_eq!(request.header("Host"), Some("localhost"));
        assert_eq!(request.header("host"), Some("localhost"));
    }

    #[test]
    fn get_param() {
        let request = Request::fake_http("GET", "/?p=hello", vec![], vec![]);
        assert_eq!(request.get_param("p"), Some("hello".to_owned()));
    }

    #[test]
    fn get_param_multiple_param() {
        let request = Request::fake_http("GET", "/?foo=bar&message=hello", vec![], vec![]);
        assert_eq!(request.get_param("message"), Some("hello".to_owned()));
    }
    #[test]
    fn get_param_no_match() {
        let request = Request::fake_http("GET", "/?hello=world", vec![], vec![]);
        assert_eq!(request.get_param("foo"), None);
    }

    #[test]
    fn get_param_partial_suffix_match() {
        let request = Request::fake_http("GET", "/?hello=world", vec![], vec![]);
        assert_eq!(request.get_param("lo"), None);
    }

    #[test]
    fn get_param_partial_prefix_match() {
        let request = Request::fake_http("GET", "/?hello=world", vec![], vec![]);
        assert_eq!(request.get_param("he"), None);
    }

    #[test]
    fn get_param_superstring_match() {
        let request = Request::fake_http("GET", "/?jan=01", vec![], vec![]);
        assert_eq!(request.get_param("january"), None);
    }

    #[test]
    fn get_param_flag_with_equals() {
        let request = Request::fake_http("GET", "/?flag=", vec![], vec![]);
        assert_eq!(request.get_param("flag"), Some("".to_owned()));
    }

    #[test]
    fn get_param_flag_without_equals() {
        let request = Request::fake_http("GET", "/?flag", vec![], vec![]);
        assert_eq!(request.get_param("flag"), Some("".to_owned()));
    }

    #[test]
    fn get_param_flag_with_multiple_params() {
        let request = Request::fake_http("GET", "/?flag&foo=bar", vec![], vec![]);
        assert_eq!(request.get_param("flag"), Some("".to_owned()));
    }

    #[test]
    fn body_twice() {
        let request = Request::fake_http("GET", "/", vec![], vec![62, 62, 62]);
        assert!(request.data().is_some());
        assert!(request.data().is_none());
    }

    #[test]
    fn url_strips_get_query() {
        let request = Request::fake_http("GET", "/?p=hello", vec![], vec![]);
        assert_eq!(request.url(), "/");
    }

    #[test]
    fn urlencode_query_string() {
        let request = Request::fake_http("GET", "/?p=hello%20world", vec![], vec![]);
        assert_eq!(request.get_param("p"), Some("hello world".to_owned()));
    }

    #[test]
    fn plus_in_query_string() {
        let request = Request::fake_http("GET", "/?p=hello+world", vec![], vec![]);
        assert_eq!(request.get_param("p"), Some("hello world".to_owned()));
    }

    #[test]
    fn encoded_plus_in_query_string() {
        let request = Request::fake_http("GET", "/?p=hello%2Bworld", vec![], vec![]);
        assert_eq!(request.get_param("p"), Some("hello+world".to_owned()));
    }

    #[test]
    fn url_encode() {
        let request = Request::fake_http("GET", "/hello%20world", vec![], vec![]);
        assert_eq!(request.url(), "/hello world");
    }

    #[test]
    fn plus_in_url() {
        let request = Request::fake_http("GET", "/hello+world", vec![], vec![]);
        assert_eq!(request.url(), "/hello+world");
    }

    #[test]
    fn dnt() {
        let request =
            Request::fake_http("GET", "/", vec![("DNT".to_owned(), "1".to_owned())], vec![]);
        assert_eq!(request.do_not_track(), Some(true));

        let request =
            Request::fake_http("GET", "/", vec![("DNT".to_owned(), "0".to_owned())], vec![]);
        assert_eq!(request.do_not_track(), Some(false));

        let request = Request::fake_http("GET", "/", vec![], vec![]);
        assert_eq!(request.do_not_track(), None);

        let request = Request::fake_http(
            "GET",
            "/",
            vec![("DNT".to_owned(), "malformed".to_owned())],
            vec![],
        );
        assert_eq!(request.do_not_track(), None);
    }
}
