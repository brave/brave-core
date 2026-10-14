/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/base/url_util.h"
#include "services/network/public/cpp/resource_request.h"

namespace {

// A .onion page must not disclose its address to other origins. Upstream
// taints a request's origin only after a cross-origin redirect, so a direct
// cross-origin request from a .onion page is tainted too, giving it an
// opaque origin in its Origin header, its CORS preflight and the CORS checks.
bool ShouldTaintOnionOrigin(const network::ResourceRequest& request) {
  return request.request_initiator &&
         net::IsCrossOriginOnionRequest(*request.request_initiator,
                                        request.url);
}

}  // namespace

#include <services/network/cors/cors_url_loader.cc>
