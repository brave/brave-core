/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/json/json_writer.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/endpoint_client/endpoint_builder.h"
#include "net/http/http_request_headers.h"

// See //brave/components/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace endpoint_client {

template <endpoints::concepts::Endpoint Endpoint>
class Client {
  template <typename T>
  struct Parse {
    static auto FromValue(const base::Value& value) {
      return T::FromValue(value);
    }
  };

  template <typename... Ts>
  struct Parse<std::variant<Ts...>> {
    static auto FromValue(const base::Value& value) {
      std::optional<std::variant<Ts...>> result;
      (
          [&] {
            if (result) {
              return;
            }

            if (auto t = Ts::FromValue(value)) {
              result = std::move(*t);
            }
          }(),
          ...);
      return result;
    }
  };

 public:
  template <endpoints::concepts::SupportedBy<Endpoint> Request>
  static void Send(api_request_helper::APIRequestHelper& api_request_helper,
                   Request request,
                   Endpoint::template CallbackFor<Request> callback) {
    auto on_response = [](decltype(callback) callback,
                          api_request_helper::APIRequestResult result) {
      using Response = Endpoint::template ResponseFor<Request>;
      using Error = Endpoint::template ErrorFor<Request>;
      using Expected = Endpoint::template ExpectedFor<Request>;

      std::move(callback).Run(
          result.response_code(),
          result.Is2XXResponseCode()
              ? Expected(Parse<Response>::FromValue(result.value_body()))
              : base::unexpected(Parse<Error>::FromValue(result.value_body())));
    };

    const auto json = base::WriteJson(request.ToValue());
    CHECK(json) << "Failed to serialize request to JSON!";

    api_request_helper.Request(
        std::string(request.Method()), Endpoint::URL(), *json,
        "application/json",
        base::BindOnce(std::move(on_response), std::move(callback)),
        base::ToVector(request.headers.GetHeaderVector(),
                       [](const auto& header) {
                         return std::pair(header.key, header.value);
                       }));
  }
};

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_
