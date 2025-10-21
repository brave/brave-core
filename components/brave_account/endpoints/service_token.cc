/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/service_token.h"

#include "brave/components/brave_account/endpoints/host.h"
#include "net/http/http_request_headers.h"

namespace brave_account::endpoints {

GURL ServiceToken::URL() {
  return Host().Resolve("/v2/auth/service_token");
}

std::string_view ServiceToken::Method() {
  return net::HttpRequestHeaders::kPostMethod;
}

}  // namespace brave_account::endpoints
