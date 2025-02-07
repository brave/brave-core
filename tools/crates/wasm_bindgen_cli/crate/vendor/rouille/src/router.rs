// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

/// Equivalent to a `match` expression but for routes.
///
/// # Example
///
/// ```no_run
/// # #[macro_use] extern crate rouille; fn main() {
/// # let request: rouille::Request = unsafe { std::mem::uninitialized() };
/// let _result = router!(request,
///     // first route
///     (GET) (/) => {
///         12
///     },
///
///     // second route
///     (GET) (/hello) => {
///         43 * 7
///     },
///
///     // ... other routes here ...
///
///     // default route
///     _ => 5
/// );
/// # }
/// ```
///
/// # Details
///
/// The macro will take each route one by one and execute the first one that matches, similar to the
/// `match` language construct. The whole `router!` expression then returns what the body
/// returns, therefore all the bodies must return the same type of data.
///
/// You can use parameters by putting them inside `{}`:
///
/// ```ignore
/// (GET) (/{id}/foo) => {
///     ...
/// },
/// ```
///
/// If you use parameters inside `{}`, then a variable with the same name will be available in the
/// code in the body.
/// Each parameter gets parsed through the `FromStr` trait. If the parsing fails, the route is
/// ignored. If you get an error because the type of the parameter couldn't be inferred, you can
/// also specify the type inside the brackets:
///
/// ```ignore
/// (GET) (/{id: u32}/foo) => {
///     ...
/// },
/// ```
///
///
/// # Alternative syntax (**string-style**)
///
/// You can also define url routes using strings. This allows using characters that are not valid rust
/// `ident`s (e.g. periods and numbers).
///
/// ```ignore
/// (GET) ["/hello/2"] => { ... },
/// ```
///
/// You can use parameters by putting them inside `{}`, and adding an `identity: type` pair. Note
/// that `identity` names **must** match the parameter names used in the URL string and an
/// accompanying `type` is required. The declared identities (variables) will be parsed through the
/// `FromStr` trait and made available in the code of the route's body. If the parsing fails, the
/// route is ignored.
///
/// ```ignore
/// (GET) ["/add/{a}/plus/{b}", a: u32, b: u32] => {
///     let c = a + b;
///     ...
/// },
/// ```
///
/// # Some other things to note
///
/// - The right of the `=>` must be a block (must be surrounded by `{` and `}`).
/// - The default handler (with `_`) must be present or will get a compilation error.
/// - The pattern of the URL must be inside parentheses for ident-style syntax
///   and brackets for string-style syntax. This is to bypass limitations of Rust's macros system.
/// - String-style and token-style definitions are mutually exclusive. Either all routes are defined with
///   tokens or all routes are defined with strings.
/// - When using URL parameters with **string-style** syntax, the parameter names in the URL and `identity: type`
///   pairs must be the same, e.g. `... ["/users/{name}", name: String] ...` .
///   This can't be checked at compile time so bad route definitions will cause a runtime `panic`.
///
#[macro_export]
macro_rules! router {
    // -----------------
    // --- New style ---
    // -----------------
    ($request:expr,
     $(($method:ident) [$url_pattern:expr $(, $param:ident: $param_type:ty)*] => $handle:expr,)*
     _ => $default:expr $(,)*) => {
        {
            let request = &$request;

            // ignoring the GET parameters (everything after `?`)
            let request_url = request.raw_url();
            let request_url = {
                let pos = request_url.find('?').unwrap_or(request_url.len());
                &request_url[..pos]
            };

            let mut ret = None;
            $({
                if ret.is_none() && request.method() == stringify!($method) {
                    ret = $crate::router!(__param_dispatch request_url, $url_pattern => $handle ; $($param: $param_type),*);
                }
            })+

            if let Some(ret) = ret {
                ret
            } else {
                $default
            }
        }
    };

    // No url parameters, just check the url and evaluate the `$handle`
    (__param_dispatch $request_url:ident, $url_pattern:expr => $handle:expr ; ) => {
        $crate::router!(__check_url_match $request_url, $url_pattern => $handle)
    };

    // Url parameters found, check and parse the url against the provided pattern
    (__param_dispatch $request_url:ident, $url_pattern:expr => $handle:expr ; $($param:ident: $param_type:ty),*) => {
        $crate::router!(__check_parse_pattern $request_url, $url_pattern => $handle ; $($param: $param_type),*)
    };

    (__check_url_match $request_url:ident, $url_pattern:expr => $handle:expr) => {
        if $request_url == $url_pattern {
            Some($handle)
        } else {
            None
        }
    };

    // Compare each url segment while attempting to parse any url parameters.
    // If parsing fails, return `None` so this route gets skipped.
    // If parsing is successful, recursively bind each url parameter to the given identity
    // before evaluating the `$handle`
    // Note: Url parameters need to be held in the `RouilleUrlParams` struct since
    //       we need to be able to "evaluate to None" (if url segments don't match or parsing fails)
    //       and we can't actually "return None" since we'd be returning from whatever scope the macro is being used in.
    (__check_parse_pattern $request_url_str:ident, $url_pattern:expr => $handle:expr ; $($param:ident: $param_type:ty),*) => {
        {
            let request_url = $request_url_str.split("/")
                .map(|s| $crate::percent_encoding::percent_decode(s.as_bytes()).decode_utf8_lossy().into_owned())
                .collect::<Vec<_>>();
            let url_pattern = $url_pattern.split("/").collect::<Vec<_>>();
            if request_url.len() != url_pattern.len() {
                None
            } else {
                struct RouilleUrlParams {
                    $( $param: Option<$param_type> ),*
                }
                impl RouilleUrlParams {
                    fn new() -> Self {
                        Self {
                            $( $param: None ),*
                        }
                    }
                }
                let url_params = (|| {
                    let mut url_params = RouilleUrlParams::new();
                    for (actual, desired) in request_url.iter().zip(url_pattern.iter()) {
                        if let Some(key) = desired.strip_prefix("{").and_then(|d| d.strip_suffix("}")) {
                            $crate::router!(__insert_param $request_url_str, url_params, key, actual ; $($param: $param_type)*)
                        } else if actual != desired {
                            return None
                        }
                    }
                    Some(url_params)
                })();
                if let Some(url_params) = url_params {
                    $crate::router!(__build_resp $request_url_str, url_params, $handle ; $($param: $param_type)*)
                } else {
                    None
                }
            }
        }
    };

    // We walked through all the given url parameter identities and couldn't find one that
    // matches the parameter name defined in the url-string
    //   e.g. `(GET) ("/name/{title}", name: String)
    (__insert_param $request_url:ident, $url_params:ident, $key:expr, $actual:expr ; ) => {
        panic!("Unable to match url parameter name, `{}`, to an `identity: type` pair in url: {:?}", $key, $request_url);
    };

    // Walk through all the given url parameter identities. If they match the current
    // `$key` (a parameter name in the string-url), then set them in the `$url_params` struct
    (__insert_param $request_url:ident, $url_params:ident, $key:expr, $actual:expr ; $param:tt: $param_type:tt $($params:tt: $param_types:tt)*) => {
        if $key == stringify!($param) {
            $crate::router!(__bind_url_param $url_params, $actual, $param, $param_type)
        } else {
            $crate::router!(__insert_param $request_url, $url_params, $key, $actual ; $($params: $param_types)*);
        }
    };

    (__bind_url_param $url_params:ident, $actual:expr, $param:ident, $param_type:ty) => {
        {
            match $actual.parse::<$param_type>() {
                Ok(value) => $url_params.$param = Some(value),
                // it's safe to `return` here since we're in a closure
                Err(_) => return None,
            }
        }
    };

    // No more url parameters to bind
    (__build_resp $request_url:ident, $url_params:expr, $handle:expr ; ) => {
        { Some($handle) }
    };

    // There's still some params to bind
    (__build_resp $request_url:ident, $url_params:expr, $handle:expr ; $param:tt: $param_type:tt $($params:tt: $param_types:tt)*) => {
        $crate::router!(__bind_param $request_url, $url_params, $handle, $param: $param_type ; $($params: $param_types)*)
    };

    // Recursively pull out and bind a url param
    (__bind_param $request_url:ident, $url_params:expr, $handle:expr, $param:ident: $param_type:ty ; $($params:tt: $param_types:tt)*) => {
        {
            let $param = match $url_params.$param {
                Some(p) => p,
                None => {
                    let param_name = stringify!($param);
                    panic!("Url parameter identity, `{}`, does not have a matching `{{{}}}` segment in url: {:?}",
                           param_name, param_name, $request_url);
                }
            };
            $crate::router!(__build_resp $request_url, $url_params, $handle ; $($params: $param_types)*)
        }
    };


    // -----------------
    // --- Old style ---
    // -----------------
    ($request:expr, $(($method:ident) ($($pat:tt)+) => $value:block,)* _ => $def:expr $(,)*) => {
        {
            let request = &$request;

            // ignoring the GET parameters (everything after `?`)
            let request_url = request.raw_url();
            let request_url = {
                let pos = request_url.find('?').unwrap_or(request_url.len());
                &request_url[..pos]
            };

            let mut ret = None;

            $({
                if ret.is_none() && request.method() == stringify!($method) {
                    ret = $crate::router!(__check_pattern request_url $value $($pat)+);
                }
            })+

            if let Some(ret) = ret {
                ret
            } else {
                $def
            }
        }
    };

    (__check_pattern $url:ident $value:block /{$p:ident} $($rest:tt)*) => (
        if let Some(url) = $url.strip_prefix('/') {
            let url = &$url[1..];
            let pat_end = url.find('/').unwrap_or(url.len());
            let rest_url = &url[pat_end..];

            if let Ok($p) = url[0 .. pat_end].parse() {
                $crate::router!(__check_pattern rest_url $value $($rest)*)
            } else {
                None
            }
        } else {
            None
        }
    );

    (__check_pattern $url:ident $value:block /{$p:ident: $t:ty} $($rest:tt)*) => (
        if let Some(url) = $url.strip_prefix('/') {
            let url = &$url[1..];
            let pat_end = url.find('/').unwrap_or(url.len());
            let rest_url = &url[pat_end..];

            if let Ok($p) = $crate::percent_encoding::percent_decode(url[0 .. pat_end].as_bytes())
                .decode_utf8_lossy().parse() {
                let $p: $t = $p;
                $crate::router!(__check_pattern rest_url $value $($rest)*)
            } else {
                None
            }
        } else {
            None
        }
    );

    (__check_pattern $url:ident $value:block /$p:ident $($rest:tt)*) => (
        {
            let required = concat!("/", stringify!($p));
            if let Some(rest_url) = $url.strip_prefix(required) {
                $crate::router!(__check_pattern rest_url $value $($rest)*)
            } else {
                None
            }
        }
    );

    (__check_pattern $url:ident $value:block - $($rest:tt)*) => (
        {
            if let Some(rest_url) = $url.strip_prefix('-') {
                $crate::router!(__check_pattern rest_url $value $($rest)*)
            } else {
                None
            }
        }
    );

    (__check_pattern $url:ident $value:block) => (
        if $url.len() == 0 { Some($value) } else { None }
    );

    (__check_pattern $url:ident $value:block /) => (
        if $url == "/" { Some($value) } else { None }
    );

    (__check_pattern $url:ident $value:block $p:ident $($rest:tt)*) => (
        {
            let required = stringify!($p);
            if let Some(rest_url) = $url.strip_prefix(required) {
                $crate::router!(__check_pattern rest_url $value $($rest)*)
            } else {
                None
            }
        }
    );
}

