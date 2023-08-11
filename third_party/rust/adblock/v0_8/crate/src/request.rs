//! Contains structures needed to describe network requests.

use std::borrow::Cow;

use thiserror::Error;

use crate::url_parser;
use crate::utils;

/// The type of resource requested from the URL endpoint.
#[derive(Clone, PartialEq, Debug)]
pub enum RequestType {
    Beacon,
    Csp,
    Document,
    Dtd,
    Fetch,
    Font,
    Image,
    Media,
    Object,
    Other,
    Ping,
    Script,
    Stylesheet,
    Subdocument,
    Websocket,
    Xlst,
    Xmlhttprequest,
}

/// Possible failure reasons when creating a [`Request`].
#[derive(Debug, Error, PartialEq)]
pub enum RequestError {
    #[error("hostname parsing failed")]
    HostnameParseError,
    #[error("source hostname parsing failed")]
    SourceHostnameParseError,
    #[error("invalid Unicode provided")]
    UnicodeDecodingError,
}

impl From<idna::Errors> for RequestError {
    fn from(_err: idna::Errors) -> RequestError {
        RequestError::UnicodeDecodingError
    }
}

impl From<url::ParseError> for RequestError {
    fn from(_err: url::ParseError) -> RequestError {
        RequestError::HostnameParseError
    }
}

fn cpt_match_type(cpt: &str) -> RequestType {
    match cpt {
        "beacon" => RequestType::Ping,
        "csp_report" => RequestType::Csp,
        "document" | "main_frame" => RequestType::Document,
        "font" => RequestType::Font,
        "image" | "imageset" => RequestType::Image,
        "media" => RequestType::Media,
        "object" | "object_subrequest" => RequestType::Object,
        "ping" => RequestType::Ping,
        "script" => RequestType::Script,
        "stylesheet" => RequestType::Stylesheet,
        "sub_frame" | "subdocument" => RequestType::Subdocument,
        "websocket" => RequestType::Websocket,
        "xhr" | "xmlhttprequest" => RequestType::Xmlhttprequest,
        "other" => RequestType::Other,
        "speculative" => RequestType::Other,
        "web_manifest" => RequestType::Other,
        "xbl" => RequestType::Other,
        "xml_dtd" => RequestType::Other,
        "xslt" => RequestType::Other,
        _ => RequestType::Other,
    }
}

/// A network [`Request`], used as an interface for network blocking in the [`crate::Engine`].
#[derive(Clone, Debug)]
pub struct Request {
    pub request_type: RequestType,

    pub is_http: bool,
    pub is_https: bool,
    pub is_supported: bool,
    pub is_third_party: bool,
    pub url: String,
    pub hostname: String,
    pub source_hostname_hashes: Option<Vec<utils::Hash>>,

    pub(crate) original_url: String,
}

impl Request {
    pub(crate) fn get_url(&self, case_sensitive: bool) -> std::borrow::Cow<str> {
        if case_sensitive {
            Cow::Borrowed(&self.url)
        } else {
            Cow::Owned(self.url.to_ascii_lowercase())
        }
    }

    pub fn get_tokens(&self, token_buffer: &mut Vec<utils::Hash>) {
        token_buffer.clear();
        utils::tokenize_pooled(&self.url.to_ascii_lowercase(), token_buffer);
        // Add zero token as a fallback to wildcard rule bucket
        token_buffer.push(0);
    }

