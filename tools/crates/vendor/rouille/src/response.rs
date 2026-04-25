// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

use percent_encoding;
use serde;
use serde_json;
use std::borrow::Cow;
use std::fmt;
use std::fs::File;
use std::io;
use std::io::Cursor;
use std::io::Read;
use Request;
use Upgrade;

/// Contains a prototype of a response.
///
/// The response is only sent to the client when you return the `Response` object from your
/// request handler. This means that you are free to create as many `Response` objects as you want.
pub struct Response {
    /// The status code to return to the user.
    pub status_code: u16,

    /// List of headers to be returned in the response.
    ///
    /// The value of the following headers will be ignored from this list, even if present:
    ///
    /// - Accept-Ranges
    /// - Connection
    /// - Content-Length
    /// - Content-Range
    /// - Trailer
    /// - Transfer-Encoding
    ///
    /// Additionally, the `Upgrade` header is ignored as well unless the `upgrade` field of the
    /// `Response` is set to something.
    ///
    /// The reason for this is that these headers are too low-level and are directly handled by
    /// the underlying HTTP response system.
    ///
    /// The value of `Content-Length` is automatically determined by the `ResponseBody` object of
    /// the `data` member.
    ///
    /// If you want to send back `Connection: upgrade`, you should set the value of the `upgrade`
    /// field to something.
    pub headers: Vec<(Cow<'static, str>, Cow<'static, str>)>,

    /// An opaque type that contains the body of the response.
    pub data: ResponseBody,

    /// If set, rouille will give ownership of the client socket to the `Upgrade` object.
    ///
    /// In all circumstances, the value of the `Connection` header is managed by the framework and
    /// cannot be customized. If this value is set, the response will automatically contain
    /// `Connection: Upgrade`.
    pub upgrade: Option<Box<dyn Upgrade + Send>>,
}

impl fmt::Debug for Response {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("Response")
            .field("status_code", &self.status_code)
            .field("headers", &self.headers)
            .finish()
    }
}

impl Response {
    /// Returns true if the status code of this `Response` indicates success.
    ///
    /// This is the range [200-399].
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::text("hello world");
    /// assert!(response.is_success());
    /// ```
    #[inline]
    pub fn is_success(&self) -> bool {
        self.status_code >= 200 && self.status_code < 400
    }

    /// Shortcut for `!response.is_success()`.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::empty_400();
    /// assert!(response.is_error());
    /// ```
    #[inline]
    pub fn is_error(&self) -> bool {
        !self.is_success()
    }

