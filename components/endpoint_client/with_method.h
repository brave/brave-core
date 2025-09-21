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

namespace endpoints::detail {

enum class Method {
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

template <RequestBody Body, Method M>
struct WithMethod : Body {
  static constexpr std::string_view Method() {
    if constexpr (M == Method::kConnect) {
      return net::HttpRequestHeaders::kConnectMethod;
    } else if constexpr (M == Method::kDelete) {
      return net::HttpRequestHeaders::kDeleteMethod;
    } else if constexpr (M == Method::kGet) {
      return net::HttpRequestHeaders::kGetMethod;
    } else if constexpr (M == Method::kHead) {
      return net::HttpRequestHeaders::kHeadMethod;
    } else if constexpr (M == Method::kOptions) {
      return net::HttpRequestHeaders::kOptionsMethod;
    } else if constexpr (M == Method::kPatch) {
      return net::HttpRequestHeaders::kPatchMethod;
    } else if constexpr (M == Method::kPost) {
      return net::HttpRequestHeaders::kPostMethod;
    } else if constexpr (M == Method::kPut) {
      return net::HttpRequestHeaders::kPutMethod;
    } else if constexpr (M == Method::kTrace) {
      return net::HttpRequestHeaders::kTraceMethod;
    } else if constexpr (M == Method::kTrack) {
      return net::HttpRequestHeaders::kTrackMethod;
    } else {
      static_assert(
          base::AlwaysFalse<std::integral_constant<detail::Method, M>>,
          "Unhandled Method enumerator!");
    }
  }
};

}  // namespace endpoints::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_HTTP_METHOD_H_
