// Copyright 2013-2015 The rust-url developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use std::cmp;
use std::fmt::{self};
use std::hash;
use std::str;
use std::error::Error;
use std::convert::TryFrom;
use std::fmt::Formatter;

macro_rules! simple_enum_error {
    ($($name: ident => $description: expr,)+) => {
        /// Errors that can occur during parsing.
        ///
        /// This may be extended in the future so exhaustive matching is
        /// discouraged with an unused variant.
        #[derive(PartialEq, Eq, Clone, Copy, Debug)]
        #[non_exhaustive]
        pub enum ParseError {
            $(
                $name,
            )+
        }

        impl fmt::Display for ParseError {
            fn fmt(&self, fmt: &mut Formatter<'_>) -> fmt::Result {
                match *self {
                    $(
                        ParseError::$name => fmt.write_str($description),
                    )+
                }
            }
        }
    }
}

impl Error for ParseError {}

simple_enum_error! {
    Invalid => "invalid",
    EmptyHost => "empty host",
    IdnaError => "invalid international domain name",
    InvalidPort => "invalid port number",
    InvalidIpv4Address => "invalid IPv4 address",
    InvalidIpv6Address => "invalid IPv6 address",
    InvalidDomainCharacter => "invalid domain character",
    RelativeUrlWithoutBase => "relative URL without a base",
    RelativeUrlWithCannotBeABaseBase => "relative URL with a cannot-be-a-base base",
    SetHostOnCannotBeABaseUrl => "a cannot-be-a-base URL doesn’t have a host to set",
    Overflow => "URLs more than 4 GB are not supported",
}

#[cxx::bridge(namespace = "parse")]
mod ffi {
    #[derive(Clone)]
    struct ParseResult {
        serialization: String,
        has_host: bool,
        host: String,
        has_path:  bool,
        path: String,
        has_fragment: bool,
        fragment: String,
        has_scheme: bool,
        scheme: String,
        has_query: bool,
        query: String,
        has_port: bool,
        port: u16,
        valid: bool
    }

    unsafe extern "C++" {
        include!("brave/third_party/rust/url/v2/crate/parse.h");
        #[allow(dead_code)]
        fn InitializeICUForTesting();
        fn ParseURL(url: &str) -> ParseResult;
        fn Resolve(base: &ParseResult, relative: &str) -> ParseResult;
    }
}

#[derive(Clone)]
pub struct Url {
   parsed_result: ffi::ParseResult
}

impl Url {
    pub fn parse(input: &str) -> Result<Url, ParseError> {
        let res = ffi::ParseURL(input);
        if res.valid {
            Ok(Url{parsed_result: res})
        } else {
            Err(ParseError::Invalid)
        }
    }

    pub fn join(&self, input: &str) -> Result<Url, ParseError> {
        let url = Url {
            parsed_result: ffi::Resolve(&self.parsed_result, input)
        };
        if url.parsed_result.valid {
            Ok(url)
        } else {
            Err(ParseError::Invalid)
        }
    }

    #[inline]
    pub fn as_str(&self) -> &str {
        &self.parsed_result.serialization
    }

    #[inline]
    #[deprecated(since = "2.3.0", note = "use Into<String>")]
    pub fn into_string(self) -> String {
        self.into()
    }

    #[inline]
    pub fn scheme(&self) -> &str {
        self.parsed_result.scheme.as_str()
    }

    #[inline]
    pub fn has_host(&self) -> bool {
        self.parsed_result.has_host
    }

    #[inline]
    pub fn host_str(&self) -> Option<&str> {
        match self.has_host() {
            false => None,
            true => Some(self.parsed_result.host.as_str())
        }
    }

    #[inline]
    pub fn port(&self) -> Option<u16> {
        match self.parsed_result.has_port {
            false => None,
            true => Some(self.parsed_result.port)
        }
    }

    #[inline]
    pub fn path(&self) -> &str {
        self.parsed_result.path.as_str()
    }

    #[inline]
    pub fn query(&self) -> Option<&str> {
        match self.parsed_result.has_query {
            false => None,
            true => Some(self.parsed_result.query.as_str())
        }
    }

    #[inline]
    pub fn fragment(&self) -> Option<&str> {
        match self.parsed_result.has_fragment {
            false => None,
            true => Some(self.parsed_result.fragment.as_str())
        }
    }
}

impl str::FromStr for Url {
    type Err = ParseError;

    #[inline]
    fn from_str(input: &str) -> Result<Url, ParseError> {
        Url::parse(input)
    }
}

impl<'a> TryFrom<&'a str> for Url {
    type Error = ParseError;

    fn try_from(s: &'a str) -> Result<Self, Self::Error> {
        Url::parse(s)
    }
}

impl fmt::Display for Url {
    #[inline]
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Display::fmt(&self.parsed_result.serialization, formatter)
    }
}

impl From<Url> for String {
    fn from(value: Url) -> String {
        value.parsed_result.serialization
    }
}

impl fmt::Debug for Url {
    #[inline]
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter
            .debug_struct("Url")
            .field("host", &self.host_str())
            .field("port", &self.port())
            .field("path", &self.path())
            .field("query", &self.query())
            .field("fragment", &self.fragment())
            .finish()
    }
}

impl Eq for Url {}

