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

use std::str::Split;
use Request;

/// Attempts to parse the list of cookies from the request.
///
/// Returns an iterator that produces a pair of `(key, value)`. If the header is missing or
/// malformed, an empty iterator is returned.
///
/// # Example
///
/// ```
/// use rouille::Request;
/// use rouille::input;
///
/// # let request: Request = return;
/// if let Some((_, val)) = input::cookies(&request).find(|&(n, _)| n == "cookie-name") {
///     println!("Value of cookie = {:?}", val);
/// }
/// ```
// TODO: should an error be returned if the header is malformed?
// TODO: be less tolerant to what is accepted?
pub fn cookies(request: &Request) -> CookiesIter {
    let header = request.header("Cookie").unwrap_or("");

    CookiesIter {
        iter: header.split(';'),
    }
}

/// Iterator that returns the list of cookies of a request.
///
/// See [the `cookies` functions](fn.cookies.html).
pub struct CookiesIter<'a> {
    iter: Split<'a, char>,
}

impl<'a> Iterator for CookiesIter<'a> {
    type Item = (&'a str, &'a str);

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            let cookie = match self.iter.next() {
                Some(c) => c,
                None => return None,
            };

            let mut splits = cookie.splitn(2, |c| c == '=');
            let key = match splits.next() {
                None => continue,
                Some(v) => v,
            };
            let value = match splits.next() {
                None => continue,
                Some(v) => v,
            };

            let key = key.trim();
            let value = value.trim().trim_matches(|c| c == '"');

            return Some((key, value));
        }
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let (_, len) = self.iter.size_hint();
        (0, len)
    }
}

#[cfg(test)]
mod test {
    use super::cookies;
    use Request;

    #[test]
    fn no_cookie() {
        let request = Request::fake_http("GET", "/", vec![], Vec::new());
        assert_eq!(cookies(&request).count(), 0);
    }

    #[test]
    fn cookies_ok() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Cookie".to_owned(), "a=b; hello=world".to_owned())],
            Vec::new(),
        );

        assert_eq!(
            cookies(&request).collect::<Vec<_>>(),
            vec![("a".into(), "b".into()), ("hello".into(), "world".into())]
        );
    }
}
