/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_H_

#include "brave/components/brave_account/endpoint_client/is_response_body.h"
#include "brave/components/brave_account/endpoint_client/response.h"

namespace brave_account::endpoint_client::detail {

template <typename>
inline constexpr bool kIsJSONResponse = false;

template <IsJSONResponseBody SuccessBody, IsJSONResponseBody ErrorBody>
inline constexpr bool kIsJSONResponse<Response<SuccessBody, ErrorBody>> = true;

// A JSON response is a Response<> whose bodies are JSON response bodies.
template <typename Response>
concept IsJSONResponse = kIsJSONResponse<Response>;

template <typename>
inline constexpr bool kIsProtobufResponse = false;

template <IsProtobufResponseBody SuccessBody, IsProtobufResponseBody ErrorBody>
inline constexpr bool kIsProtobufResponse<Response<SuccessBody, ErrorBody>> =
    true;

// A Protobuf response is a Response<> whose bodies are Protobuf response
// bodies.
template <typename Response>
concept IsProtobufResponse = kIsProtobufResponse<Response>;

// A response is either a JSON response or a Protobuf response.
template <typename Response>
concept IsResponse = IsJSONResponse<Response> || IsProtobufResponse<Response>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_H_
