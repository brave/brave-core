# Endpoint Client

This component provides a **generic, type-safe client abstraction** for calling
HTTP endpoints.

It centralizes and standardizes:

- serializing strongly typed C++ request objects to JSON or Protobuf
- issuing requests via `network::SimpleURLLoader`
- mapping HTTP status codes to typed success/error bodies
- deserializing JSON or Protobuf responses into strongly typed C++ objects
- surfacing transport errors, HTTP status, and parse failures consistently

## Motivation

Without a shared abstraction, each endpoint reimplements the same mechanics
described above.

This repetition leads to:

- duplicated infrastructure code
- inconsistencies in error handling
- fragile, stringly-typed JSON construction/parsing
- avoidable runtime failures

This abstraction consolidates these concerns behind a compile-time-validated
interface and enforces correctness guarantees through C++ `concept`s and
`base::expected<>`.

## Usage

Request and response bodies are either JSON or Protobuf; an endpoint uses one
format for both.

- JSON request bodies implement `ToValue()`, and JSON response bodies implement
  `FromValue()`. The JSON schema/IDL compiler generates such types; hand-written
  ones are supported, but the IDL workflow is recommended.
- Protobuf request and response bodies are message types derived from
  `google::protobuf::MessageLite`.

The examples below use JSON; Protobuf endpoints work the same way, with
generated message types in place of the IDL ones.

---

### 1. Define your endpoint type

First, define IDL schemas for your request and response bodies.

`login_init_bodies.idl`:

```cpp
// Schema for the /v2/auth/login/init request and response.
namespace endpoints {
  // JSON sent:
  // {
  //   "email": "user@example.com",
  //   "serializedKE1": "0e85b8e68bd22965e15e..."
  // }
  dictionary LoginInitRequestBody {
    DOMString email;
    DOMString serializedKE1;
  };

  // JSON received (2xx):
  // {
  //   "loginToken": "eyJhbGciOiJFUzI1NiIs...",
  //   "serializedKE2": "b4fb94e4f9d3d87c368d..."
  // }
  dictionary LoginInitSuccessBody {
    DOMString loginToken;
    DOMString serializedKE2;
  };

  // JSON received (non-2xx):
  // {
  //   "code": 14005,
  //   "error": "incorrect email",
  //   "status": 401
  // }
  dictionary LoginInitErrorBody {
    // Can be null or a number (long).
    any code;
    DOMString error;
    long status;
  };
};
```

Use the `generated_types` template in your `BUILD.gn` to generate C++
request/response classes from the IDL.

`BUILD.gn`:

```python
generated_types("generated_api_types") {
  sources = [ "login_init_bodies.idl" ]

  deps = [ "//base" ]

  root_namespace = "brave_account::%(namespace)s"
}
```

Then define the endpoint in a header file using `BraveEndpoint<>`.

`login_init.h`:

```cpp
namespace brave_account::endpoints {

using LoginInit = BraveEndpoint<
    "accounts.bsg",         // service domain prefix
    "/v2/auth/login/init",  // URL path
    Post<LoginInitRequestBody>,
    Response<LoginInitSuccessBody, LoginInitErrorBody>>;

}  // namespace brave_account::endpoints
```

The `BraveEndpoint<>` `template` simplifies defining endpoints for Brave
services. It uses `brave_domains::GetServicesDomain()` under the hood to
automatically resolve the service domain to the correct environment.

Non-official builds resolve to staging, so that local testing never writes to
production. Official builds always resolve to production. Either way, the
environment can be overridden per service domain prefix with
`--env-<prefix>={dev,staging,prod}`, or globally with
`--brave-services-env={dev,staging,prod}`.

For non-Brave services, you can define an endpoint manually:

```cpp
struct MyEndpoint {
  using Request = Post<MyRequestBody>;
  using Response = Response<MySuccessBody, MyErrorBody>;

  static GURL URL() {
    return GURL("https://myservice.com/my_endpoint");
  }
};
```

---

### 2. Send the request and receive the response

`Client<>` enforces type safety: only the declared request type is accepted, and
only the declared response type is returned.

```cpp
LoginInit::Request request;
request.network_traffic_annotation_tag =
    net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
request.body.email = "user@example.com";
request.body.serialized_ke1 = "0e85b8e68bd22965e15e...";

Client<LoginInit>::Send(
    url_loader_factory_, std::move(request),
    base::BindOnce([](LoginInit::Response response) {
      if (!response.body) {
        // response.net_error, response.status_code
        return;
      }

      auto result =
          std::move(*response.body)
               .transform_error([](LoginInit::Response::ErrorBody body) {
                 // body.code, body.error, body.status
                 return ...;
               })
               .and_then([](LoginInit::Response::SuccessBody body) {
                 // body.login_token, body.serialized_ke2
                 return ...;
               });
    }));
```

`response.body` is `std::nullopt` when:

- the request failed (`net_error != net::OK`) or no response headers were
  received, or
- the response body fails to parse into the declared type: JSON bodies via
  `base::JSONReader` and `FromValue()`, Protobuf bodies via `ParseFromString()`.

#### Per-request options

`network_traffic_annotation_tag` is required (only tests may leave it unset).
Requests also have:

- `max_body_size`: forwarded to `SimpleURLLoader::DownloadToString()`; defaults
  to `SimpleURLLoader::kMaxBoundedStringDownloadSize` (5 MiB), which
  `DownloadToString()` `DCHECK`s it does not exceed. Larger responses fail with
  `net::ERR_INSUFFICIENT_RESOURCES`. `std::nullopt` lifts the limit via
  `SimpleURLLoader::DownloadToStringOfUnboundedSizeUntilCrashAndDie()`.
