# Endpoint Client

This component provides a **generic, type-safe client abstraction** for calling HTTP endpoints.

It centralizes and standardizes:

- serializing strongly typed C++ request objects to JSON
- issuing requests via `network::SimpleURLLoader`
- mapping HTTP status codes to typed success/error bodies
- deserializing JSON responses into strongly typed C++ objects
- surfacing transport errors, HTTP status, and parse failures consistently

## Motivation

Without a shared abstraction, each endpoint reimplements the same mechanics described above.

This repetition leads to:

- duplicated infrastructure code
- inconsistencies in error handling
- fragile, stringly-typed JSON construction/parsing
- avoidable runtime failures

This abstraction consolidates these concerns behind a compile-time-validated interface and enforces correctness guarantees through C++ `concept`s and `base::expected<>`.

## Usage

`Client<>` integrates with the JSON schema/IDL compiler, which auto-generates request and response types implementing `ToValue()` and `FromValue()`.

Hand-written types are supported, but the IDL workflow is recommended.

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
  //   "initiatingServiceName": "accounts",
  //   "serializedKE1": "0e85b8e68bd22965e15e..."
  // }
  dictionary LoginInitRequestBody {
    DOMString email;
    DOMString initiatingServiceName;
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

Use the `generated_types` template in your `BUILD.gn` to generate C++ request/response classes from the IDL.

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
    POST<LoginInitRequestBody>,
    Response<LoginInitSuccessBody, LoginInitErrorBody>>;

}  // namespace brave_account::endpoints
```

The `BraveEndpoint<>` template simplifies defining endpoints for Brave services. It uses `brave_domains::GetServicesDomain()` under the hood to automatically resolve the service domain to the correct environment.

For non-Brave services, you can define an endpoint manually:

```cpp
struct MyEndpoint {
  using Request = POST<MyRequestBody>;
  using Response = Response<MySuccessBody, MyErrorBody>;

  static GURL URL() {
    return GURL("https://myservice.com/my_endpoint");
  }
};
```

---

### 2. Send the request and receive the response

`Client<>` enforces type safety: only the declared request type is accepted, and only the declared response type is returned.

```cpp
LoginInit::Request request;
request.network_traffic_annotation_tag =
    net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
request.email = "user@example.com";
request.initiating_service_name = "accounts";
request.serialized_ke1 = "0e85b8e68bd22965e15e...";

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

---

### 3. Adding request headers and receiving response headers

`Client<>` accepts requests with headers and can return responses with headers if you wrap your request or response types with `WithHeaders<>` — the same type safety applies.

```cpp
// Request with headers
WithHeaders<MyEndpoint::Request> request;
request.headers.SetHeader(net::HttpRequestHeaders::kAuthorization,
                          base::StrCat({"Bearer ", bearer_token}));

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

---

### 4. Cancelable requests

By default, requests are non-cancelable. To make a request cancelable, use the `RequestCancelability::kCancelable` template parameter:

```cpp
RequestHandle request_handle =
    Client<MyEndpoint>::Send<RequestCancelability::kCancelable>(
        url_loader_factory_, std::move(request),
        base::BindOnce([](MyEndpoint::Response response) {}));

request_handle.reset();  // cancels the request
```

`RequestHandle` is a type-erased `std::unique_ptr<>` to the underlying `network::SimpleURLLoader` with a sequence-aware deleter, so you can safely pass it across sequences. Destroying the handle cancels the request.

---

### 5. Mocking endpoints in tests

`Client<>::Send<>()` depends only on a `network::SharedURLLoaderFactory`, so in tests you can construct and configure a `network::TestURLLoaderFactory`, then pass `test_url_loader_factory_.GetSafeWeakWrapper()` to the code under test.

Helpers for registering typed mock responses are available in `//brave/components/endpoint_client:test_support`.

This allows you to simulate transport errors, HTTP failures, and both success/error bodies without introducing additional mock layers.

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

#### When an interceptor is required

If multiple endpoints in your test share the same URL and differ only by HTTP method, use a `network::TestURLLoaderFactory` interceptor to match the outgoing request to the expected endpoint:

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

## Contributing

The `Client<>` API surface and behavioral semantics are intentionally constrained, with ongoing plans to extend its capabilities as needed.

To keep the design coherent, please discuss proposed changes before opening pull requests against `//brave/components/endpoint_client`.
