// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

//! Dispatch a request to another HTTP server.
//!
//! This module provides functionalities to dispatch a request to another server. This can be
//! used to make rouille behave as a reverse proxy.
//!
//! This function call will return immediately after the remote server has finished sending its
//! headers. The socket to the remote will be stored in the `ResponseBody` of the response.
//!
//! # Proxy() vs full_proxy()
//!
//! The difference between `proxy()` and `full_proxy()` is that if the target server fails to
//! return a proper error, the `proxy()` function will return an error (in the form of a
//! `ProxyError`) while the `full_proxy()` will return a `Response` with a status code indicating
//! an error.
//!
//! The `full_proxy()` function will only return an error if the body was already extracted from
//! the request before it was called. Since this indicates a logic error in the code, it is a good
//! idea to `unwrap()` the `Result` returned by `full_proxy()`.
//!
//! # Example
//!
//! You can for example dispatch to a different server depending on the host requested by the
//! client.
//!
//! ```
//! use rouille::{Request, Response};
//! use rouille::proxy;
//!
//! fn handle_request(request: &Request) -> Response {
//!     let config = match request.header("Host") {
//!         Some(h) if h == "domain1.com" => {
//!             proxy::ProxyConfig {
//!                 addr: "domain1.handler.localnetwork",
//!                 replace_host: None,
//!             }
//!         },
//!
//!         Some(h) if h == "domain2.com" => {
//!             proxy::ProxyConfig {
//!                 addr: "domain2.handler.localnetwork",
//!                 replace_host: None,
//!             }
//!         },
//!
//!         _ => return Response::empty_404()
//!     };
//!
//!     proxy::full_proxy(request, config).unwrap()
//! }
//! ```

use std::borrow::Cow;
use std::error;
use std::fmt;
use std::io;
use std::io::BufRead;
use std::io::Error as IoError;
use std::io::Read;
use std::io::Write;
use std::net::TcpStream;
use std::net::ToSocketAddrs;
use std::time::Duration;

use Request;
use Response;
use ResponseBody;

/// Error that can happen when dispatching the request to another server.
#[derive(Debug)]
pub enum ProxyError {
    /// Can't pass through the body of the request because it was already extracted.
    BodyAlreadyExtracted,

    /// Could not read the body from the request, or could not connect to the remote server, or
    /// the connection to the remote server closed unexpectedly.
    IoError(IoError),

    /// The destination server didn't produce compliant HTTP.
    HttpParseError,
}

impl From<IoError> for ProxyError {
    fn from(err: IoError) -> ProxyError {
        ProxyError::IoError(err)
    }
}

impl error::Error for ProxyError {
    #[inline]
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match *self {
            ProxyError::IoError(ref e) => Some(e),
            _ => None,
        }
    }
}

impl fmt::Display for ProxyError {
    #[inline]
    fn fmt(&self, fmt: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        let description = match *self {
            ProxyError::BodyAlreadyExtracted => "the body of the request was already extracted",
            ProxyError::IoError(_) => {
                "could not read the body from the request, or could not connect to the remote \
                 server, or the connection to the remote server closed unexpectedly"
            }
            ProxyError::HttpParseError => "the destination server didn't produce compliant HTTP",
        };

        write!(fmt, "{}", description)
    }
}

/// Configuration for the reverse proxy.
#[derive(Debug, Clone)]
pub struct ProxyConfig<A> {
    /// The address to connect to. For example `example.com:80`.
    pub addr: A,
    /// If `Some`, the `Host` header will be replaced with this value.
    pub replace_host: Option<Cow<'static, str>>,
}

/// Sends the request to another HTTP server using the configuration.
///
/// If the function fails to get a response from the target, an error is returned. If you want
/// to instead return a response with a status code such as 502 (`Bad Gateway`) or 504
/// (`Gateway Time-out`), see `full_proxy`.
///
/// > **Note**: Implementation is very hacky for the moment.
///
/// > **Note**: SSL is not supported.
// TODO: ^
pub fn proxy<A>(request: &Request, config: ProxyConfig<A>) -> Result<Response, ProxyError>
where
    A: ToSocketAddrs,
{
    let mut socket = TcpStream::connect(config.addr)?;
    socket.set_read_timeout(Some(Duration::from_secs(60)))?;
    socket.set_write_timeout(Some(Duration::from_secs(60)))?;

    let mut data = match request.data() {
        Some(d) => d,
        None => return Err(ProxyError::BodyAlreadyExtracted),
    };

    socket
        .write_all(format!("{} {} HTTP/1.1\r\n", request.method(), request.raw_url()).as_bytes())?;
    for (header, value) in request.headers() {
        let value = if header == "Host" {
            if let Some(ref replace) = config.replace_host {
                &**replace
            } else {
                value
            }
        } else {
            value
        };
        if header == "Connection" {
            continue;
        }

        socket.write_all(format!("{}: {}\r\n", header, value).as_bytes())?;
    }
    socket.write_all(b"Connection: close\r\n\r\n")?;
    io::copy(&mut data, &mut socket)?;

    let mut socket = io::BufReader::new(socket);

    let mut headers = Vec::new();
    let status_code;
    {
        let mut lines = socket.by_ref().lines();

        {
            let line = match lines.next() {
                Some(l) => l,
                None => return Err(ProxyError::HttpParseError),
            }?;
            let mut splits = line.splitn(3, ' ');
            let _ = splits.next();
            let status_str = match splits.next() {
                Some(l) => l,
                None => return Err(ProxyError::HttpParseError),
            };
            status_code = match status_str.parse() {
                Ok(s) => s,
                Err(_) => return Err(ProxyError::HttpParseError),
            };
        }

        for header in lines {
            let header = header?;
            if header.is_empty() {
                break;
            }

            let mut splits = header.splitn(2, ':');
            let header = match splits.next() {
                Some(v) => v,
                None => return Err(ProxyError::HttpParseError),
            };
            let val = match splits.next() {
                Some(v) => v,
                None => return Err(ProxyError::HttpParseError),
            };
            let val = &val[1..];

            headers.push((header.to_owned().into(), val.to_owned().into()));
        }
    }

    Ok(Response {
        status_code,
        headers,
        data: ResponseBody::from_reader(socket),
        upgrade: None,
    })
}

/// Error that can happen when calling `full_proxy`.
#[derive(Debug)]
pub enum FullProxyError {
    /// Can't pass through the body of the request because it was already extracted.
    BodyAlreadyExtracted,
}

impl error::Error for FullProxyError {}

impl fmt::Display for FullProxyError {
    #[inline]
    fn fmt(&self, fmt: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        let description = match *self {
            FullProxyError::BodyAlreadyExtracted => "the body of the request was already extracted",
        };

        write!(fmt, "{}", description)
    }
}

/// Sends the request to another HTTP server using the configuration.
///
/// Contrary to `proxy`, if the server fails to return a proper response then a response is
/// generated with the status code 502 or 504.
///
/// The only possible remaining error is if the body of the request was already extracted. Since
/// this would be a logic error, it is acceptable to unwrap it.
pub fn full_proxy<A>(request: &Request, config: ProxyConfig<A>) -> Result<Response, FullProxyError>
where
    A: ToSocketAddrs,
{
    match proxy(request, config) {
        Ok(r) => Ok(r),
        Err(ProxyError::IoError(_)) => Ok(Response::text("Gateway Time-out").with_status_code(504)),
        Err(ProxyError::HttpParseError) => Ok(Response::text("Bad Gateway").with_status_code(502)),
        Err(ProxyError::BodyAlreadyExtracted) => Err(FullProxyError::BodyAlreadyExtracted),
    }
}
