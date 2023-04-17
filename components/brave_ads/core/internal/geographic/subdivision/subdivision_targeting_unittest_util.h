/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_UNITTEST_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_alias.h"
#include "net/http/http_status_code.h"

namespace brave_ads::geographic {

URLResponsePair BuildSubdivisionTargetingUrlResponseBody(
    net::HttpStatusCode status_code,
    const std::string& country,
    const std::string& region);

URLResponsePair BuildSubdivisionTargetingUrlResponseBody(
    net::HttpStatusCode status_code,
    const std::string& response_body = "");

URLResponseMap BuildValidSubdivisionTargetingUrlResponses(
    net::HttpStatusCode status_code,
    const std::string& country,
    const std::string& region);

}  // namespace brave_ads::geographic

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_UNITTEST_UTIL_H_
