/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/base/url_util.h"
#include "services/network/public/cpp/resource_request.h"

namespace {

bool IsCrossOriginOnionRequest(const network::ResourceRequest& request) {
  return net::IsCrossOriginOnionRequest(*request.request_initiator,
                                        request.url);
}

}  // namespace

#include <services/network/cors/cors_url_loader.cc>