- `redirect_mode`: forwarded to `network::ResourceRequest::redirect_mode`;
  defaults to `kError`, so the network service fails a redirect with
  `net::ERR_FAILED`. With `kFollow`, the headers set on the request (e.g. bearer
  tokens) and, unless the redirect changes the method, the body are sent to the
  redirect target, even if it is another origin.
- `retry_options`: forwarded to `SimpleURLLoader::SetRetryOptions()`; defaults
  to no retries.
- `timeout_duration`: forwarded to `SimpleURLLoader::SetTimeoutDuration()`;
  defaults to zero, i.e. no timeout.
- `url_replacements`: overrides the host, path, and/or query of
  `Endpoint::URL()` for this request.

```cpp
request.retry_options = {
    .max_retries = 1,
    .retry_mode = network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE};
request.timeout_duration = base::Seconds(30);
request.url_replacements.SetHost(hostname).SetPath("/v1/resource");
```

---

### 3. Adding request headers and receiving response headers

`Client<>` accepts requests with headers and can return responses with headers
if you wrap your request or response types with `WithHeaders<>` — the same type
safety applies.

```cpp
// Request with headers
WithHeaders<MyEndpoint::Request> request;
request.headers.SetHeader("X-Custom-Header", "value");

// Bearer tokens are common enough that there's a helper for them.
SetBearerToken(request, bearer_token);

Client<MyEndpoint>::Send(
    url_loader_factory_, std::move(request),
    base::BindOnce([](MyEndpoint::Response response) {}));
```

```cpp
// Response with headers
Client<MyEndpoint>::Send(
    url_loader_factory_, std::move(request),
    base::BindOnce([](WithHeaders<MyEndpoint::Response> response) {
      // response.headers
    }));
```

`response.headers` is set only when `response.status_code` is, i.e. on `net::OK`
with response headers received.

---

### 4. Cancelable requests

By default, requests are non-cancelable. To make a request cancelable, use the
`RequestCancelability::kCancelable` `template` parameter:

```cpp
RequestHandle request_handle =
    Client<MyEndpoint>::Send<RequestCancelability::kCancelable>(
        url_loader_factory_, std::move(request),
        base::BindOnce([](MyEndpoint::Response response) {}));

request_handle.reset();  // cancels the request
```

`RequestHandle` is an opaque, move-only owner of the underlying
`network::SimpleURLLoader`, so callers can only hold, move, or reset it.

Destroying the handle cancels the request **synchronously**: the response
callback can't run afterwards, since `SimpleURLLoader` guarantees that a loader
deleted before its callback runs never runs it.

A non-empty handle must be destroyed (or `reset()`) on the sequence that called
`Send<>()`; destroying it elsewhere `CHECK`-fails.

---

### 5. Mocking endpoints in tests

`Client<>::Send<>()` depends only on a `network::SharedURLLoaderFactory`, so in
tests you can construct and configure a `network::TestURLLoaderFactory`, then
pass `test_url_loader_factory_.GetSafeWeakWrapper()` to the code under test.

Helpers for registering typed mock responses are available in
`//brave/components/endpoint_client:test_support`.

This allows you to simulate transport errors, HTTP failures, and both
success/error bodies without introducing additional mock layers.

```cpp
MockResponseFor<LoginInit>(test_url_loader_factory_,
                           {.net_error = net::OK,
                            .status_code = net::HTTP_OK,
                            .body = [] {
                              LoginInit::Response::SuccessBody body;
                              body.login_token = "eyJhbGciOiJFUzI1NiIs...";
                              body.serialized_ke2 = "b4fb94e4f9d3d87c368d...";
                              return body;
                            }()});
```

If the request sets `url_replacements`, pass the same value as the last argument
to `MockResponseFor<>()`, so the mock matches the URL that is actually
requested.

#### When an interceptor is required

If multiple endpoints in your test share the same URL and differ only by HTTP
method, use a `network::TestURLLoaderFactory` interceptor to match the outgoing
request to the expected endpoint:

```cpp
test_url_loader_factory_.SetInterceptor(
    base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
      if (MatchesEndpoint<LoginInit>(request)) {
        MockResponseFor<LoginInit>(test_url_loader_factory_,
                                   {.net_error = net::OK,
                                    .status_code = net::HTTP_UNAUTHORIZED,
                                    .body = base::unexpected([] {
                                      LoginInit::Response::ErrorBody body;
                                      body.code = base::Value(14005);
                                      body.error = "incorrect email";
                                      body.status = 401;
                                      return body;
                                    }())});
      }
    }));
```

Like `MockResponseFor<>()`, `MatchesEndpoint<>()` takes the request's
`url_replacements` as an optional last argument.

#### Asserting on responses

Use the `ResponseEq()` gmock matcher to compare a received response against an
expected one, without defining `operator==()` for your body types. It compares
`net_error`, `status_code`, and the `body` field by field (and the `headers`,
for `WithHeaders<>` responses), and reports which one differs on failure.

```cpp
base::test::TestFuture<LoginInit::Response> future;
Client<LoginInit>::Send(test_url_loader_factory_.GetSafeWeakWrapper(),
                        std::move(request), future.GetCallback());
EXPECT_THAT(future.Take(), ResponseEq(expected_response));
```

JSON bodies are compared by their `ToValue()`, and Protobuf bodies by their
known fields. Both `ResponseEq()` and `MockResponseFor<>()` therefore require
JSON response bodies to provide `ToValue()` in addition to `FromValue()`; types
generated by the IDL compiler already do.

## Contributing

The `Client<>` API surface and behavioral semantics are intentionally
constrained, with ongoing plans to extend its capabilities as needed.

To keep the design coherent, please discuss proposed changes before opening pull
requests against `//brave/components/endpoint_client`.
