// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

//! Apply content encodings (such as gzip compression) to the response.
//!
//! This module provides access to the content encodings supported by a request as well as
//! a function to automatically apply common content encodings to a response.
//! # Basic example
//!
//! Here is a basic example showing how to use content encodings:
//!
//! ```
//! use rouille::Request;
//! use rouille::Response;
//! use rouille::content_encoding;
//!
//! fn handle_request(request: &Request) -> Response {
//!     let response = Response::text("Hello world");
//!     content_encoding::apply(&request, response)
//! }
//! ```
use input;
use Request;
use Response;

/// Applies content encoding to the response.
///
/// Analyzes the `Accept-Encoding` header of the request. If one of the encodings is recognized and
/// supported by rouille, it adds a `Content-Encoding` header to the `Response` and encodes its
/// body.
///
/// If the response already has a `Content-Encoding` header, this function is a no-op.
/// If the response has a `Content-Type` header that isn't textual content, this function is a
/// no-op.
///
/// The gzip encoding is supported only if you enable the `gzip` feature of rouille (which is
/// enabled by default).
///
/// # Example
///
/// ```rust
/// use rouille::content_encoding;
/// use rouille::Request;
/// use rouille::Response;
///
/// fn handle(request: &Request) -> Response {
///     content_encoding::apply(request, Response::text("hello world"))
/// }
/// ```
pub fn apply(request: &Request, mut response: Response) -> Response {
    // Only text should be encoded. Otherwise just return.
    if !response_is_text(&response) {
        return response;
    }

    // If any of the response's headers is equal to `Content-Encoding`, ignore the function
    // call and return immediately.
    if response
        .headers
        .iter()
        .any(|&(ref key, _)| key.eq_ignore_ascii_case("Content-Encoding"))
    {
        return response;
    }

    // Now let's get the list of content encodings accepted by the request.
    // The list should be ordered from the most desired to the least desired.
    let encoding_preference = ["br", "gzip", "x-gzip", "identity"];
    let accept_encoding_header = request.header("Accept-Encoding").unwrap_or("");
    if let Some(preferred_index) = input::priority_header_preferred(
        accept_encoding_header,
        encoding_preference.iter().cloned(),
    ) {
        match encoding_preference[preferred_index] {
            "br" => brotli(&mut response),
            "gzip" | "x-gzip" => gzip(&mut response),
            _ => (),
        }
    }

    response
}

// Returns true if the Content-Type of the response is a type that should be encoded.
// Since encoding is purely an optimization, it's not a problem if the function sometimes has
// false positives or false negatives.
fn response_is_text(response: &Response) -> bool {
    response.headers.iter().any(|&(ref key, ref value)| {
        if !key.eq_ignore_ascii_case("Content-Type") {
            return false;
        }

        let content_type = value.to_lowercase();
        content_type.starts_with("text/")
            || content_type.contains("javascript")
            || content_type.contains("json")
            || content_type.contains("xml")
            || content_type.contains("font")
    })
}

#[cfg(feature = "gzip")]
fn gzip(response: &mut Response) {
    use deflate::deflate_bytes_gzip;
    use std::io;
    use std::mem;
    use ResponseBody;

    response
        .headers
        .push(("Content-Encoding".into(), "gzip".into()));
    let previous_body = mem::replace(&mut response.data, ResponseBody::empty());
    let (mut raw_data, size) = previous_body.into_reader_and_size();
    let mut src = match size {
        Some(size) => Vec::with_capacity(size),
        None => Vec::new(),
    };
    io::copy(&mut raw_data, &mut src).expect("Failed reading response body while gzipping");
    let zipped = deflate_bytes_gzip(&src);
    response.data = ResponseBody::from_data(zipped);
}

#[cfg(not(feature = "gzip"))]
#[inline]
fn gzip(response: &mut Response) {}

#[cfg(feature = "brotli")]
fn brotli(response: &mut Response) {
    use brotli::enc::reader::CompressorReader;
    use std::mem;
    use ResponseBody;

    response
        .headers
        .push(("Content-Encoding".into(), "br".into()));
    let previous_body = mem::replace(&mut response.data, ResponseBody::empty());
    let (raw_data, _) = previous_body.into_reader_and_size();
    // Using default Brotli parameters: 0 buffer_size == 4096, compression level 6, lgwin == 22
    response.data = ResponseBody::from_reader(CompressorReader::new(raw_data, 0, 6, 22));
}

#[cfg(not(feature = "brotli"))]
#[inline]
fn brotli(response: &mut Response) {}

#[cfg(test)]
mod tests {
    use content_encoding;
    use Request;
    use Response;

    // TODO: more tests for encoding stuff
    #[test]
    fn text_response() {
        assert!(content_encoding::response_is_text(&Response::text("")));
    }

    #[test]
    fn non_text_response() {
        assert!(!content_encoding::response_is_text(&Response::from_data(
            "image/jpeg",
            ""
        )));
    }

