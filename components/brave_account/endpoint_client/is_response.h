/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_H_

#include "brave/components/brave_account/endpoint_client/is_response_body.h"
#include "brave/components/brave_account/endpoint_client/response.h"

namespace brave_account::endpoint_client::detail {

// Primary template: a type does not satisfy IsResponse unless
// matched by the partial specialization below.
template <typename>
inline constexpr bool kIsResponse = false;

// Partial specialization: Response<T, E> satisfies IsResponse if
// T and E satisfy IsResponseBody.
template <IsResponseBody T, IsResponseBody E>
inline constexpr bool kIsResponse<Response<T, E>> = true;

// Concept: a type satisfies IsResponse if
// its kIsResponse specialization evaluates to true.
template <typename T>
concept IsResponse = kIsResponse<T>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_H_
