/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/origin.h"

namespace {

// A cross-origin request initiated by a .onion page must not disclose the
// address it came from, so its Origin header is nullified.
bool IsCrossOriginOnionRequest(const network::ResourceRequest& request) {
  return base::EndsWith(request.request_initiator->host(), ".onion",
                        base::CompareCase::INSENSITIVE_ASCII) &&
         !request.request_initiator->IsSameOriginWith(
             url::Origin::Create(request.url));
}

}  // namespace

#include <services/network/cors/cors_url_loader.cc>