    #[test]
    fn no_req_encodings() {
        let request = Request::fake_http("GET", "/", vec![], vec![]);
        let response = Response::html("<p>Hello world</p>");
        let encoded_response = content_encoding::apply(&request, response);
        assert!(!encoded_response
            .headers
            .iter()
            .any(|(header_name, _)| header_name == "Content-Encoding")); // No Content-Encoding header
        let mut encoded_content = vec![];
        encoded_response
            .data
            .into_reader_and_size()
            .0
            .read_to_end(&mut encoded_content)
            .unwrap();
        assert_eq!(
            String::from_utf8(encoded_content).unwrap(),
            "<p>Hello world</p>"
        ); // No encoding applied
    }

    #[test]
    fn empty_req_encodings() {
        let request = {
            let h = vec![("Accept-Encoding".to_owned(), "".to_owned())];
            Request::fake_http("GET", "/", h, vec![])
        };
        let response = Response::html("<p>Hello world</p>");

        let encoded_response = content_encoding::apply(&request, response);
        assert!(!encoded_response
            .headers
            .iter()
            .any(|(header_name, _)| header_name == "Content-Encoding")); // No Content-Encoding header
        let mut encoded_content = vec![];
        encoded_response
            .data
            .into_reader_and_size()
            .0
            .read_to_end(&mut encoded_content)
            .unwrap();
        assert_eq!(
            String::from_utf8(encoded_content).unwrap(),
            "<p>Hello world</p>"
        ); // No encoding applied
    }

    #[test]
    fn multi_req_encoding() {
        let request = {
            let h = vec![("Accept-Encoding".to_owned(), "foo".to_owned())];
            Request::fake_http("GET", "/", h, vec![])
        };
        let response = Response::html("<p>Hello world</p>");

        let encoded_response = content_encoding::apply(&request, response);
        assert!(!encoded_response
            .headers
            .iter()
            .any(|(header_name, _)| header_name == "Content-Encoding")); // No Content-Encoding header
        let mut encoded_content = vec![];
        encoded_response
            .data
            .into_reader_and_size()
            .0
            .read_to_end(&mut encoded_content)
            .unwrap();
        assert_eq!(
            String::from_utf8(encoded_content).unwrap(),
            "<p>Hello world</p>"
        ); // No encoding applied
    }

    #[test]
    fn unknown_req_encoding() {
        let request = {
            let h = vec![("Accept-Encoding".to_owned(), "x-gzip, br".to_owned())];
            Request::fake_http("GET", "/", h, vec![])
        };
        let response = Response::html("<p>Hello world</p>");

        let encoded_response = content_encoding::apply(&request, response);
        assert!(encoded_response
            .headers
            .contains(&("Content-Encoding".into(), "br".into()))); // Brotli Content-Encoding header
    }

    #[test]
    fn brotli_encoding() {
        let request = {
            let h = vec![("Accept-Encoding".to_owned(), "br".to_owned())];
            Request::fake_http("GET", "/", h, vec![])
        };
        let response = Response::html(
            "<html><head><title>Hello world</title><body><p>Hello world</p></body></html>",
        );

        let encoded_response = content_encoding::apply(&request, response);
        assert!(encoded_response
            .headers
            .contains(&("Content-Encoding".into(), "br".into()))); // Brotli Content-Encoding header
        let mut encoded_content = vec![];
        encoded_response
            .data
            .into_reader_and_size()
            .0
            .read_to_end(&mut encoded_content)
            .unwrap();
        assert_eq!(
            encoded_content,
            vec![
                27, 75, 0, 0, 4, 28, 114, 164, 129, 5, 210, 206, 25, 30, 90, 114, 224, 114, 73,
                109, 45, 196, 23, 126, 240, 144, 77, 40, 26, 211, 228, 67, 73, 40, 236, 55, 101,
                254, 127, 147, 194, 129, 132, 65, 130, 120, 152, 249, 68, 56, 93, 2
            ]
        ); // Applied proper Brotli encoding
    }

    #[test]
    fn gzip_encoding() {
        let request = {
            let h = vec![("Accept-Encoding".to_owned(), "gzip".to_owned())];
            Request::fake_http("GET", "/", h, vec![])
        };
        let response = Response::html(
            "<html><head><title>Hello world</title><body><p>Hello world</p></body></html>",
        );

        let encoded_response = content_encoding::apply(&request, response);
        assert!(encoded_response
            .headers
            .contains(&("Content-Encoding".into(), "gzip".into()))); // gzip Content-Encoding header
        let mut encoded_content = vec![];
        encoded_response
            .data
            .into_reader_and_size()
            .0
            .read_to_end(&mut encoded_content)
            .unwrap();

        // The 10-byte Gzip header contains an OS ID and a 4 byte timestamp
        // which are not stable, so we skip them in this comparison. Doing a
        // literal compare here is slightly silly, but the `deflate` crate has
        // no public decompressor functions for us to test a round-trip
        assert_eq!(
            encoded_content[10..],
            vec![
                179, 201, 40, 201, 205, 177, 179, 201, 72, 77, 76, 177, 179, 41, 201, 44, 201, 73,
                181, 243, 72, 205, 201, 201, 87, 40, 207, 47, 202, 73, 177, 209, 135, 8, 217, 36,
                229, 167, 84, 218, 217, 20, 160, 202, 21, 216, 217, 232, 67, 36, 244, 193, 166, 0,
                0, 202, 239, 44, 120, 76, 0, 0, 0
            ]
        ); // Applied proper gzip encoding
    }
}
