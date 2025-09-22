/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_

#include "base/memory/scoped_refptr.h"
#include "base/types/always_false.h"
#include "base/types/is_instantiation.h"
#include "brave/components/endpoint_client/is_request.h"
#include "brave/components/endpoint_client/response.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"

namespace endpoint_client {

// Primary template: a type cannot be wrapped in WithHeaders<> unless
// matched by a partial specialization below.
template <typename T>
struct WithHeaders {
  static_assert(base::AlwaysFalse<T>,
                "T must satisfy endpoint_client::detail::IsRequest or "
                "endpoint_client::detail::Response!");
};

// Partial specialization: wraps an IsRequest with HTTP request headers.
template <detail::IsRequest Request>
struct WithHeaders<Request> : Request {
  net::HttpRequestHeaders headers;
};

// Partial specialization: wraps a Response with HTTP response headers.
template <detail::Response Rsp>
struct WithHeaders<Rsp> : Rsp {
  scoped_refptr<net::HttpResponseHeaders> headers;
};

namespace detail {

// Concept: true if T is an instantiation of WithHeaders<>.
template <typename T>
concept IsWithHeaders = base::is_instantiation<T, WithHeaders>;

}  // namespace detail

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_
