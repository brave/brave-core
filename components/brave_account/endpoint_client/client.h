/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_

#include <concepts>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/json/json_writer.h"
#include "base/types/expected.h"
#include "base/types/is_instantiation.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/http/http_request_headers.h"

// See //brave/components/brave_account/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace brave_account::endpoint_client {

template <IsEndpoint T>
class Client {
  using Response = typename T::Response;
  using Error = typename T::Error;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;
  using Callback = base::OnceCallback<void(int, Expected)>;

 public:
  template <typename Request>
    requires(std::same_as<detail::MaybeStripWithHeaders<Request>,
                          typename T::Request>)
  static void Send(api_request_helper::APIRequestHelper& api_request_helper,
                   Request request,
                   Callback callback) {
    auto on_response = [](Callback callback,
                          api_request_helper::APIRequestResult result) {
      std::move(callback).Run(
          result.response_code(),
          result.Is2XXResponseCode()
              ? Expected(Response::FromValue(result.value_body()))
              : base::unexpected(Error::FromValue(result.value_body())));
    };

    std::string json;
    if (auto dict = request.ToValue(); !dict.empty()) {
      json = base::WriteJson(std::move(dict)).value_or("");
      CHECK(!json.empty()) << "Failed to serialize request to JSON!";
    }

    net::HttpRequestHeaders headers;
    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      headers = std::move(request.headers);
    }

    api_request_helper.Request(
        std::string(Request::Method()), T::URL(), json, "application/json",
        base::BindOnce(std::move(on_response), std::move(callback)),
        base::ToVector(headers.GetHeaderVector(), [](const auto& header) {
          return std::pair(header.key, header.value);
        }));
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
