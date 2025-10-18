/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/verify_result.h"

#include "brave/components/brave_account/endpoints/host.h"
#include "net/http/http_request_headers.h"

namespace brave_account::endpoints {

GURL VerifyResult::URL() {
  return Host().Resolve("/v2/verify/result");
}

std::string_view VerifyResult::Method() {
  return net::HttpRequestHeaders::kPostMethod;
}

}  // namespace brave_account::endpoints
