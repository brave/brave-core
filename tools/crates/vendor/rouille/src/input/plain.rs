// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

use std::error;
use std::fmt;
use std::io::Error as IoError;
use std::io::Read;
use Request;

/// Error that can happen when parsing the request body as plain text.
#[derive(Debug)]
pub enum PlainTextError {
    /// Can't parse the body of the request because it was already extracted.
    BodyAlreadyExtracted,

    /// Wrong content type.
    WrongContentType,

    /// Could not read the body from the request.
    IoError(IoError),

    /// The limit to the number of bytes has been exceeded.
    LimitExceeded,

    /// The content-type encoding is not ASCII or UTF-8, or the body is not valid UTF-8.
    NotUtf8,
}

impl From<IoError> for PlainTextError {
    fn from(err: IoError) -> PlainTextError {
        PlainTextError::IoError(err)
    }
}

impl error::Error for PlainTextError {
    #[inline]
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match *self {
            PlainTextError::IoError(ref e) => Some(e),
            _ => None,
        }
    }
}

impl fmt::Display for PlainTextError {
    #[inline]
    fn fmt(&self, fmt: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        let description = match *self {
            PlainTextError::BodyAlreadyExtracted => "the body of the request was already extracted",
            PlainTextError::WrongContentType => "the request didn't have a plain text content type",
            PlainTextError::IoError(_) => {
                "could not read the body from the request, or could not execute the CGI program"
            }
            PlainTextError::LimitExceeded => "the limit to the number of bytes has been exceeded",
            PlainTextError::NotUtf8 => {
                "the content-type encoding is not ASCII or UTF-8, or the body is not valid UTF-8"
            }
        };

        write!(fmt, "{}", description)
    }
}

/// Read plain text data from the body of a request.
///
/// Returns an error if the content-type of the request is not text/plain. Only the UTF-8 encoding
/// is supported. You will get an error if the client passed non-UTF8 data.
///
/// If the body of the request exceeds 1MB of data, an error is returned to prevent a malicious
/// client from crashing the server. Use the `plain_text_body_with_limit` function to customize
/// the limit.
///
/// # Example
///
/// ```
/// # #[macro_use] extern crate rouille;
/// # use rouille::{Request, Response};
/// # fn main() {}
/// fn route_handler(request: &Request) -> Response {
///     let text = try_or_400!(rouille::input::plain_text_body(request));
///     Response::text(format!("you sent: {}", text))
/// }
/// ```
///
#[inline]
pub fn plain_text_body(request: &Request) -> Result<String, PlainTextError> {
    plain_text_body_with_limit(request, 1024 * 1024)
}

/// Reads plain text data from the body of a request.
///
/// This does the same as `plain_text_body`, but with a customizable limit in bytes to how much
/// data will be read from the request. If the limit is exceeded, a `LimitExceeded` error is
/// returned.
pub fn plain_text_body_with_limit(
    request: &Request,
    limit: usize,
) -> Result<String, PlainTextError> {
    // TODO: handle encoding ; return NotUtf8 if a non-utf8 charset is sent
    // if no encoding is specified by the client, the default is `US-ASCII` which is compatible with UTF8

    if let Some(header) = request.header("Content-Type") {
        if !header.starts_with("text/plain") {
            return Err(PlainTextError::WrongContentType);
        }
    } else {
        return Err(PlainTextError::WrongContentType);
    }

    let body = match request.data() {
        Some(b) => b,
        None => return Err(PlainTextError::BodyAlreadyExtracted),
    };

    let mut out = Vec::new();
    body.take(limit.saturating_add(1) as u64)
        .read_to_end(&mut out)?;
    if out.len() > limit {
        return Err(PlainTextError::LimitExceeded);
    }

    let out = match String::from_utf8(out) {
        Ok(o) => o,
        Err(_) => return Err(PlainTextError::NotUtf8),
    };

    Ok(out)
}

#[cfg(test)]
mod test {
    use super::plain_text_body;
    use super::plain_text_body_with_limit;
    use super::PlainTextError;
    use Request;

    #[test]
    fn ok() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Content-Type".to_owned(), "text/plain".to_owned())],
            b"test".to_vec(),
        );

        match plain_text_body(&request) {
            Ok(ref d) if d == "test" => (),
            _ => panic!(),
        }
    }

    #[test]
    fn charset() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![(
                "Content-Type".to_owned(),
                "text/plain; charset=utf8".to_owned(),
            )],
            b"test".to_vec(),
        );

        match plain_text_body(&request) {
            Ok(ref d) if d == "test" => (),
            _ => panic!(),
        }
    }

    #[test]
    fn missing_content_type() {
        let request = Request::fake_http("GET", "/", vec![], Vec::new());

        match plain_text_body(&request) {
            Err(PlainTextError::WrongContentType) => (),
            _ => panic!(),
        }
    }

    #[test]
    fn wrong_content_type() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Content-Type".to_owned(), "text/html".to_owned())],
            b"test".to_vec(),
        );

        match plain_text_body(&request) {
            Err(PlainTextError::WrongContentType) => (),
            _ => panic!(),
        }
    }

    #[test]
    fn body_twice() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![(
                "Content-Type".to_owned(),
                "text/plain; charset=utf8".to_owned(),
            )],
            b"test".to_vec(),
        );

        match plain_text_body(&request) {
            Ok(ref d) if d == "test" => (),
            _ => panic!(),
        }

        match plain_text_body(&request) {
            Err(PlainTextError::BodyAlreadyExtracted) => (),
            _ => panic!(),
        }
    }

    #[test]
    fn bytes_limit() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Content-Type".to_owned(), "text/plain".to_owned())],
            b"test".to_vec(),
        );

        match plain_text_body_with_limit(&request, 2) {
            Err(PlainTextError::LimitExceeded) => (),
            _ => panic!(),
        }
    }

    #[test]
    fn exact_limit() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Content-Type".to_owned(), "text/plain".to_owned())],
            b"test".to_vec(),
        );

        match plain_text_body_with_limit(&request, 4) {
            Ok(ref d) if d == "test" => (),
            _ => panic!(),
        }
    }

    #[test]
    fn non_utf8_body() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![(
                "Content-Type".to_owned(),
                "text/plain; charset=utf8".to_owned(),
            )],
            b"\xc3\x28".to_vec(),
        );

        match plain_text_body(&request) {
            Err(PlainTextError::NotUtf8) => (),
            _ => panic!(),
        }
    }

    #[test]
    #[ignore] // TODO: not implemented
    fn non_utf8_encoding() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![(
                "Content-Type".to_owned(),
                "text/plain; charset=iso-8859-1".to_owned(),
            )],
            b"test".to_vec(),
        );

        match plain_text_body(&request) {
            Err(PlainTextError::NotUtf8) => (),
            _ => panic!(),
        }
    }
}
