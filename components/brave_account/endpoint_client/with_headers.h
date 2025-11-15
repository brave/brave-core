/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_WITH_HEADERS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_WITH_HEADERS_H_

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_account/endpoint_client/is_request.h"
#include "brave/components/brave_account/endpoint_client/is_response.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"

namespace brave_account::endpoint_client {

template <typename T>
struct WithHeaders {
  static_assert(false,
                "T must satisfy detail::IsRequest or detail::IsResponse!");
};

// Wrapper that extends an IsRequest type T
// with an additional net::HttpRequestHeaders member.
template <detail::IsRequest T>
struct WithHeaders<T> : T {
  net::HttpRequestHeaders headers;
};

// Wrapper that extends an IsResponse type T
// with an additional scoped_refptr<net::HttpResponseHeaders> member.
template <detail::IsResponse T>
struct WithHeaders<T> : T {
  scoped_refptr<net::HttpResponseHeaders> headers;
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_WITH_HEADERS_H_
