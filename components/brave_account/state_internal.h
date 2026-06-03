/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_INTERNAL_H_

#include <concepts>
#include <type_traits>
#include <utility>

#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/error_body.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace brave_account::internal {

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
auto MakeRequest() {
  Request request;
  request.network_traffic_annotation_tag =
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
  return request;
}

template <typename Error>
using ClientErrorOf = typename std::remove_cvref_t<
    decltype(std::declval<Error>().get_client_error())>::element_type;

template <typename Error>
using ClientErrorCodeOf = decltype(ClientErrorOf<Error>::error_code);

template <typename Error>
using ServerErrorOf = typename std::remove_cvref_t<
    decltype(std::declval<Error>().get_server_error())>::element_type;

template <typename Error>
using ServerErrorCodeOf = decltype(ServerErrorOf<Error>::error_code);

template <typename Error, typename ClientErrorCode>
  requires std::same_as<ClientErrorCode, ClientErrorCodeOf<Error>>
auto MakeClientError(ClientErrorCode client_error_code) {
  return Error::NewClientError(ClientErrorOf<Error>::New(client_error_code));
}

template <typename Error>
auto MakeCalledInWrongStateError() {
  return base::unexpected(
      MakeClientError<Error>(ClientErrorCodeOf<Error>::kCalledInWrongState));
}

template <typename ServerErrorCode>
auto MakeServerErrorCode(endpoints::ErrorBody error_body) {
  if (error_body.code.is_none()) {
    return ServerErrorCode::kNull;
  } else if (error_body.code.is_int()) {
    if (const auto error_code =
            static_cast<ServerErrorCode>(error_body.code.GetInt());
        mojom::IsKnownEnumValue(error_code)) {
      return error_code;
    }
  }

  return ServerErrorCode::kUnknown;
}

template <typename Error, typename ServerErrorCode>
  requires std::same_as<ServerErrorCode, ServerErrorCodeOf<Error>>
auto MakeServerError(int net_error_or_http_status,
                     ServerErrorCode server_error_code) {
  return Error::NewServerError(
      ServerErrorOf<Error>::New(net_error_or_http_status, server_error_code));
}

template <typename Error>
auto MakeServerError(int net_error_or_http_status,
                     endpoints::ErrorBody error_body) {
  return MakeServerError<Error>(
      net_error_or_http_status,
      MakeServerErrorCode<ServerErrorCodeOf<Error>>(std::move(error_body)));
}

}  // namespace brave_account::internal

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_INTERNAL_H_