#[allow(unused_variables)]
#[cfg(test)]
mod tests {
    use Request;

    // -- old-style tests --
    #[test]
    fn old_style_basic() {
        let request = Request::fake_http("GET", "/", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) (/hello) => { 0 },
                (GET) (/{_val:u32}) => { 0 },
                (GET) (/) => { 1 },
                _ => 0
            )
        );
    }

    #[test]
    fn old_style_dash() {
        let request = Request::fake_http("GET", "/a-b", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) (/a/b) => { 0 },
                (GET) (/a_b) => { 0 },
                (GET) (/a-b) => { 1 },
                _ => 0
            )
        );
    }

    #[test]
    fn old_style_params() {
        let request = Request::fake_http("GET", "/hello/5", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) (/hello/) => { 0 },
                (GET) (/hello/{id:u32}) => { if id == 5 { 1 } else { 0 } },
                (GET) (/hello/{_id:String}) => { 0 },
                _ => 0
            )
        );
    }

    #[test]
    fn old_style_trailing_comma() {
        let request = Request::fake_http("GET", "/hello/5", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) (/hello/) => { 0 },
                (GET) (/hello/{id:u32}) => { if id == 5 { 1 } else { 0 } },
                (GET) (/hello/{_id:String}) => { 0 },
                _ => 0,
            )
        );
    }

    #[test]
    fn old_style_trailing_commas() {
        let request = Request::fake_http("GET", "/hello/5", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) (/hello/) => { 0 },
                (GET) (/hello/{id:u32}) => { if id == 5 { 1 } else { 0 } },
                (GET) (/hello/{_id:String}) => { 0 },
                _ => 0,,,,
            )
        );
    }

    // -- new-style tests --
    #[test]
    fn multiple_params() {
        let request = Request::fake_http("GET", "/math/3.2/plus/4", vec![], vec![]);
        let resp = router!(request,
            (GET) ["/hello"] => { 1. },
            (GET) ["/math/{a}/plus/{b}", a: u32 , b: u32] => { 7. },
            (GET) ["/math/{a}/plus/{b}", a: f32 , b: u32] => { a + (b as f32) },
            _ => 0.
        );
        assert_eq!(7.2, resp);
    }

    #[test]
    fn basic() {
        let request = Request::fake_http("GET", "/", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) ["/hello"] => { 0 },
                (GET) ["/{_val}", _val: u32] => { 0 },
                (GET) ["/"] => { 1 },
                _ => 0
            )
        );
    }

    #[test]
    fn dash() {
        let request = Request::fake_http("GET", "/a-b", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) ["/a/b"] => { 0 },
                (GET) ["/a_b"] => { 0 },
                (GET) ["/a-b"] => { 1 },
                _ => 0
            )
        );
    }

    #[test]
    fn numbers() {
        let request = Request::fake_http("GET", "/5", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) ["/a"] => { 0 },
                (GET) ["/3"] => { 0 },
                (GET) ["/5"] => { 1 },
                _ => 0
            )
        );
    }

    #[test]
    fn trailing_comma() {
        let request = Request::fake_http("GET", "/5", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) ["/a"] => { 0 },
                (GET) ["/3"] => { 0 },
                (GET) ["/5"] => { 1 },
                _ => 0,
            )
        );
    }

    #[test]
    fn trailing_commas() {
        let request = Request::fake_http("GET", "/5", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) ["/a"] => { 0 },
                (GET) ["/3"] => { 0 },
                (GET) ["/5"] => { 1 },
                _ => 0,,,,
            )
        );
    }

    #[test]
    fn files() {
        let request = Request::fake_http("GET", "/robots.txt", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) ["/a"] => { 0 },
                (GET) ["/3/2/1"] => { 0 },
                (GET) ["/robots.txt"] => { 1 },
                _ => 0
            )
        );
    }

    #[test]
    fn skip_failed_parse_float() {
        let request = Request::fake_http("GET", "/hello/5.1", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) ["/hello/"] => { 0 },
                (GET) ["/hello/{_id}", _id: u32] => { 0 },
                (GET) ["/hello/{id}", id: f32] => { if id == 5.1 { 1 } else { 0 } },
                _ => 0
            )
        );
    }

    #[test]
    fn skip_failed_parse_string() {
        let request = Request::fake_http("GET", "/word/wow", vec![], vec![]);
        let resp = router!(request,
            (GET) ["/hello"] => { "hello".to_string() },
            (GET) ["/word/{int}", int: u32] => { int.to_string() },
            (GET) ["/word/{word}", word: String] => { word },
            _ => "default".to_string()
        );
        assert_eq!("wow", resp);
    }

    #[test]
    fn url_parameter_ownership() {
        let request = Request::fake_http("GET", "/word/one/two/three/four", vec![], vec![]);
        let resp = router!(request,
            (GET) ["/hello"] => { "hello".to_string() },
            (GET) ["/word/{int}", int: u32] => { int.to_string() },
            (GET) ["/word/{a}/{b}/{c}/{d}", a: String, b: String, c: String, d: String] => {
                fn expects_strings(a: String, b: String, c: String, d: String) -> String {
                    format!("{}{}{}{}", a, b, c, d)
                }
                expects_strings(a, b, c, d)
            },
            _ => "default".to_string()
        );
        assert_eq!("onetwothreefour", resp);
    }

    #[test]
    #[should_panic(
        expected = "Url parameter identity, `id`, does not have a matching `{id}` segment in url: \"/hello/james\""
    )]
    fn identity_not_present_in_url_string() {
        let request = Request::fake_http("GET", "/hello/james", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) ["/hello/"] => { 0 },
                (GET) ["/hello/{name}", name: String, id: u32] => { 1 }, // this should fail
                _ => 0
            )
        );
    }

    #[test]
    #[should_panic(
        expected = "Unable to match url parameter name, `name`, to an `identity: type` pair in url: \"/hello/1/james\""
    )]
    fn parameter_with_no_matching_identity() {
        let request = Request::fake_http("GET", "/hello/1/james", vec![], vec![]);

        assert_eq!(
            1,
            router!(request,
                (GET) ["/hello/"] => { 0 },
                (GET) ["/hello/{id}/{name}"] => { 0 },           // exact match should be ignored
                (GET) ["/hello/{id}/{name}", id: u32] => { id }, // this one should fail
                _ => 0
            )
        );
    }

    #[test]
    fn encoded() {
        let request = Request::fake_http("GET", "/hello/%3Fa/test", vec![], vec![]);

        assert_eq!(
            "?a",
            router!(request,
           (GET) ["/hello/{val}/test", val: String] => { val },
           _ => String::from(""))
        );
    }

    #[test]
    fn encoded_old() {
        let request = Request::fake_http("GET", "/hello/%3Fa/test", vec![], vec![]);

        assert_eq!(
            "?a",
            router!(request,
           (GET) (/hello/{val: String}/test) => { val },
           _ => String::from(""))
        );
    }

    #[test]
    fn param_slash() {
        let request = Request::fake_http("GET", "/hello%2F5", vec![], vec![]);

        router!(request,
            (GET) ["/{a}", a: String] => { assert_eq!(a, "hello/5") },
            _ => panic!()
        );
    }
}
