/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_

#include "base/memory/scoped_refptr.h"
#include "base/types/always_false.h"
#include "brave/components/endpoint_client/request.h"
#include "brave/components/endpoint_client/response.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"

namespace endpoints {

template <typename T>
struct WithHeaders {
  static_assert(base::AlwaysFalse<T>,
                "Invalid use of WithHeaders<T>: "
                "T must satisfy endpoints::detail::Request or "
                "endpoints::detail::Response!");
};

template <detail::Request Request>
struct WithHeaders<Request> : Request {
  net::HttpRequestHeaders headers;
};

template <detail::Response Response>
struct WithHeaders<Response> : Response {
  scoped_refptr<net::HttpResponseHeaders> headers;
};

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_
