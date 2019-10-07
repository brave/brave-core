/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_simple_url_loader.h"

#include <memory>

#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"

namespace network {

std::unique_ptr<SimpleURLLoader> BraveSimpleURLLoader::Create(
    std::unique_ptr<ResourceRequest> resource_request,
    const net::NetworkTrafficAnnotationTag& annotation_tag) {
  return SimpleURLLoader::Create(std::move(resource_request), annotation_tag);
}

} // namespace network