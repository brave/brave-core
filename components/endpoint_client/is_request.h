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
inline constexpr bool kIsBodylessRequest = false;

template <Method M>
inline constexpr bool kIsBodylessRequest<Request<M>> = true;

// A body-less request is a Request<> without a request body.
template <typename Request>
concept IsBodylessRequest = kIsBodylessRequest<Request>;

template <typename>
inline constexpr bool kIsRequestWithBody = false;

template <Method M, IsRequestBody RequestBody>
  requires(MethodAllowsBody(M))
inline constexpr bool kIsRequestWithBody<Request<M, RequestBody>> = true;

// A request with a body is a Request<> with a request body whose method allows
// one.
template <typename Request>
concept IsRequestWithBody = kIsRequestWithBody<Request>;

// A request is either a body-less request or a request with a body.
template <typename Request>
concept IsRequest = IsBodylessRequest<Request> || IsRequestWithBody<Request>;

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_REQUEST_H_
