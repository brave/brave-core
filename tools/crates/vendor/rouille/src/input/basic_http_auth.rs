// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

//! Analyze the request's headers and body.
//!
//! This module provides functions and sub-modules that allow you to easily analyze or parse the
//! request's headers and body.
//!
//! - In order to parse JSON, see [the `json` module](json/input.html).
//! - In order to parse input from HTML forms, see [the `post` module](post/input.html).
//! - In order to read a plain text body, see
//!   [the `plain_text_body` function](fn.plain_text_body.html).

use base64;
use Request;

/// Credentials returned by `basic_http_auth`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct HttpAuthCredentials {
    /// Login provided by the client.
    pub login: String,
    /// Password provided by the client.
    pub password: String,
}

/// Attempts to parse a `Authorization` header with basic HTTP auth.
///
/// If such a header is present and valid, a `HttpAuthCredentials` is returned.
///
/// # Example
///
/// ```
/// use rouille::input;
/// use rouille::Request;
/// use rouille::Response;
///
/// fn handle(request: &Request) -> Response {
///     let auth = match input::basic_http_auth(request) {
///         Some(a) => a,
///         None => return Response::basic_http_auth_login_required("realm")
///     };
///
///     if auth.login == "admin" && auth.password == "GT5GeKyLvKLxuc7mjF5h" {
///         handle_after_login(request)
///     } else {
///         Response::text("Bad login/password").with_status_code(403)
///     }
/// }
///
/// fn handle_after_login(request: &Request) -> Response {
///     Response::text("You are in a secret area")
/// }
/// ```
pub fn basic_http_auth(request: &Request) -> Option<HttpAuthCredentials> {
    let header = match request.header("Authorization") {
        None => return None,
        Some(h) => h,
    };

    let mut split = header.splitn(2, |c| c == ' ');
    let authtype = match split.next() {
        None => return None,
        Some(t) => t,
    };

    if authtype != "Basic" {
        return None;
    }

    let authvalue = match split.next().and_then(|val| base64::decode(val).ok()) {
        Some(v) => v,
        None => return None,
    };

    let mut split = authvalue.splitn(2, |&c| c == b':');

    let login = match split
        .next()
        .map(Vec::from)
        .and_then(|l| String::from_utf8(l).ok())
    {
        Some(l) => l,
        None => return None,
    };
    let password = match split
        .next()
        .map(Vec::from)
        .and_then(|p| String::from_utf8(p).ok())
    {
        Some(p) => p,
        None => return None,
    };

    Some(HttpAuthCredentials { login, password })
}

#[cfg(test)]
mod test {
    use super::basic_http_auth;
    use super::HttpAuthCredentials;
    use Request;

    #[test]
    fn basic_http_auth_no_header() {
        let request = Request::fake_http("GET", "/", vec![], Vec::new());
        assert_eq!(basic_http_auth(&request), None);
    }

    #[test]
    fn basic_http_auth_wrong_header() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Authorization".to_owned(), "hello world".to_owned())],
            Vec::new(),
        );
        assert_eq!(basic_http_auth(&request), None);

        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Authorization".to_owned(), "Basic \0\0".to_owned())],
            Vec::new(),
        );
        assert_eq!(basic_http_auth(&request), None);
    }

    #[test]
    fn basic_http_auth_ok() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![(
                "Authorization".to_owned(),
                "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==".to_owned(),
            )],
            Vec::new(),
        );

        assert_eq!(
            basic_http_auth(&request),
            Some(HttpAuthCredentials {
                login: "Aladdin".to_owned(),
                password: "open sesame".to_owned(),
            })
        );
    }
}
