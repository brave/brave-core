/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_ENDPOINTS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_ENDPOINTS_H_

#include <concepts>
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
// 1. Add an IDL file for the request type under
//    //brave/components/brave_account/endpoints/requests
//    and a matching IDL file for the response type under
//    //brave/components/brave_account/endpoints/responses.
//
// 2. Define a struct that ties them together and returns the URL:
//
//    E.g.:
//    struct <EndpointType> {
//      using Request = <RequestType>;
//      using Response = <ResponseType>;
//      static GURL URL() { return Host().Resolve("/v2/accounts/<Path>"); }
//    };
//
// 3. Send the request with Client<<EndpointType>>::Send():
//    - pass your Request object
//    - provide a callback with the matching Response type
//      (the compiler will enforce this)
//
//    E.g.:
//    <RequestType> request;
//    Client<<EndpointType>>::Send(
//        api_request_helper,
//        request,
//        base::BindOnce(
//            [](int response_code,
//               std::optional<<ResponseType>> response) {
//              // ... handle response ...
//            }));

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

struct PasswordInit {
  using Request = PasswordInitRequest;
  using Response = PasswordInitResponse;
  static GURL URL();
};

struct PasswordFinalize {
  using Request = PasswordFinalizeRequest;
  using Response = PasswordFinalizeResponse;
  static GURL URL();
};

template <typename T>
concept Request = requires(T t) {
  { t.ToValue() } -> std::convertible_to<base::ValueView>;
};

template <typename T>
concept Response = requires(api_request_helper::APIRequestResult result) {
  { T::FromValue(result.value_body()) } -> std::same_as<std::optional<T>>;
};

template <typename T>
concept Endpoint = requires {
  typename T::Request;
  typename T::Response;
  { T::URL() } -> std::same_as<GURL>;
} && Request<typename T::Request> && Response<typename T::Response>;

template <Endpoint T>
struct Client {
  using Request = typename T::Request;
  using Response = typename T::Response;

  static void Send(
      api_request_helper::APIRequestHelper& api_request_helper,
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
        "POST", T::URL(), base::WriteJson(request.ToValue()).value_or(""),
        "application/json",
        base::BindOnce(std::move(on_response), std::move(callback)), headers);
  }
};

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_ENDPOINTS_H_
