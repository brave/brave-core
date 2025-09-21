/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_PARSE_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_PARSE_H_

#include <optional>
#include <utility>
#include <variant>

#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "brave/components/endpoint_client/response.h"
#include "brave/components/endpoint_client/with_headers.h"
#include "net/http/http_response_headers.h"

namespace endpoint_client::detail {

template <endpoints::detail::Response Response>
struct Parse {
  static auto From(const std::optional<base::Value>& value,
                   scoped_refptr<net::HttpResponseHeaders> = nullptr) {
    return value ? Response::FromValue(*value) : std::nullopt;
  }
};

template <endpoints::detail::Response Response>
struct Parse<endpoints::WithHeaders<Response>> {
  static auto From(const std::optional<base::Value>& value,
                   scoped_refptr<net::HttpResponseHeaders> headers) {
    std::optional<endpoints::WithHeaders<Response>> result;
    auto response = Parse<Response>::From(value);
    if (!response) {
      return result;
    }

    result.emplace(std::move(*response));
    result->headers = std::move(headers);
    return result;
  }
};

template <endpoints::detail::Response... Responses>
struct Parse<std::variant<Responses...>> {
  static auto From(const std::optional<base::Value>& value,
                   scoped_refptr<net::HttpResponseHeaders> headers) {
    std::optional<std::variant<Responses...>> result;
    (
        [&] {
          if (result) {
            return;
          }

          if (auto response = Parse<Responses>::From(value, headers)) {
            result = std::move(*response);
          }
        }(),
        ...);
    return result;
  }
};

}  // namespace endpoint_client::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_PARSE_H_
