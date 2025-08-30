# Endpoint Client

This module provides a **generic client abstraction** for calling endpoints.  
It reduces boilerplate and centralizes:
- building the request body
- serializing requests to JSON
- submitting HTTP requests via `APIRequestHelper`
- parsing structured responses or errors into strongly typed C++ objects

With C++20 concepts, the client enforces **at compile time** that each endpoint type provides:
- `Request`
- `Response`
- `Error`
- `URL()`
- `Method()`

---

## Motivation

In the browser today, each endpoint implementation typically reimplements logic to:
- construct and serialize its request payload
- send it through `APIRequestHelper` (or another abstraction)
- handle response codes
- deserialize the body
- map failures

This leads to **duplicate code** and inconsistent error handling across different endpoints.  
The client abstraction aims to:

1. **unify request/response handling** — one pattern for all endpoints  
2. **enforce correctness at compile time** — endpoints that don't satisfy the `Endpoint` concept won't compile  
3. **encapsulate complexity** — call sites provide only:  
   - a `Request` object  
   - a callback consuming `base::expected<std::optional<Response>, std::optional<Error>>`

Together, this standardizes endpoint handling, eliminates per-endpoint duplication, and improves safety with C++20 concepts and `base::expected<>` — making endpoints easier to implement, test, maintain, and reason about, while also being less error-prone.

---

## Usage

You can hand-write `Request`, `Response`, and `Error`, but this abstraction is primarily geared toward using the JSON schema/IDL compiler, which generates them for you (`ToValue()`, `FromValue()`, etc.). You then provide a small endpoint binder and call the generic client.

1. **add schemas/IDL** for request, response, and error under:  
   `//brave/components/brave_account/endpoints`

2. **define the endpoint binder** (ties types together and provides URL/method), e.g.:
   ```cpp
   struct <EndpointType> {
     using Request = <RequestType>;
     using Response = <ResponseType>;
     using Error = <ErrorType>;
     static GURL URL() { return Host().Resolve("/v2/accounts/<Path>"); }
     static std::string_view Method() { return "<METHOD>"; }
   };
   ```
3. send the request with `Client<<EndpointType>>::Send()`:
   ```cpp
   <EndpointType>::Request request;
   Client<<EndpointType>>::Send(
       api_request_helper,
       request,
       base::BindOnce(
           [](base::expected<std::optional<<EndpointType>::Response>,
                             std::optional<<EndpointType>::Error>> reply) {
             // ... handle server reply ...
           }));
   ```

---

#### A note on `base::expected<std::optional<Response>, std::optional<Error>>`:

- **success case** (`has_value()`):
  - returned for **2xx** statuses
  - `value()` is a `std::optional<Response>`:
    - **`Response`** — body parsed successfully into your `Response` type
    - **`std::nullopt`** — body was missing or failed to parse

- **error case** (`!has_value()`):
  - returned for **non-2xx** statuses
  - `error()` is a `std::optional<Error>`:
    - **`Error`** — body parsed successfully into your `Error` type
    - **`std::nullopt`** — body was missing or failed to parse
