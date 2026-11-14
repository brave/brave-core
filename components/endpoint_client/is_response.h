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
inline constexpr bool kIsBodylessResponse = false;

template <>
inline constexpr bool kIsBodylessResponse<Response<>> = true;

// A body-less response is a Response<> without response bodies.
template <typename Response>
concept IsBodylessResponse = kIsBodylessResponse<Response>;

template <typename>
inline constexpr bool kIsResponseWithBodies = false;

template <IsResponseBody SuccessBody, IsResponseBody ErrorBody>
inline constexpr bool kIsResponseWithBodies<Response<SuccessBody, ErrorBody>> =
    true;

// A response with bodies is a Response<> with a success and an error response
// body, each of which is either a JSON or a Protobuf response body.
template <typename Response>
concept IsResponseWithBodies = kIsResponseWithBodies<Response>;

// A response is either a body-less response or a response with bodies.
template <typename Response>
concept IsResponse =
    IsBodylessResponse<Response> || IsResponseWithBodies<Response>;

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_RESPONSE_H_
