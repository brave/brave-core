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
#include "brave/components/endpoint_client/is_response.h"
#include "brave/components/endpoint_client/with_headers.h"
#include "net/http/http_response_headers.h"

namespace endpoint_client::detail {

template <IsResponse Reponse>
struct Parse {
  static auto From(const std::optional<base::Value>& value,
                   scoped_refptr<net::HttpResponseHeaders> = nullptr) {
    return value ? Reponse::FromValue(*value) : std::nullopt;
  }
};

template <IsResponse Reponse>
struct Parse<WithHeaders<Reponse>> {
  static auto From(const std::optional<base::Value>& value,
                   scoped_refptr<net::HttpResponseHeaders> headers) {
    std::optional<WithHeaders<Reponse>> result;
    auto response = Parse<Reponse>::From(value);
    if (!response) {
      return result;
    }

    result.emplace(std::move(*response));
    result->headers = std::move(headers);
    return result;
  }
};

template <IsResponse... Reponses>
struct Parse<std::variant<Reponses...>> {
  static auto From(const std::optional<base::Value>& value,
                   scoped_refptr<net::HttpResponseHeaders> headers) {
    std::optional<std::variant<Reponses...>> result;
    (
        [&] {
          if (result) {
            return;
          }

          if (auto response = Parse<Reponses>::From(value, headers)) {
            result = std::move(*response);
          }
        }(),
        ...);
    return result;
  }
};

}  // namespace endpoint_client::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_PARSE_H_