impl PartialEq for Url {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        self.parsed_result.serialization == other.parsed_result.serialization
    }
}

impl Ord for Url {
    #[inline]
    fn cmp(&self, other: &Self) -> cmp::Ordering {
        self.parsed_result.serialization.cmp(&other.parsed_result.serialization)
    }
}

impl PartialOrd for Url {
    #[inline]
    fn partial_cmp(&self, other: &Self) -> Option<cmp::Ordering> {
        self.parsed_result.serialization.partial_cmp(&other.parsed_result.serialization)
    }
}

impl hash::Hash for Url {
    #[inline]
    fn hash<H>(&self, state: &mut H)
    where
        H: hash::Hasher,
    {
        hash::Hash::hash(&self.parsed_result.serialization, state)
    }
}

impl AsRef<str> for Url {
    #[inline]
    fn as_ref(&self) -> &str {
        &self.parsed_result.serialization
    }
}

// Tests from original url crate
// To build tests run
// `npm run build -- --target=brave/third_party/rust/url/v0_2:lib_url_v0_2_unittests`
// and then run `out_dir/lib_url_v0_2_unittests`
#[test]
fn test_relative() {
    let base: Url = "sc://%C3%B1".parse().unwrap();
    let url = base.join("/resources/testharness.js").unwrap();
    assert_eq!(url.as_str(), "sc://%C3%B1/resources/testharness.js");
}

#[test]
fn test_relative_empty() {
    let base: Url = "sc://%C3%B1".parse().unwrap();
    let url = base.join("").unwrap();
    assert_eq!(url.as_str(), "sc://%C3%B1");
}

#[test]
fn test_idna() {
    ffi::InitializeICUForTesting();
    assert!("http://goșu.ro".parse::<Url>().is_ok());
    assert_eq!(
        Url::parse("http://☃.net/").unwrap().host_str(),
        Some("xn--n3h.net")
    );
    assert!("https://r2---sn-huoa-cvhl.googlevideo.com/crossdomain.xml"
        .parse::<Url>()
        .is_ok());
}

#[test]
fn test_make_relative() {
    let tests = [
        (
            "http://127.0.0.1:8080/test",
            "http://127.0.0.1:8080/test",
            "",
        ),
        (
            "http://127.0.0.1:8080/test",
            "http://127.0.0.1:8080/test/",
            "test/",
        ),
        (
            "http://127.0.0.1:8080/test/",
            "http://127.0.0.1:8080/test",
            "../test",
        ),
        (
            "http://127.0.0.1:8080/",
            "http://127.0.0.1:8080/?foo=bar#123",
            "?foo=bar#123",
        ),
        (
            "http://127.0.0.1:8080/",
            "http://127.0.0.1:8080/test/video",
            "test/video",
        ),
        (
            "http://127.0.0.1:8080/test",
            "http://127.0.0.1:8080/test/video",
            "test/video",
        ),
        (
            "http://127.0.0.1:8080/test/",
            "http://127.0.0.1:8080/test/video",
            "video",
        ),
        (
            "http://127.0.0.1:8080/test",
            "http://127.0.0.1:8080/test2/video",
            "test2/video",
        ),
        (
            "http://127.0.0.1:8080/test/",
            "http://127.0.0.1:8080/test2/video",
            "../test2/video",
        ),
        (
            "http://127.0.0.1:8080/test/bla",
            "http://127.0.0.1:8080/test2/video",
            "../test2/video",
        ),
        (
            "http://127.0.0.1:8080/test/bla/",
            "http://127.0.0.1:8080/test2/video",
            "../../test2/video",
        ),
        (
            "http://127.0.0.1:8080/test/?foo=bar#123",
            "http://127.0.0.1:8080/test/video",
            "video",
        ),
        (
            "http://127.0.0.1:8080/test/",
            "http://127.0.0.1:8080/test/video?baz=meh#456",
            "video?baz=meh#456",
        ),
        (
            "http://127.0.0.1:8080/test",
            "http://127.0.0.1:8080/test?baz=meh#456",
            "?baz=meh#456",
        ),
        (
            "http://127.0.0.1:8080/test/",
            "http://127.0.0.1:8080/test?baz=meh#456",
            "../test?baz=meh#456",
        ),
        (
            "http://127.0.0.1:8080/test/",
            "http://127.0.0.1:8080/test/?baz=meh#456",
            "?baz=meh#456",
        ),
        (
            "http://127.0.0.1:8080/test/?foo=bar#123",
            "http://127.0.0.1:8080/test/video?baz=meh#456",
            "video?baz=meh#456",
        ),
        (
            "http://127.0.0.1:8080/file.txt",
            "http://127.0.0.1:8080/test/file.txt",
            "test/file.txt",
        ),
        (
            "http://127.0.0.1:8080/not_equal.txt",
            "http://127.0.0.1:8080/test/file.txt",
            "test/file.txt",
        ),
    ];

    for (base, uri, relative) in &tests {
        let base_uri = url::Url::parse(base).unwrap();
        let relative_uri = url::Url::parse(uri).unwrap();
        assert_eq!(
            base_uri.join(relative).unwrap().as_str(),
            *uri,
            "base: {}, uri: {}, relative: {}",
            base,
            uri,
            relative
        );
    }
}
