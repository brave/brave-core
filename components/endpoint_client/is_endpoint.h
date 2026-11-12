/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_ENDPOINT_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_ENDPOINT_H_

#include <concepts>

#include "brave/components/endpoint_client/is_request.h"
#include "brave/components/endpoint_client/is_response.h"
#include "url/gurl.h"

namespace endpoint_client {

namespace internal {

// An endpoint defines Request and Response type aliases and a static URL().
template <typename Endpoint>
concept IsEndpointLike = requires {
  typename Endpoint::Request;
  typename Endpoint::Response;
  { Endpoint::URL() } -> std::same_as<GURL>;
};

// A JSON endpoint has a body-less or JSON request and a JSON response.
template <typename Endpoint>
concept IsJSONEndpoint = IsEndpointLike<Endpoint> &&
                         (IsBodylessRequest<typename Endpoint::Request> ||
                          IsJSONRequest<typename Endpoint::Request>) &&
                         IsJSONResponse<typename Endpoint::Response>;

// A Protobuf endpoint has a body-less or Protobuf request and a Protobuf
// response.
template <typename Endpoint>
concept IsProtobufEndpoint = IsEndpointLike<Endpoint> &&
                             (IsBodylessRequest<typename Endpoint::Request> ||
                              IsProtobufRequest<typename Endpoint::Request>) &&
                             IsProtobufResponse<typename Endpoint::Response>;

}  // namespace internal

// An endpoint is either a JSON endpoint or a Protobuf endpoint.
template <typename Endpoint>
concept IsEndpoint = internal::IsJSONEndpoint<Endpoint> ||
                     internal::IsProtobufEndpoint<Endpoint>;

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_ENDPOINT_H_
