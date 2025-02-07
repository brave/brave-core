// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

//! Everything in this module is private, but is still publicly accessible from the outside
//! because of the `try_or_400!` macro.

use std::error::Error;

/// This macro assumes that the current function returns a `Response` and takes a `Result`.
/// If the expression you pass to the macro is an error, then a 400 response is returned.
///
/// # Example
///
/// ```
/// # #[macro_use] extern crate rouille;
/// # fn main() {
/// use rouille::Request;
/// use rouille::Response;
///
/// fn handle_something(request: &Request) -> Response {
///     let data = try_or_400!(post_input!(request, {
///         field1: u32,
///         field2: String,
///     }));
///
///     Response::text("hello")
/// }
/// # }
/// ```
#[macro_export]
macro_rules! try_or_400 {
    ($result:expr) => {
        match $result {
            Ok(r) => r,
            Err(err) => {
                let json = $crate::try_or_400::ErrJson::from_err(&err);
                return $crate::Response::json(&json).with_status_code(400);
            }
        }
    };
}

#[derive(Serialize)]
pub struct ErrJson {
    description: String,
    cause: Option<Box<ErrJson>>,
}

impl ErrJson {
    pub fn from_err<E: ?Sized + Error>(err: &E) -> ErrJson {
        let cause = err.source().map(ErrJson::from_err).map(Box::new);
        ErrJson {
            description: err.to_string(),
            cause,
        }
    }
}