    /// Builds a `Response` that redirects the user to another URL with a 301 status code. This
    /// semantically means a permanent redirect.
    ///
    /// > **Note**: If you're uncertain about which status code to use for a redirection, 303 is
    /// > the safest choice.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::redirect_301("/foo");
    /// ```
    #[inline]
    pub fn redirect_301<S>(target: S) -> Response
    where
        S: Into<Cow<'static, str>>,
    {
        Response {
            status_code: 301,
            headers: vec![("Location".into(), target.into())],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Builds a `Response` that redirects the user to another URL with a 302 status code. This
    /// semantically means a temporary redirect.
    ///
    /// > **Note**: If you're uncertain about which status code to use for a redirection, 303 is
    /// > the safest choice.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::redirect_302("/bar");
    /// ```
    #[inline]
    pub fn redirect_302<S>(target: S) -> Response
    where
        S: Into<Cow<'static, str>>,
    {
        Response {
            status_code: 302,
            headers: vec![("Location".into(), target.into())],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Builds a `Response` that redirects the user to another URL with a 303 status code. This
    /// means "See Other" and is usually used to indicate where the response of a query is
    /// located.
    ///
    /// For example when a user sends a POST request to URL `/foo` the server can return a 303
    /// response with a target to `/bar`, in which case the browser will automatically change
    /// the page to `/bar` (with a GET request to `/bar`).
    ///
    /// > **Note**: If you're uncertain about which status code to use for a redirection, 303 is
    /// > the safest choice.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let user_id = 5;
    /// let response = Response::redirect_303(format!("/users/{}", user_id));
    /// ```
    #[inline]
    pub fn redirect_303<S>(target: S) -> Response
    where
        S: Into<Cow<'static, str>>,
    {
        Response {
            status_code: 303,
            headers: vec![("Location".into(), target.into())],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Builds a `Response` that redirects the user to another URL with a 307 status code. This
    /// semantically means a permanent redirect.
    ///
    /// The difference between 307 and 301 is that the client must keep the same method after
    /// the redirection. For example if the browser sends a POST request to `/foo` and that route
    /// returns a 307 redirection to `/bar`, then the browser will make a POST request to `/bar`.
    /// With a 301 redirection it would use a GET request instead.
    ///
    /// > **Note**: If you're uncertain about which status code to use for a redirection, 303 is
    /// > the safest choice.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::redirect_307("/foo");
    /// ```
    #[inline]
    pub fn redirect_307<S>(target: S) -> Response
    where
        S: Into<Cow<'static, str>>,
    {
        Response {
            status_code: 307,
            headers: vec![("Location".into(), target.into())],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Builds a `Response` that redirects the user to another URL with a 302 status code. This
    /// semantically means a temporary redirect.
    ///
    /// The difference between 308 and 302 is that the client must keep the same method after
    /// the redirection. For example if the browser sends a POST request to `/foo` and that route
    /// returns a 308 redirection to `/bar`, then the browser will make a POST request to `/bar`.
    /// With a 302 redirection it would use a GET request instead.
    ///
    /// > **Note**: If you're uncertain about which status code to use for a redirection, 303 is
    /// > the safest choice.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::redirect_302("/bar");
    /// ```
    #[inline]
    pub fn redirect_308<S>(target: S) -> Response
    where
        S: Into<Cow<'static, str>>,
    {
        Response {
            status_code: 308,
            headers: vec![("Location".into(), target.into())],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Builds a 200 `Response` with data.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::from_data("application/octet-stream", vec![1, 2, 3, 4]);
    /// ```
    #[inline]
    pub fn from_data<C, D>(content_type: C, data: D) -> Response
    where
        C: Into<Cow<'static, str>>,
        D: Into<Vec<u8>>,
    {
        Response {
            status_code: 200,
            headers: vec![("Content-Type".into(), content_type.into())],
            data: ResponseBody::from_data(data),
            upgrade: None,
        }
    }

    /// Builds a 200 `Response` with the content of a file.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use std::fs::File;
    /// use rouille::Response;
    ///
    /// let file = File::open("image.png").unwrap();
    /// let response = Response::from_file("image/png", file);
    /// ```
    #[inline]
    pub fn from_file<C>(content_type: C, file: File) -> Response
    where
        C: Into<Cow<'static, str>>,
    {
        Response {
            status_code: 200,
            headers: vec![("Content-Type".into(), content_type.into())],
            data: ResponseBody::from_file(file),
            upgrade: None,
        }
    }

    /// Builds a `Response` that outputs HTML.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::html("<p>hello <strong>world</strong></p>");
    /// ```
    #[inline]
    pub fn html<D>(content: D) -> Response
    where
        D: Into<String>,
    {
        Response {
            status_code: 200,
            headers: vec![("Content-Type".into(), "text/html; charset=utf-8".into())],
            data: ResponseBody::from_string(content),
            upgrade: None,
        }
    }

    /// Builds a `Response` that outputs SVG.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::svg("<svg xmlns='http://www.w3.org/2000/svg'/>");
    /// ```
    #[inline]
    pub fn svg<D>(content: D) -> Response
    where
        D: Into<String>,
    {
        Response {
            status_code: 200,
            headers: vec![("Content-Type".into(), "image/svg+xml; charset=utf-8".into())],
            data: ResponseBody::from_string(content),
            upgrade: None,
        }
    }

    /// Builds a `Response` that outputs plain text.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::text("hello world");
    /// ```
    #[inline]
    pub fn text<S>(text: S) -> Response
    where
        S: Into<String>,
    {
        Response {
            status_code: 200,
            headers: vec![("Content-Type".into(), "text/plain; charset=utf-8".into())],
            data: ResponseBody::from_string(text),
            upgrade: None,
        }
    }

    /// Builds a `Response` that outputs JSON.
    ///
    /// # Example
    ///
    /// ```
    /// extern crate serde;
    /// #[macro_use] extern crate serde_derive;
    /// #[macro_use] extern crate rouille;
    /// use rouille::Response;
    /// # fn main() {
    ///
    /// #[derive(Serialize)]
    /// struct MyStruct {
    ///     field1: String,
    ///     field2: i32,
    /// }
    ///
    /// let response = Response::json(&MyStruct { field1: "hello".to_owned(), field2: 5 });
    /// // The Response will contain something like `{ field1: "hello", field2: 5 }`
    /// # }
    /// ```
    #[inline]
    pub fn json<T>(content: &T) -> Response
    where
        T: serde::Serialize,
    {
        let data = serde_json::to_string(content).unwrap();

        Response {
            status_code: 200,
            headers: vec![(
                "Content-Type".into(),
                "application/json; charset=utf-8".into(),
            )],
            data: ResponseBody::from_data(data),
            upgrade: None,
        }
    }

    /// Builds a `Response` that returns a `401 Not Authorized` status
    /// and a `WWW-Authenticate` header.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::basic_http_auth_login_required("realm");
    /// ```
    #[inline]
    pub fn basic_http_auth_login_required(realm: &str) -> Response {
        // TODO: escape the realm
        Response {
            status_code: 401,
            headers: vec![(
                "WWW-Authenticate".into(),
                format!("Basic realm=\"{}\"", realm).into(),
            )],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Builds an empty `Response` with a 204 status code.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::empty_204();
    /// ```
    #[inline]
    pub fn empty_204() -> Response {
        Response {
            status_code: 204,
            headers: vec![],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Builds an empty `Response` with a 400 status code.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::empty_400();
    /// ```
    #[inline]
    pub fn empty_400() -> Response {
        Response {
            status_code: 400,
            headers: vec![],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Builds an empty `Response` with a 404 status code.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::empty_404();
    /// ```
    #[inline]
    pub fn empty_404() -> Response {
        Response {
            status_code: 404,
            headers: vec![],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Builds an empty `Response` with a 406 status code.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::empty_406();
    /// ```
    #[inline]
    pub fn empty_406() -> Response {
        Response {
            status_code: 406,
            headers: vec![],
            data: ResponseBody::empty(),
            upgrade: None,
        }
    }

    /// Changes the status code of the response.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::Response;
    /// let response = Response::text("hello world").with_status_code(500);
    /// ```
    #[inline]
    pub fn with_status_code(mut self, code: u16) -> Response {
        self.status_code = code;
        self
    }

    /// Removes all headers from the response that match `header`.
    pub fn without_header(mut self, header: &str) -> Response {
        self.headers
            .retain(|&(ref h, _)| !h.eq_ignore_ascii_case(header));
        self
    }

    /// Adds an additional header to the response.
    #[inline]
    pub fn with_additional_header<H, V>(mut self, header: H, value: V) -> Response
    where
        H: Into<Cow<'static, str>>,
        V: Into<Cow<'static, str>>,
    {
        self.headers.push((header.into(), value.into()));
        self
    }

    /// Removes all headers from the response whose names are `header`, and replaces them .
    pub fn with_unique_header<H, V>(mut self, header: H, value: V) -> Response
    where
        H: Into<Cow<'static, str>>,
        V: Into<Cow<'static, str>>,
    {
        // If Vec::retain provided a mutable reference this code would be much simpler and would
        // only need to iterate once.
        // See https://github.com/rust-lang/rust/issues/25477

        // TODO: if the response already has a matching header we shouldn't have to build a Cow
        // from the header

        let header = header.into();

        let mut found_one = false;
        self.headers.retain(|&(ref h, _)| {
            if h.eq_ignore_ascii_case(&header) {
                if !found_one {
                    found_one = true;
                    true
                } else {
                    false
                }
            } else {
                true
            }
        });

        if found_one {
            for &mut (ref h, ref mut v) in &mut self.headers {
                if !h.eq_ignore_ascii_case(&header) {
                    continue;
                }
                *v = value.into();
                break;
            }
            self
        } else {
            self.with_additional_header(header, value)
        }
    }

    /// Adds or replaces a `ETag` header to the response, and turns the response into an empty 304
    /// response if the ETag matches a `If-None-Match` header of the request.
    ///
    /// An ETag is a unique representation of the content of a resource. If the content of the
    /// resource changes, the ETag should change as well.
    /// The purpose of using ETags is that a client can later ask the server to send the body of
    /// a response only if it still matches a certain ETag the client has stored in memory.
    ///
    /// > **Note**: You should always try to specify an ETag for responses that have a large body.
    ///
    /// # Example
    ///
    /// ```rust
    /// use rouille::Request;
    /// use rouille::Response;
    ///
    /// fn handle(request: &Request) -> Response {
    ///     Response::text("hello world").with_etag(request, "my-etag-1234")
    /// }
    /// ```
    #[inline]
    pub fn with_etag<E>(self, request: &Request, etag: E) -> Response
    where
        E: Into<Cow<'static, str>>,
    {
        self.with_etag_keep(etag).simplify_if_etag_match(request)
    }

    /// Turns the response into an empty 304 response if the `ETag` that is stored in it matches a
    /// `If-None-Match` header of the request.
    pub fn simplify_if_etag_match(mut self, request: &Request) -> Response {
        if self.status_code < 200 || self.status_code >= 300 {
            return self;
        }

        let mut not_modified = false;
        for &(ref key, ref etag) in &self.headers {
            if !key.eq_ignore_ascii_case("ETag") {
                continue;
            }

            not_modified = request
                .header("If-None-Match")
                .map(|header| header == etag)
                .unwrap_or(false);
        }

        if not_modified {
            self.data = ResponseBody::empty();
            self.status_code = 304;
        }

        self
    }

    /// Adds a `ETag` header to the response, or replaces an existing header if there is one.
    ///
    /// > **Note**: Contrary to `with_etag`, this function doesn't try to turn the response into
    /// > a 304 response. If you're unsure of what to do, prefer `with_etag`.
    #[inline]
    pub fn with_etag_keep<E>(self, etag: E) -> Response
    where
        E: Into<Cow<'static, str>>,
    {
        self.with_unique_header("ETag", etag)
    }

    /// Adds or replace a `Content-Disposition` header of the response. Tells the browser that the
    /// body of the request should fire a download popup instead of being shown in the browser.
    ///
    /// # Example
    ///
    /// ```rust
    /// use rouille::Request;
    /// use rouille::Response;
    ///
    /// fn handle(request: &Request) -> Response {
    ///     Response::text("hello world").with_content_disposition_attachment("book.txt")
    /// }
    /// ```
    ///
    /// When the response is sent back to the browser, it will show a popup asking the user to
    /// download the file "book.txt" whose content will be "hello world".
    pub fn with_content_disposition_attachment(mut self, filename: &str) -> Response {
        // The name must be percent-encoded.
        let name = percent_encoding::percent_encode(filename.as_bytes(), super::DEFAULT_ENCODE_SET);

        // If you find a more elegant way to do the thing below, don't hesitate to open a PR

        // Support for this format varies browser by browser, so this may not be the most
        // ideal thing.
        // TODO: it's maybe possible to specify multiple file names
        let mut header = Some(format!("attachment; filename*=UTF8''{}", name).into());

        for &mut (ref key, ref mut val) in &mut self.headers {
            if key.eq_ignore_ascii_case("Content-Disposition") {
                *val = header.take().unwrap();
                break;
            }
        }

        if let Some(header) = header {
            self.headers.push(("Content-Disposition".into(), header));
        }

        self
    }

    /// Adds or replaces a `Cache-Control` header that specifies that the resource is public and
    /// can be cached for the given number of seconds.
    ///
    /// > **Note**: This function doesn't do any caching itself. It just indicates that clients
    /// > that receive this response are allowed to cache it.
    #[inline]
    pub fn with_public_cache(self, max_age_seconds: u64) -> Response {
        self.with_unique_header(
            "Cache-Control",
            format!("public, max-age={}", max_age_seconds),
        )
        .without_header("Expires")
        .without_header("Pragma")
    }

    /// Adds or replaces a `Cache-Control` header that specifies that the resource is private and
    /// can be cached for the given number of seconds.
    ///
    /// Only the browser or the final client is authorized to cache the resource. Intermediate
    /// proxies must not cache it.
    ///
    /// > **Note**: This function doesn't do any caching itself. It just indicates that clients
    /// > that receive this response are allowed to cache it.
    #[inline]
    pub fn with_private_cache(self, max_age_seconds: u64) -> Response {
        self.with_unique_header(
            "Cache-Control",
            format!("private, max-age={}", max_age_seconds),
        )
        .without_header("Expires")
        .without_header("Pragma")
    }

    /// Adds or replaces a `Cache-Control` header that specifies that the client must not cache
    /// the resource.
    #[inline]
    pub fn with_no_cache(self) -> Response {
        self.with_unique_header("Cache-Control", "no-cache, no-store, must-revalidate")
            .with_unique_header("Expires", "0")
            .with_unique_header("Pragma", "no-cache")
    }
}

/// An opaque type that represents the body of a response.
///
/// You can't access the inside of this struct, but you can build one by using one of the provided
/// constructors.
///
/// # Example
///
/// ```
/// use rouille::ResponseBody;
/// let body = ResponseBody::from_string("hello world");
/// ```
pub struct ResponseBody {
    data: Box<dyn Read + Send>,
    data_length: Option<usize>,
}

impl ResponseBody {
    /// Builds a `ResponseBody` that doesn't return any data.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::ResponseBody;
    /// let body = ResponseBody::empty();
    /// ```
    #[inline]
    pub fn empty() -> ResponseBody {
        ResponseBody {
            data: Box::new(io::empty()),
            data_length: Some(0),
        }
    }

    /// Builds a new `ResponseBody` that will read the data from a `Read`.
    ///
    /// Note that this is suboptimal compared to other constructors because the length
    /// isn't known in advance.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use std::io;
    /// use std::io::Read;
    /// use rouille::ResponseBody;
    ///
    /// let body = ResponseBody::from_reader(io::stdin().take(128));
    /// ```
    #[inline]
    pub fn from_reader<R>(data: R) -> ResponseBody
    where
        R: Read + Send + 'static,
    {
        ResponseBody {
            data: Box::new(data),
            data_length: None,
        }
    }

    /// Builds a new `ResponseBody` that will read the data from a `Read`.
    ///
    /// The caller must provide the content length. It is unspecified
    /// what will happen if the content length does not match the actual
    /// length of the data returned from the reader.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use std::io;
    /// use std::io::Read;
    /// use rouille::ResponseBody;
    ///
    /// let body = ResponseBody::from_reader_and_size(io::stdin().take(128), 128);
    /// ```
    #[inline]
    pub fn from_reader_and_size<R>(data: R, size: usize) -> ResponseBody
    where
        R: Read + Send + 'static,
    {
        ResponseBody {
            data: Box::new(data),
            data_length: Some(size),
        }
    }

    /// Builds a new `ResponseBody` that returns the given data.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::ResponseBody;
    /// let body = ResponseBody::from_data(vec![12u8, 97, 34]);
    /// ```
    #[inline]
    pub fn from_data<D>(data: D) -> ResponseBody
    where
        D: Into<Vec<u8>>,
    {
        let data = data.into();
        let len = data.len();

        ResponseBody {
            data: Box::new(Cursor::new(data)),
            data_length: Some(len),
        }
    }

    /// Builds a new `ResponseBody` that returns the content of the given file.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use std::fs::File;
    /// use rouille::ResponseBody;
    ///
    /// let file = File::open("page.html").unwrap();
    /// let body = ResponseBody::from_file(file);
    /// ```
    #[inline]
    pub fn from_file(file: File) -> ResponseBody {
        let len = file.metadata().map(|metadata| metadata.len() as usize).ok();

        ResponseBody {
            data: Box::new(file),
            data_length: len,
        }
    }

    /// Builds a new `ResponseBody` that returns an UTF-8 string.
    ///
    /// # Example
    ///
    /// ```
    /// use rouille::ResponseBody;
    /// let body = ResponseBody::from_string("hello world");
    /// ```
    #[inline]
    pub fn from_string<S>(data: S) -> ResponseBody
    where
        S: Into<String>,
    {
        ResponseBody::from_data(data.into().into_bytes())
    }

    /// Extracts the content of the response.
    ///
    /// Returns the size of the body and the body itself. If the size is `None`, then it is
    /// unknown.
    #[inline]
    pub fn into_reader_and_size(self) -> (Box<dyn Read + Send>, Option<usize>) {
        (self.data, self.data_length)
    }
}

#[cfg(test)]
mod tests {
    use Response;

    #[test]
    fn unique_header_adds() {
        let r = Response {
            headers: vec![],
            ..Response::empty_400()
        };

        let r = r.with_unique_header("Foo", "Bar");

        assert_eq!(r.headers.len(), 1);
        assert_eq!(r.headers[0], ("Foo".into(), "Bar".into()));
    }

    #[test]
    fn unique_header_adds_without_touching() {
        let r = Response {
            headers: vec![("Bar".into(), "Foo".into())],
            ..Response::empty_400()
        };

        let r = r.with_unique_header("Foo", "Bar");

        assert_eq!(r.headers.len(), 2);
        assert_eq!(r.headers[0], ("Bar".into(), "Foo".into()));
        assert_eq!(r.headers[1], ("Foo".into(), "Bar".into()));
    }

    #[test]
    fn unique_header_replaces() {
        let r = Response {
            headers: vec![
                ("foo".into(), "A".into()),
                ("fOO".into(), "B".into()),
                ("Foo".into(), "C".into()),
            ],
            ..Response::empty_400()
        };

        let r = r.with_unique_header("Foo", "Bar");

        assert_eq!(r.headers.len(), 1);
        assert_eq!(r.headers[0], ("foo".into(), "Bar".into()));
    }
}
