/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_HTTP_METHOD_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_HTTP_METHOD_H_

#include <string_view>
#include <type_traits>

#include "base/types/always_false.h"
#include "brave/components/endpoint_client/request.h"
#include "net/http/http_request_headers.h"

namespace endpoints {
namespace detail {

enum class HTTPMethod {
  kConnect,
  kDelete,
  kGet,
  kHead,
  kOptions,
  kPatch,
  kPost,
  kPut,
  kTrace,
  kTrack
};

template <HasToValue Body, HTTPMethod M>
struct WithHTTPMethod : Body {
  static constexpr std::string_view Method() {
    if constexpr (M == HTTPMethod::kConnect) {
      return net::HttpRequestHeaders::kConnectMethod;
    } else if constexpr (M == HTTPMethod::kDelete) {
      return net::HttpRequestHeaders::kDeleteMethod;
    } else if constexpr (M == HTTPMethod::kGet) {
      return net::HttpRequestHeaders::kGetMethod;
    } else if constexpr (M == HTTPMethod::kHead) {
      return net::HttpRequestHeaders::kHeadMethod;
    } else if constexpr (M == HTTPMethod::kOptions) {
      return net::HttpRequestHeaders::kOptionsMethod;
    } else if constexpr (M == HTTPMethod::kPatch) {
      return net::HttpRequestHeaders::kPatchMethod;
    } else if constexpr (M == HTTPMethod::kPost) {
      return net::HttpRequestHeaders::kPostMethod;
    } else if constexpr (M == HTTPMethod::kPut) {
      return net::HttpRequestHeaders::kPutMethod;
    } else if constexpr (M == HTTPMethod::kTrace) {
      return net::HttpRequestHeaders::kTraceMethod;
    } else if constexpr (M == HTTPMethod::kTrack) {
      return net::HttpRequestHeaders::kTrackMethod;
    } else {
      static_assert(base::AlwaysFalse<std::integral_constant<HTTPMethod, M>>,
                    "Unhandled HTTPMethod enum!");
    }
  }
};

}  // namespace detail

template <detail::HasToValue Body>
using CONNECT = detail::WithHTTPMethod<Body, detail::HTTPMethod::kConnect>;

template <detail::HasToValue Body>
using DELETE = detail::WithHTTPMethod<Body, detail::HTTPMethod::kDelete>;

template <detail::HasToValue Body>
using GET = detail::WithHTTPMethod<Body, detail::HTTPMethod::kGet>;

template <detail::HasToValue Body>
using HEAD = detail::WithHTTPMethod<Body, detail::HTTPMethod::kHead>;

template <detail::HasToValue Body>
using OPTIONS = detail::WithHTTPMethod<Body, detail::HTTPMethod::kOptions>;

template <detail::HasToValue Body>
using PATCH = detail::WithHTTPMethod<Body, detail::HTTPMethod::kPatch>;

template <detail::HasToValue Body>
using POST = detail::WithHTTPMethod<Body, detail::HTTPMethod::kPost>;

template <detail::HasToValue Body>
using PUT = detail::WithHTTPMethod<Body, detail::HTTPMethod::kPut>;

template <detail::HasToValue Body>
using TRACE = detail::WithHTTPMethod<Body, detail::HTTPMethod::kTrace>;

template <detail::HasToValue Body>
using TRACK = detail::WithHTTPMethod<Body, detail::HTTPMethod::kTrack>;

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_HTTP_METHOD_H_
