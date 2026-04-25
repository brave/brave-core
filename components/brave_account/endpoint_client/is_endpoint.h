/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_ENDPOINT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_ENDPOINT_H_

#include <concepts>

#include "brave/components/brave_account/endpoint_client/is_request.h"
#include "brave/components/brave_account/endpoint_client/is_response.h"
#include "url/gurl.h"

namespace brave_account::endpoint_client {

namespace detail {

// An endpoint defines Request and Response type aliases and a static URL().
template <typename Endpoint>
concept IsEndpointLike = requires {
  typename Endpoint::Request;
  typename Endpoint::Response;
  { Endpoint::URL() } -> std::same_as<GURL>;
};

// A JSON endpoint has a JSON request and response type.
template <typename Endpoint>
concept IsJSONEndpoint =
    IsEndpointLike<Endpoint> && IsJSONRequest<typename Endpoint::Request> &&
    IsJSONResponse<typename Endpoint::Response>;

// A Protobuf endpoint has a Protobuf request and response type.
template <typename Endpoint>
concept IsProtobufEndpoint =
    IsEndpointLike<Endpoint> && IsProtobufRequest<typename Endpoint::Request> &&
    IsProtobufResponse<typename Endpoint::Response>;

}  // namespace detail

// An endpoint is either a JSON endpoint or a Protobuf endpoint.
template <typename Endpoint>
concept IsEndpoint =
    detail::IsJSONEndpoint<Endpoint> || detail::IsProtobufEndpoint<Endpoint>;

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_ENDPOINT_H_
