/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_

#include "base/memory/scoped_refptr.h"
#include "brave/components/endpoint_client/request.h"
#include "brave/components/endpoint_client/response.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"

namespace endpoints {
template <typename>
struct WithHeaders;

template <detail::Request Request>
struct WithHeaders<Request> : Request {
  net::HttpRequestHeaders headers;
};

template <detail::Response Response>
struct WithHeaders<Response> : Response {
  scoped_refptr<net::HttpResponseHeaders> headers;
};

namespace detail {
template <typename T>
concept HasHeaders =
    (Request<T> && requires(T& t) {
      { t.headers } -> std::same_as<net::HttpRequestHeaders&>;
    }) || (Response<T> && requires(T& t) {
      { t.headers } -> std::same_as<scoped_refptr<net::HttpResponseHeaders>&>;
    });

}  // namespace detail

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_
