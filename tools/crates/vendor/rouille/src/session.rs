// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

//! Sessions handling.
//!
//! The main feature of this module is the `session` function which handles a session. This
//! function guarantees that a single unique identifier is assigned to each client. This identifier
//! is accessible through the parameter passed to the inner closure.
//!
//! # Basic example
//!
//! Here is a basic example showing how to get a session ID.
//!
//! ```
//! use rouille::Request;
//! use rouille::Response;
//! use rouille::session;
//!
//! fn handle_request(request: &Request) -> Response {
//!     session::session(request, "SID", 3600, |session| {
//!         let id: &str = session.id();
//!
//!         // This id is unique to each client.
//!
//!         Response::text(format!("Session ID: {}", id))
//!     })
//! }
//! ```

use rand;
use rand::distributions::Alphanumeric;
use rand::Rng;
use std::borrow::Cow;
use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering;

use input;
use Request;
use Response;

pub fn session<'r, F>(request: &'r Request, cookie_name: &str, timeout_s: u64, inner: F) -> Response
where
    F: FnOnce(&Session<'r>) -> Response,
{
    let mut cookie = input::cookies(request);
    let cookie = cookie.find(|&(ref k, _)| k == &cookie_name);
    let cookie = cookie.map(|(_, v)| v);

    let session = if let Some(cookie) = cookie {
        Session {
            key_was_retrieved: AtomicBool::new(false),
            key_was_given: true,
            key: cookie.into(),
        }
    } else {
        Session {
            key_was_retrieved: AtomicBool::new(false),
            key_was_given: false,
            key: generate_session_id().into(),
        }
    };

    let mut response = inner(&session);

    if session.key_was_retrieved.load(Ordering::Relaxed) {
        // TODO: use `get_mut()`
        // FIXME: correct interactions with existing headers
        // TODO: allow setting domain
        let header_value = format!(
            "{}={}; Max-Age={}; Path=/; HttpOnly",
            cookie_name, session.key, timeout_s
        );
        response
            .headers
            .push(("Set-Cookie".into(), header_value.into()));
    }

    response
}

/// Contains the ID of the session.
pub struct Session<'r> {
    key_was_retrieved: AtomicBool,
    key_was_given: bool,
    key: Cow<'r, str>,
}

impl<'r> Session<'r> {
    /// Returns true if the client gave us a session ID.
    ///
    /// If this returns false, then we are sure that no data is available.
    #[inline]
    pub fn client_has_sid(&self) -> bool {
        self.key_was_given
    }

    /// Returns the id of the session.
    #[inline]
    pub fn id(&self) -> &str {
        self.key_was_retrieved.store(true, Ordering::Relaxed);
        &self.key
    }

    /*/// Generates a new id. This modifies the value returned by `id()`.
    // TODO: implement
    #[inline]
    pub fn regenerate_id(&self) {
        unimplemented!()
    }*/
}

/// Generates a string suitable for a session ID.
///
/// The output string doesn't contain any punctuation or character such as quotes or brackets
/// that could need to be escaped.
pub fn generate_session_id() -> String {
    // 5e+114 possibilities is reasonable.
    rand::thread_rng()
        .sample_iter(&Alphanumeric)
        .map(char::from)
        .filter(|&c| {
            ('a'..='z').contains(&c) || ('A'..='Z').contains(&c) || ('0'..='9').contains(&c)
        })
        .take(64)
        .collect::<String>()
}

#[test]
fn test_generate_session_id() {
    assert!(generate_session_id().len() >= 32);
}
