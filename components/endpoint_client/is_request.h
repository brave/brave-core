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
inline constexpr bool kIsJSONRequest = false;

template <Method M, IsJSONRequestBody RequestBody>
  requires(MethodAllowsBody(M))
inline constexpr bool kIsJSONRequest<Request<M, RequestBody>> = true;

// A JSON request is a Request<> whose body is a JSON request body.
template <typename Request>
concept IsJSONRequest = kIsJSONRequest<Request>;

template <typename>
inline constexpr bool kIsProtobufRequest = false;

template <Method M, IsProtobufRequestBody RequestBody>
  requires(MethodAllowsBody(M))
inline constexpr bool kIsProtobufRequest<Request<M, RequestBody>> = true;

// A Protobuf request is a Request<> whose body is a Protobuf request body.
template <typename Request>
concept IsProtobufRequest = kIsProtobufRequest<Request>;

// A request is a body-less request, a JSON request, or a Protobuf request.
template <typename Request>
concept IsRequest = IsBodylessRequest<Request> || IsJSONRequest<Request> ||
                    IsProtobufRequest<Request>;

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_REQUEST_H_
