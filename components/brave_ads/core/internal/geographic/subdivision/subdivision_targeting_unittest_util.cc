/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting_unittest_util.h"

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/get_subdivision_url_request_builder_constants.h"

namespace brave_ads::geographic {

namespace {

std::string BuildUrlResponseBody(const std::string& country,
                                 const std::string& region) {
  return base::ReplaceStringPlaceholders(R"({"country":"$1", "region":"$2"})",
                                         {country, region}, nullptr);
}

}  // namespace

URLResponsePair BuildSubdivisionTargetingUrlResponseBody(
    const net::HttpStatusCode status_code,
    const std::string& country,
    const std::string& region) {
  return {status_code, BuildUrlResponseBody(country, region)};
}

URLResponsePair BuildSubdivisionTargetingUrlResponseBody(
    const net::HttpStatusCode status_code,
    const std::string& response_body) {
  return {status_code, response_body};
}

URLResponseMap BuildValidSubdivisionTargetingUrlResponses(
    const net::HttpStatusCode status_code,
    const std::string& country,
    const std::string& region) {
  return {{kSubdivisionTargetingUrlPath,
           {{{BuildSubdivisionTargetingUrlResponseBody(status_code, country,
                                                       region)}}}}};
}

}  // namespace brave_ads::geographic
