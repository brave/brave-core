/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_H_

#include "brave/components/brave_account/endpoint_client/is_request_body.h"
#include "brave/components/brave_account/endpoint_client/request.h"

namespace brave_account::endpoint_client::detail {

template <typename>
inline constexpr bool kIsJSONRequest = false;

template <IsJSONRequestBody RequestBody, Method M>
inline constexpr bool kIsJSONRequest<Request<RequestBody, M>> = true;

// A JSON request is a Request<> whose body is a JSON request body.
template <typename Request>
concept IsJSONRequest = kIsJSONRequest<Request>;

template <typename>
inline constexpr bool kIsProtobufRequest = false;

template <IsProtobufRequestBody RequestBody, Method M>
inline constexpr bool kIsProtobufRequest<Request<RequestBody, M>> = true;

// A Protobuf request is a Request<> whose body is a Protobuf request body.
template <typename Request>
concept IsProtobufRequest = kIsProtobufRequest<Request>;

// A request is either a JSON request or a Protobuf request.
template <typename Request>
concept IsRequest = IsJSONRequest<Request> || IsProtobufRequest<Request>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_H_
