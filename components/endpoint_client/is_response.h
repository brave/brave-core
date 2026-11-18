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
inline constexpr bool kIsResponseWithoutBodies = false;

// A response without bodies is a Response<> with no body types.
template <>
inline constexpr bool kIsResponseWithoutBodies<Response<>> = true;

template <typename Response>
concept IsResponseWithoutBodies = kIsResponseWithoutBodies<Response>;

template <typename>
inline constexpr bool kIsResponseWithBodies = false;

// A response with bodies is a Response<> with a success and an error response
// body.
template <IsResponseBody SuccessBody, IsResponseBody ErrorBody>
inline constexpr bool kIsResponseWithBodies<Response<SuccessBody, ErrorBody>> =
    true;

template <typename Response>
concept IsResponseWithBodies = kIsResponseWithBodies<Response>;

// A response is either a response without bodies or a response with bodies.
template <typename Response>
concept IsResponse =
    IsResponseWithoutBodies<Response> || IsResponseWithBodies<Response>;

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_RESPONSE_H_
