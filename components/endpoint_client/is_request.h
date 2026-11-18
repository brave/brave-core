/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_REQUEST_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_REQUEST_H_

#include "brave/components/endpoint_client/is_request_body.h"
#include "brave/components/endpoint_client/request.h"

namespace endpoint_client::internal {

template <typename>
inline constexpr bool kIsRequestWithoutBody = false;

// A request without a body is a Request<> with only an HTTP method.
template <Method M>
inline constexpr bool kIsRequestWithoutBody<Request<M>> = true;

template <typename Request>
concept IsRequestWithoutBody = kIsRequestWithoutBody<Request>;

template <typename>
inline constexpr bool kIsRequestWithBody = false;

// A request with a body is a Request<> with a request body whose method allows
// one.
template <Method M, IsRequestBody RequestBody>
  requires(MethodAllowsBody(M))
inline constexpr bool kIsRequestWithBody<Request<M, RequestBody>> = true;

template <typename Request>
concept IsRequestWithBody = kIsRequestWithBody<Request>;

// A request is either a request without a body or a request with a body.
template <typename Request>
concept IsRequest = IsRequestWithoutBody<Request> || IsRequestWithBody<Request>;

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_REQUEST_H_