    #[allow(clippy::too_many_arguments)]
    fn from_detailed_parameters(
        raw_type: &str,
        url: &str,
        schema: &str,
        hostname: &str,
        source_hostname: &str,
        third_party: bool,
        original_url: String,
    ) -> Request {
        let is_http: bool;
        let is_https: bool;
        let is_supported: bool;
        let request_type: RequestType;

        if schema.is_empty() {
            // no ':' was found
            is_https = true;
            is_http = false;
            is_supported = true;
            request_type = cpt_match_type(raw_type);
        } else {
            is_http = schema == "http";
            is_https = !is_http && schema == "https";

            let is_websocket = !is_http && !is_https && (schema == "ws" || schema == "wss");
            is_supported = is_http || is_https || is_websocket;
            if is_websocket {
                request_type = RequestType::Websocket;
            } else {
                request_type = cpt_match_type(raw_type);
            }
        }

        let source_hostname_hashes = if !source_hostname.is_empty() {
            let mut hashes = Vec::with_capacity(4);
            hashes.push(utils::fast_hash(source_hostname));
            for (i, c) in source_hostname.char_indices() {
                if c == '.' && i + 1 < source_hostname.len() {
                    hashes.push(utils::fast_hash(&source_hostname[i + 1..]));
                }
            }
            Some(hashes)
        } else {
            None
        };

        Request {
            request_type,
            url: url.to_owned(),
            hostname: hostname.to_owned(),
            source_hostname_hashes,
            is_third_party: third_party,
            is_http,
            is_https,
            is_supported,
            original_url,
        }
    }

    /// Construct a new [`Request`].
    pub fn new(
        url: &str,
        source_url: &str,
        request_type: &str,
    ) -> Result<Request, RequestError> {
        if let Some(parsed_url) = url_parser::parse_url(url) {
            if let Some(parsed_source) = url_parser::parse_url(source_url) {
                let source_domain = parsed_source.domain();

                let third_party = source_domain != parsed_url.domain();

                Ok(Request::from_detailed_parameters(
                    request_type,
                    &parsed_url.url,
                    parsed_url.schema(),
                    parsed_url.hostname(),
                    parsed_source.hostname(),
                    third_party,
                    url.to_string(),
                ))
            } else {
                Ok(Request::from_detailed_parameters(
                    request_type,
                    &parsed_url.url,
                    parsed_url.schema(),
                    parsed_url.hostname(),
                    "",
                    true,
                    url.to_string(),
                ))
            }
        } else {
            Err(RequestError::HostnameParseError)
        }
    }

