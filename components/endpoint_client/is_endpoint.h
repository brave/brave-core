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

// An endpoint defines a static URL() and Request and Response type aliases that
// satisfy IsRequest and IsResponse.
template <typename Endpoint>
concept IsEndpoint =
    requires {
      { Endpoint::URL() } -> std::same_as<GURL>;
      typename Endpoint::Request;
      typename Endpoint::Response;
    } && internal::IsRequest<typename Endpoint::Request> &&
    internal::IsResponse<typename Endpoint::Response>;

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_ENDPOINT_H_
