/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_H_

#include "brave/components/brave_account/endpoint_client/is_request_body.h"

namespace brave_account::endpoint_client::detail {

enum class Method;
template <IsRequestBody, Method>
struct Request;

template <typename>
inline constexpr bool kIsRequest = false;

template <IsRequestBody RequestBody, Method M>
inline constexpr bool kIsRequest<Request<RequestBody, M>> = true;

template <typename T>
concept IsRequest = kIsRequest<T>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_H_
