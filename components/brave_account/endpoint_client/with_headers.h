/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_WITH_HEADERS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_WITH_HEADERS_H_

#include "brave/components/brave_account/endpoint_client/is_request.h"
#include "net/http/http_request_headers.h"

namespace brave_account::endpoint_client {

// Wrapper that extends an IsRequest type T
// with an additional net::HttpRequestHeaders member.
template <detail::IsRequest T>
struct WithHeaders : T {
  net::HttpRequestHeaders headers;
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_WITH_HEADERS_H_
