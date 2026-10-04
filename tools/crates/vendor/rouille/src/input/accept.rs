// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

/// Dispatches between blocks depending on the value of the `Accept` header.
///
/// This macro takes as first parameter the request object, and then each additional parameter must
/// be of the form `mime => value` where `mime` is a MIME type in quotes and `value` is an
/// expression of any type.
///
/// The macro returns the value corresponding to the MIME type that has the highest priority in
/// the request's `Accept` header. If multiple MIME types have the same priority, the earliest in
/// the list passed to the macro is chosen. If no MIME matches the request, the first in the list
/// is chosen. If there is no `Accept` header in the request, it is as if the header's value
/// was `*/*`.
///
/// You can also use `*` in the MIME types you pass to the macro. The MIME `*/*` can be used as a
/// default handler.
///
/// > **Note**: Using `|` just like in real match expressions is not yet supported because the
/// > author didn't find a way to make it work with Rust macros.
///
/// # Basic example
///
/// ```
/// # #[macro_use] extern crate rouille;
/// use rouille::Request;
/// use rouille::Response;
///
/// fn handle(request: &Request) -> Response {
///     accept!(request,
///         "text/html" => Response::html("<p>Hello world</p>"),
///         "text/plain" => Response::text("Hello world"),
///     )
/// }
/// # fn main() {}
/// ```
///
/// # Example with a default handler
///
/// ```
/// # #[macro_use] extern crate rouille;
/// use rouille::Request;
/// use rouille::Response;
///
/// fn handle(request: &Request) -> Response {
///     accept!(request,
///         "text/html" => Response::html("<p>Hello world</p>"),
///         "text/plain" => Response::text("Hello world"),
///         "*/*" => Response::empty_406()
///     )
/// }
/// # fn main() {}
/// ```
#[macro_export]
macro_rules! accept {
    ($request:expr, $($mime:expr => $val:expr),+ $(,)*) => ({
        use $crate::input;
        use std::iter;

        let header = $request.header("Accept").unwrap_or("*/*");

        let handled = {
            let i = iter::empty();
            $(let i = i.chain(iter::once($mime));)+
            i
        };

        let mut preferred = input::priority_header_preferred(header, handled).unwrap_or(0);

        let mut outcome = None;

        preferred += 1;
        $(
            if preferred >= 1 {
                preferred -= 1;
                if preferred == 0 {
                    outcome = Some($val);
                }
            }
        )+

        outcome.unwrap()    // unwrap() can only panic if priority_header_preferred has a bug or
                            // if the list of mimes is empty (which can't happen)
    });
}

#[cfg(test)]
mod tests {
    use Request;

    #[test]
    fn basic() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Accept".to_owned(), "text/plain, */*".to_owned())],
            vec![],
        );

        let result = accept!(&request,
            "text/plain" => 5,
            "*/*" => 12,
        );

        assert_eq!(result, 5);
    }

    #[test]
    fn wildcard() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Accept".to_owned(), "image/gif".to_owned())],
            vec![],
        );

        let result = accept!(&request,
            "text/plain" => 5,
            "*/*" => 12,
        );

        assert_eq!(result, 12);
    }

    #[test]
    fn no_match() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Accept".to_owned(), "image/gif".to_owned())],
            vec![],
        );

        let result = accept!(&request,
            "text/plain" => 5,
            "image/svg+xml" => 12,
        );

        assert_eq!(result, 5);
    }

    #[test]
    fn multimatch_first() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Accept".to_owned(), "image/gif".to_owned())],
            vec![],
        );

        let result = accept!(&request,
            "text/plain" => 5,
            "text/plain" => 12,
            "text/plain" => 28,
        );

        assert_eq!(result, 5);
    }

    #[test]
    fn no_header_first() {
        let request = Request::fake_http("GET", "/", vec![], vec![]);

        let result = accept!(&request,
            "image/gif" => 5,
            "text/plain" => 12,
            "text/html" => 28,
        );

        assert_eq!(result, 5);
    }

    #[test]
    fn no_header_wildcard() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![("Accept".to_owned(), "image/tiff".to_owned())],
            vec![],
        );

        let result = accept!(&request,
            "image/gif" => 5,
            "text/plain" => 12,
            "text/html" => 28,
            "*/*" => 37
        );

        assert_eq!(result, 37);
    }
}
