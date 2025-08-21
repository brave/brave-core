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
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/json/json_writer.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_account/endpoint_client/concepts.h"

namespace brave_account::endpoint_client {

// See //brave/components/brave_account/endpoint_client/README.md
// for design, motivation, usage, and examples.
template <concepts::Endpoint T>
class Client {
  using Request = typename T::Request;
  using Response = typename T::Response;
  using Error = typename T::Error;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;

 public:
  static void Send(
      api_request_helper::APIRequestHelper& api_request_helper,
      const Request& request,
      base::OnceCallback<void(Expected)> callback,
      const base::flat_map<std::string, std::string>& headers = {}) {
    auto on_response = [](decltype(callback) cb,
                          api_request_helper::APIRequestResult result) {
      std::move(cb).Run(
          result.Is2XXResponseCode()
              ? Expected(Response::FromValue(result.value_body()))
              : base::unexpected(Error::FromValue(result.value_body())));
    };

    const auto json = base::WriteJson(request.ToValue());
    CHECK(json) << "Failed to serialize request to JSON!";

    api_request_helper.Request(
        std::string(T::Method()), T::URL(), *json, "application/json",
        base::BindOnce(std::move(on_response), std::move(callback)), headers);
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
