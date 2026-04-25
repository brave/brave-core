/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/mock_api_request_helper.h"

#include <utility>

#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace api_request_helper {

MockAPIRequestHelper::MockAPIRequestHelper(
    net::NetworkTrafficAnnotationTag network_traffic_annotation_tag,
    scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory)
    : APIRequestHelper(std::move(network_traffic_annotation_tag),
                       std::move(shared_url_loader_factory)) {}

MockAPIRequestHelper::~MockAPIRequestHelper() = default;

}  // namespace api_request_helper