    /// If you're building a [`Request`] in a context that already has access to parsed
    /// representations of the input URLs, you can use this constructor to avoid extra lookups from
    /// the public suffix list. Take care to pass data correctly.
    pub fn preparsed(
        url: &str,
        hostname: &str,
        source_hostname: &str,
        request_type: &str,
        third_party: bool,
    ) -> Request {
        let splitter = memchr::memchr(b':', url.as_bytes()).unwrap_or(0);
        let schema: &str = &url[..splitter];

        Request::from_detailed_parameters(
            request_type,
            url,
            schema,
            hostname,
            source_hostname,
            third_party,
            url.to_string(),
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn build_request(
        raw_type: &str,
        url: &str,
        schema: &str,
        hostname: &str,
        domain: &str,
        source_hostname: &str,
        source_domain: &str,
    ) -> Request {
        let third_party = source_domain != domain;

        Request::from_detailed_parameters(
            raw_type,
            url,
            schema,
            hostname,
            source_hostname,
            third_party,
            url.to_string(),
        )
    }

    #[test]
    fn new_works() {
        let simple_example = build_request(
            "document",
            "https://example.com/ad",
            "https",
            "example.com",
            "example.com",
            "example.com",
            "example.com",
        );
        assert_eq!(simple_example.is_https, true);
        assert_eq!(simple_example.is_supported, true);
        assert_eq!(simple_example.is_third_party, false);
        assert_eq!(simple_example.request_type, RequestType::Document);
        assert_eq!(
            simple_example.source_hostname_hashes,
            Some(vec![
                utils::fast_hash("example.com"),
                utils::fast_hash("com")
            ]),
        );

        let unsupported_example = build_request(
            "document",
            "file://example.com/ad",
            "file",
            "example.com",
            "example.com",
            "example.com",
            "example.com",
        );
        assert_eq!(unsupported_example.is_https, false);
        assert_eq!(unsupported_example.is_http, false);
        assert_eq!(unsupported_example.is_supported, false);

        let first_party = build_request(
            "document",
            "https://subdomain.example.com/ad",
            "https",
            "subdomain.example.com",
            "example.com",
            "example.com",
            "example.com",
        );
        assert_eq!(first_party.is_https, true);
        assert_eq!(first_party.is_supported, true);
        assert_eq!(first_party.is_third_party, false);

        let third_party = build_request(
            "document",
            "https://subdomain.anotherexample.com/ad",
            "https",
            "subdomain.anotherexample.com",
            "anotherexample.com",
            "example.com",
            "example.com",
        );
        assert_eq!(third_party.is_https, true);
        assert_eq!(third_party.is_supported, true);
        assert_eq!(third_party.is_third_party, true);

        let websocket = build_request(
            "document",
            "wss://subdomain.anotherexample.com/ad",
            "wss",
            "subdomain.anotherexample.com",
            "anotherexample.com",
            "example.com",
            "example.com",
        );
        assert_eq!(websocket.is_https, false);
        assert_eq!(websocket.is_https, false);
        assert_eq!(websocket.is_supported, true);
        assert_eq!(websocket.is_third_party, true);
        assert_eq!(websocket.request_type, RequestType::Websocket);

        let assumed_https = build_request(
            "document",
            "//subdomain.anotherexample.com/ad",
            "",
            "subdomain.anotherexample.com",
            "anotherexample.com",
            "example.com",
            "example.com",
        );
        assert_eq!(assumed_https.is_https, true);
        assert_eq!(assumed_https.is_http, false);
        assert_eq!(assumed_https.is_supported, true);
    }

    fn tokenize(tokens: &[&str], extra_tokens: &[utils::Hash]) -> Vec<utils::Hash> {
        let mut tokens: Vec<_> = tokens.into_iter().map(|t| utils::fast_hash(&t)).collect();
        tokens.extend(extra_tokens);
        tokens
    }

    #[test]
    fn tokens_works() {
        let simple_example = build_request(
            "document",
            "https://subdomain.example.com/ad",
            "https",
            "subdomain.example.com",
            "example.com",
            "subdomain.example.com",
            "example.com",
        );
        assert_eq!(
            simple_example
                .source_hostname_hashes
                .as_ref()
                .unwrap()
                .as_slice(),
            tokenize(&["subdomain.example.com", "example.com", "com",], &[]).as_slice()
        );
        let mut tokens = Vec::new();
        simple_example.get_tokens(&mut tokens);
        assert_eq!(
            tokens.as_slice(),
            tokenize(&["https", "subdomain", "example", "com", "ad"], &[0]).as_slice()
        )
    }

    #[test]
    fn parses_urls() {
        let parsed = Request::new(
            "https://subdomain.example.com/ad",
            "https://example.com/",
            "document",
        )
        .unwrap();
        assert_eq!(parsed.is_https, true);
        assert_eq!(parsed.is_supported, true);
        assert_eq!(parsed.is_third_party, false);
        assert_eq!(parsed.request_type, RequestType::Document);

        // assert_eq!(parsed.domain, "example.com");
        assert_eq!(parsed.hostname, "subdomain.example.com");

        // assert_eq!(parsed.source_domain, "example.com");
        assert_eq!(
            parsed.source_hostname_hashes,
            Some(vec![
                utils::fast_hash("example.com"),
                utils::fast_hash("com")
            ]),
        );
        // assert_eq!(parsed.source_hostname, "example.com");

        let bad_url = Request::new(
            "subdomain.example.com/ad",
            "https://example.com/",
            "document",
        );
        assert_eq!(bad_url.err(), Some(RequestError::HostnameParseError));
    }

    #[test]
    fn fuzzing_errors() {
        {
            let parsed = Request::new("https://߶", "https://example.com", "other");
            assert!(parsed.is_ok());
        }
        {
            let parsed = Request::new(&format!(
                "https://{}",
                std::str::from_utf8(&[9, 9, 64]).unwrap()
            ), "https://example.com", "other");
            assert!(parsed.is_err());
        }
    }
}
