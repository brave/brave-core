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
//! - In order to parse JSON, see [the `json` module](json/index.html).
//! - In order to parse input from HTML forms, see [the `post` module](post/index.html).
//! - In order to read a plain text body, see
//!   [the `plain_text_body` function](fn.plain_text_body.html).

pub use self::basic_http_auth::basic_http_auth;
pub use self::basic_http_auth::HttpAuthCredentials;
pub use self::cookies::cookies;
pub use self::cookies::CookiesIter;
pub use self::json::json_input;
pub use self::plain::plain_text_body;
pub use self::plain::plain_text_body_with_limit;
pub use self::priority_header::parse_priority_header;
pub use self::priority_header::priority_header_preferred;
pub use self::priority_header::PriorityHeaderIter;

pub mod json;
pub mod multipart;
pub mod post;

mod accept;
mod basic_http_auth;
mod cookies;
mod plain;
mod priority_header;
