/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/search_query_metrics_url_util.h"

#include <string_view>

#include "url/gurl.h"

namespace metrics {

namespace {

constexpr std::string_view kStagingUrl =
    "https://anonymous.metrics.bravesoftware.com/v1/metrics/search";
constexpr std::string_view kProductionUrl =
    "https://anonymous.metrics.brave.com/v1/metrics/search";

}  // namespace

GURL GetUrl(bool use_staging) {
  return GURL(use_staging ? kStagingUrl : kProductionUrl);
}

}  // namespace metrics
