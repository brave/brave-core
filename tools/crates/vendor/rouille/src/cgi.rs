// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

//! Allows you to let an external process handle the request through CGI.
//!
//! This module provides a trait named `CgiRun` which is implemented on `std::process::Command`.
//! In order to dispatch a request, simply start building a `Command` object and call `start_cgi`
//! on it.
//!
//! ## Example
//!
//! ```no_run
//! use std::process::Command;
//! use rouille::cgi::CgiRun;
//!
//! rouille::start_server("localhost:8080", move |request| {
//!     Command::new("php-cgi").start_cgi(request).unwrap()
//! });
//! ```
//!
//! # About the Result returned by start_cgi
//!
//! The `start_cgi` method returns a `Result<Response, std::io::Error>`. This object will contain
//! an error if and only if there was a problem executing the command (for example if it fails to
//! start, or starts then crashes, ...).
//!
//! If the process returns an error 400 or an error 404 for example, then the result will contain
//! `Ok`.
//!
//! It is therefore appropriate to simply call `.unwrap()` on that result. Any panic will be turned
//! into an error 500 and add an entry to the logs, which is probably what you want when your
//! server is misconfigured.

use std::error;
use std::fmt;
use std::io;
use std::io::BufRead;
use std::io::Error as IoError;
use std::io::Read;
use std::process::Command;
use std::process::Stdio;

use Request;
use Response;
use ResponseBody;

/// Error that can happen when parsing the JSON input.
#[derive(Debug)]
pub enum CgiError {
    /// Can't pass through the body of the request because it was already extracted.
    BodyAlreadyExtracted,

    /// Could not read the body from the request, or could not execute the CGI program.
    IoError(IoError),
}

impl From<IoError> for CgiError {
    fn from(err: IoError) -> CgiError {
        CgiError::IoError(err)
    }
}

impl error::Error for CgiError {
    #[inline]
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match *self {
            CgiError::IoError(ref e) => Some(e),
            _ => None,
        }
    }
}

impl fmt::Display for CgiError {
    #[inline]
    fn fmt(&self, fmt: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        let description = match *self {
            CgiError::BodyAlreadyExtracted => "the body of the request was already extracted",
            CgiError::IoError(_) => {
                "could not read the body from the request, or could not execute the CGI program"
            }
        };

        write!(fmt, "{}", description)
    }
}

pub trait CgiRun {
    /// Dispatches a request to the process.
    ///
    /// This function modifies the `Command` to add all the required environment variables
    /// and the request's body, then executes the command and waits until the child process has
    /// returned all the headers of the response. Once the headers have been sent back, this
    /// function returns.
    ///
    /// The body of the returned `Response` will hold a handle to the child's stdout output. This
    /// means that the child can continue running in the background and send data to the client,
    /// even after you have finished handling the request.
    fn start_cgi(self, request: &Request) -> Result<Response, CgiError>;
}

impl CgiRun for Command {
    fn start_cgi(mut self, request: &Request) -> Result<Response, CgiError> {
        self.env("SERVER_SOFTWARE", "rouille")
            .env("SERVER_NAME", "localhost") // FIXME:
            .env("GATEWAY_INTERFACE", "CGI/1.1")
            .env("SERVER_PROTOCOL", "HTTP/1.1") // FIXME:
            .env("SERVER_PORT", "80") // FIXME:
            .env("REQUEST_METHOD", request.method())
            .env("PATH_INFO", &request.url()) // TODO: incorrect + what about PATH_TRANSLATED?
            .env("SCRIPT_NAME", "") // FIXME:
            .env("QUERY_STRING", request.raw_query_string())
            .env("REMOTE_ADDR", &request.remote_addr().to_string())
            .env("AUTH_TYPE", "") // FIXME:
            .env("REMOTE_USER", "") // FIXME:
            .env(
                "CONTENT_TYPE",
                &request.header("Content-Type").unwrap_or(""),
            )
            .env(
                "CONTENT_LENGTH",
                &request.header("Content-Length").unwrap_or(""),
            )
            .stdout(Stdio::piped())
            .stderr(Stdio::inherit())
            .stdin(Stdio::piped());

        // TODO: `HTTP_` env vars with the headers

        let mut child = self.spawn()?;

        if let Some(mut body) = request.data() {
            io::copy(&mut body, child.stdin.as_mut().unwrap())?;
        } else {
            return Err(CgiError::BodyAlreadyExtracted);
        }

        let response = {
            let mut stdout = io::BufReader::new(child.stdout.take().unwrap());

            let mut headers = Vec::new();
            let mut status_code = 200;
            for header in stdout.by_ref().lines() {
                let header = header?;
                if header.is_empty() {
                    break;
                }

                let (header, val) = header.split_once(':').unwrap();
                let val = &val[1..];

                if header == "Status" {
                    status_code = val[0..3]
                        .parse()
                        .expect("Status returned by CGI program is invalid");
                } else {
                    headers.push((header.to_owned().into(), val.to_owned().into()));
                }
            }

            Response {
                status_code,
                headers,
                data: ResponseBody::from_reader(stdout),
                upgrade: None,
            }
        };

        Ok(response)
    }
}
