# Changelog

## Version 3.6.2
- Expose `new_ssl` feature for `rustls` support from `tiny-http`.
- Switch to `sha1-smol` for a smaller footprint, more stable hash library.
- Remove dependency on `num_cpus` using `std::thread::available_parallelism` instead.

## Version 3.6.1

Reverts [Added a number of default features](https://github.com/tomaka/rouille/pull/254) as it breaks any downstreams 
who were specifying `default-features = false`.

## Version 3.6.0
- Added [`rustls`](https://github.com/rustls/rustls) support (via `tiny-http`), if you're currently using the `ssl` feature
  you can switch from OpenSSL to Rustls by instead enabling the `rustls` feature in your `Cargo.toml`.
- [Added a number of default features](https://github.com/tomaka/rouille/pull/254) to allow users to reduce
  their dependency graph where they don't need all the functionality Rouille provides. `logging`, `assets`,
  `post` and `session` are now optional, but enabled by default for backwards compatibility.
- [Correctly support 'flag' type query parameters](https://github.com/tomaka/rouille/pull/259) where the parameter has no
  associated value. Previously a query like `GET /?foo` would return `None` from `get_param`, instead of `Some("")`.
- Updated `tiny-http` to 0.12.0, further reducing the dependency tree by breaking our hard requirement on `time-rs`. This
  version of `tiny-http` also enables Unix socket listeners, which will be exposed in a future release of Rouille.

## Version 3.5.0
- Replaced our use of the `brotli2` crate with the alternative pure Rust implementation
  [`brotli`](https://github.com/dropbox/rust-brotli). This removes Rouille's vulnerability to
  [RUSTSEC-2021-0131](https://rustsec.org/advisories/RUSTSEC-2021-0131.html), which existed due to `brotli-sys`
  bundling a vulnerable version of the underlying C library.
- Unpinned `time-rs` and as a result increased our MSRV to 1.51, we don't have a formal MSRV policy and the ecosystem
  is making it more and more difficult to support compiler versions more than about 6 months old.

## Version 3.4.0
- Resolved a number of cleanup & refactoring TODOs
- Correctly identify non-lowercase content types as text (e.g. `text/JSON`
  would be incorrectly identified as non-text).
- Pinned `time-rs` to 0.3.2 to avoid a semver-breaking change in their MSRV.
- Bumped `chrono` to 0.4.19 and disabled their default feature set to avoid
  warnings about `RUSTSEC-2020-0071` (Rouille was never vulnerable, but used a
  vulnerable version of `chrono`).

## Version 3.3.1
- Use `.strip_prefix` in place of `.starts_with` where appropriate, this stops a Clippy lint from
  leaking out of our `router!` macro and into downstream code.

## Version 3.3.0
- Bumped minimum supported Rust version to 1.48
- Added module-level documentation for `rouille::content_encoding`
- Updated `time` dependency to `0.3` and `postgres` to `0.19` to fix a compile failure due to a yanked version of
  `sha1`.
- Fixed numerous typos in the crate documentation.

## Version 3.2.1

- Removed unused dependency `term` and updated `rand`, `multipart`, `deflate`
  and `time` to latest supported versions.

## Version 3.2

- Add `ResponseBody::from_reader_and_size` for constructing a `ResponseBody`
  from a `Reader` and an already known size such that `Content-Length` may be
  set on the response.

## Version 3.1.1

- Replace all uses of deprecated `try!` with `?` to suppress warnings that can
  leak out of macro contexts.

## Version 3.1.0

- Add `Server::poll_timeout()` for polling more efficiently.
- Add `Server::stoppable()` for running a single, cancellable server thread.
- Add `Server::join()` for finalising all in-flight requests before shutting down.
- [Prevent infinite loop on Websocket EOF](https://github.com/tomaka/rouille/pull/212)
- Update `tiny-http` to 0.8.1 containing fixes for:
  - HTTPS deadlock where one request holds a locked resource while another is
    attempting HTTPS negotiation
  - Fix [RUSTSEC-2020-0031](https://rustsec.org/advisories/RUSTSEC-2020-0031.html)
  - Don't set `Transfer-Encoding: chunked` on 1xx or 204 responses (which can lead
    to clients hanging).
- Bump minimum support Rust version to 1.41.1

## Version 3.0.0

- Bump minimum supported Rust version to 1.34.2
- embedded, exposed `url` version increased to 2.0
- Don't use deprecated `Error::description()`

## Version 2.2.0

- Bump minimum supported Rust version to 1.20.0.
- Expose that the `Request` body (accessible with `request.data()`)
  has a `Send` bound.

## Version 2.1.0

- Replace `flate2` with `deflate`
- Fixed handling of url-encoded path components in route!() macro.
  Previously, URL was eagerly decoded and thus would fail to match
  intended routes if special characters were used (such as ? or /).
  Now, individual matched components are decoded after matching.
- Added `Response::empty_204`.
- Added ssl feature and new_ssl constructor to Server, for https
  support. The certificate and private key must be supplied by user.

## Version 2.0.0

- Dropped the use of [rustc-serialize](https://crates.io/rustc-serialize)
  in favor of using [serde](https://crates.io/serde).
- Updated `multipart` to 0.13. The `input::multipart::get_multipart_input` function returns
  types reexported from `multipart` which have small but breaking API changes.
- Update `Server` with an option to use a thread pool to process requests

## Version 1.0.0

- `input::cookies` changed to return an iterator that yields `(&str, &str)`.

## Version 0.4.2

- The `content_encoding` module now supports brotli.
- Added an `accept!` macro similar to a `match` expression that chooses a block depending on the
  value of the `Accept` header of the request.
- Added `proxy::full_proxy`. It behaves the same as `proxy` but returns more status codes and less
  errors.
- Added `Response::from_data`, `from_file`, `with_content_disposition_attachment`, `empty_406`,
  `with_public_cache`, `with_private_cache`, `with_no_cache`, `without_header`,
  `with_additional_header` and `with_unique_header` for easier response manipulation.
- Added `Request::headers()` that provides an iterator to the list of headers.
- Added `input::priority_header_preferred` and `input::parse_priority_header` to easily parse
  request headers such as `Accept` or `Accept-Language`.
- MIME types that contain "font" are now also compressed by the `content_encoding` module.
- Changed `text/xml` to `application/xml` in the MIME types auto-determination.

## Version 0.4.1

- Added a `Server` struct as an alternative to `start_server` for manual control over the behavior.
- Added a `content_encoding::apply` function that applies `Content-Encoding` to a response.
- The `try_or_400!` macro now returns a response whose body describes the error in JSON.
- The `try_or_400!` macro now requires the error to implement the `std::error::Error` trait.

## Version 0.4.0

- Added support for websockets with the `websocket` module.
- Added `Request::do_not_track()` to query the DNT header.
- Renamed `get_json_input()` to `json_input()`.
- Renamed `get_cookies()` to `cookies()`.
- Renamed `get_basic_http_auth()` to `basic_http_auth()`.
- The logs now show the time of the start of the request processing.
- `Request::header()` now returns a `Option<&str>` instead of `Option<String>`.
- `Response::svg()` and `Response::html()` now take a `Into<String>` instead of a `Into<Vec<u8>>`.
- Renamed `Response::error()` and `success()` to `is_error()` and `is_success()`.
- The `headers` field of `Response` are now `Vec<(Cow<'static, str>, Cow<'static, str>)>` instead
  of `Vec<(String, String)>`.
- Removed `Response::redirect` and replaced it with `redirect_301`, `redirect_302`, etc.
- Added `Response::with_etag()` to add an ETag header to a response.
- Added an `upgrade` field to `Response`, necessary for websockets.
- Fixed being able to set the value of the Content-Length and Transfer-Encoding headers.
- `plain_text_body` now has a limit of 1 MB of data before returning an error.
- Added `plain_text_body_with_limit` which does the same as `plain_text_body` but with a
  customizable limit.
- Implemented the `std::error::Error` trait on all error types.
- Added `Response::into_reader_and_size()` to retrieve a `Read` object from a `ResponseBody`.
- Fixed issue with static files not being found on Windows because of `/` and `\` mismatch.

## Version 0.3.3

- Added the `proxy` module with basic reverse proxy.

## Version 0.3.2

- Added the `rouille::input::plain_text_body` function.

## Version 0.3.1

- Empty Vecs are now allowed for POST input.

## Version 0.3.0

- Reworked POST input. You can now use the `post_input!` macro instead of creating a decodable
  struct.
- Removed the `input::session` module and replaced it with the `session` module. Sessions no longer
  store data in a hashmap, but instead only provide a way to generate a unique ID per client.

## Version 0.2.0

- Fixed the `+` character in the query string not being replaced with a space as it should.
- `Request::data()` now returns an `Option<impl Read>` instead of a `Vec<u8>`. If `data()` is
  called twice, the second call will return `None`.
- `RouteError` has been removed. You are now encouraged to return a `Response` everywhere instead
  of a `Result<Response, RouteError>`.
- The `try_or_400!`, `find_route!` and `assert_or_4OO!` macros and the `match_assets` function have
  been adjusted for the previous change.
- Added a `try_or_404!` macro similar to `try_or_400!`.
- In the case of a panic, the response with status code 500 that the server answers now contains a
  small text in its body, indicating the user that an internal server error occured.
- Added `Response::empty_400()`, `Response::empty_404()`, `Response::success()` and
  `Response::error()`.
