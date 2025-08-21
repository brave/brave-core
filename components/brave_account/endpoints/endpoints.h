/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_ENDPOINTS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_ENDPOINTS_H_

#include <optional>
#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_writer.h"
#include "base/types/always_false.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_account/endpoints/requests/password_finalize.h"
#include "brave/components/brave_account/endpoints/requests/password_init.h"
#include "brave/components/brave_account/endpoints/responses/password_finalize.h"
#include "brave/components/brave_account/endpoints/responses/password_init.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

// To add a new endpoint:
//
// 1. Add a new request schema under
//    //brave/components/brave_account/endpoints/requests
//    and a new response schema under
//    //brave/components/brave_account/endpoints/responses.
//
// 2. Add an Endpoint<> specialization here:
//
//    template <>
//    struct Endpoint<requests::<Name>> {
//      using Response = responses::<Name>;
//      static GURL URL() { return Host().Resolve("/v2/accounts/<Path>"); }
//    };
//
// 3. Call Send() with the request type. From the Request, the system infers:
//    - the endpoint URL
//    - the corresponding Response type
//    - the correct callback signature `void(int, std::optional<Response>)`
//
//    This means you only need to provide the request and a matching callback;
//    everything else is determined automatically.
//
//    requests::<Name> request;
//    Send(api_request_helper, request,
//         base::BindOnce([](int response_code,
//                           std::optional<responses::<Name>> response) {
//           // ... handle response ...
//         }));

namespace brave_account::endpoints {

inline constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_account_endpoints",
                                        R"(
    semantics {
      sender: "Brave Account client"
      description:
        "Implements the creation or sign-in process for a Brave Account."
      trigger:
        "User attempts to create or sign in to a Brave Account from settings."
      user_data: {
        type: EMAIL
      }
      data:
        "Blinded cryptographic message for secure password setup "
        "and account email address."
        "Verification token for account activation and "
        "serialized cryptographic record for account finalization."
      destination: OTHER
      destination_other: "Brave Account service"
    }
    policy {
      cookies_allowed: NO
      policy_exception_justification:
        "These requests are essential for Brave Account creation and sign-in "
        "and cannot be disabled by policy."
    }
  )");

template <typename Request>
struct Endpoint {
  static_assert(base::AlwaysFalse<Request>,
                "No Endpoint specialization for this request type!");
};

template <>
struct Endpoint<requests::PasswordInit> {
  using Response = responses::PasswordInit;
  static GURL URL();
};

template <>
struct Endpoint<requests::PasswordFinalize> {
  using Response = responses::PasswordFinalize;
  static GURL URL();
};

template <typename Request>
using EndpointFor = Endpoint<Request>;

template <typename Request,
          typename Endpoint = EndpointFor<Request>,
          typename Response = typename Endpoint::Response>
void Send(api_request_helper::APIRequestHelper& api_request_helper,
          const Request& request,
          base::OnceCallback<void(int, std::optional<Response>)> callback,
          const base::flat_map<std::string, std::string>& headers = {}) {
  auto on_response = [](decltype(callback) cb,
                        api_request_helper::APIRequestResult result) {
    std::move(cb).Run(result.response_code(),
                      result.Is2XXResponseCode()
                          ? Response::FromValue(result.value_body())
                          : std::nullopt);
  };

  api_request_helper.Request(
      "POST", Endpoint::URL(), base::WriteJson(request.ToValue()).value_or(""),
      "application/json",
      base::BindOnce(std::move(on_response), std::move(callback)), headers);
}

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_ENDPOINTS_H_
