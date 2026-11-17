/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_RESPONSE_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_RESPONSE_H_

#include "brave/components/endpoint_client/is_response_body.h"
#include "brave/components/endpoint_client/response.h"

namespace endpoint_client::internal {

template <typename>
inline constexpr bool kIsResponse = false;

template <IsResponseBody SuccessBody, IsResponseBody ErrorBody>
inline constexpr bool kIsResponse<Response<SuccessBody, ErrorBody>> = true;

// A response is a Response<> whose success and error bodies are each either a
// JSON or a Protobuf response body.
template <typename Response>
concept IsResponse = kIsResponse<Response>;

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_RESPONSE_H_
