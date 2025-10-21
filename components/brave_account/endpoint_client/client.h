/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_

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
#include "brave/components/brave_account/endpoint_client/concepts.h"
#include "net/http/http_request_headers.h"

// See //brave/components/brave_account/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace brave_account::endpoint_client {

template <concepts::Request T>
struct WithHeaders : T {
  net::HttpRequestHeaders headers;
};

template <concepts::Endpoint T>
class Client {
  using Ticket = api_request_helper::APIRequestHelper::Ticket;
  using Request = typename T::Request;
  using Response = typename T::Response;
  using Error = typename T::Error;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;
  using Callback = base::OnceCallback<void(int, Expected)>;

 public:
  static Ticket Send(api_request_helper::APIRequestHelper& api_request_helper,
                     Request request,
                     Callback callback) {
    return SendImpl(api_request_helper, std::move(request),
                    net::HttpRequestHeaders(), std::move(callback));
  }

  static Ticket Send(api_request_helper::APIRequestHelper& api_request_helper,
                     WithHeaders<Request> request,
                     Callback callback) {
    auto headers = std::move(request.headers);
    return SendImpl(api_request_helper, std::move(request), std::move(headers),
                    std::move(callback));
  }

 private:
  static Ticket SendImpl(
      api_request_helper::APIRequestHelper& api_request_helper,
      Request request,
      net::HttpRequestHeaders headers,
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

    return api_request_helper.Request(
        std::string(T::Method()), T::URL(), json, "application/json",
        base::BindOnce(std::move(on_response), std::move(callback)),
        base::ToVector(headers.GetHeaderVector(), [](const auto& header) {
          return std::pair(header.key, header.value);
        }));
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
