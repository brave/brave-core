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

namespace detail {

template <typename>
struct MaybeStripWithHeadersImpl;

template <typename T>
  requires(detail::Request<T> || detail::Response<T>)
struct MaybeStripWithHeadersImpl<T> : std::type_identity<T> {};

template <typename T>
struct MaybeStripWithHeadersImpl<WithHeaders<T>>
    : MaybeStripWithHeadersImpl<T> {};

// This partial participates only if each alternative
// becomes a detail::Response after stripping.
template <typename... Ts>
  requires(detail::Response<typename MaybeStripWithHeadersImpl<Ts>::type> &&
           ...)
struct MaybeStripWithHeadersImpl<std::variant<Ts...>>
    : std::type_identity<
          std::variant<typename MaybeStripWithHeadersImpl<Ts>::type...>> {};

}  // namespace detail

template <typename T>
using MaybeStripWithHeaders =
    typename detail::MaybeStripWithHeadersImpl<T>::type;

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_WITH_HEADERS_H_
