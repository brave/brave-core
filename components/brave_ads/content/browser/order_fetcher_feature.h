/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCHER_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCHER_FEATURE_H_

#include <string>

#include "base/feature.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kOrderFetcherFeature);

// Maximum allowed length for any extracted order field.
inline constexpr base::FeatureParam<int> kMaxExtractedOrderFieldLength{
    &kOrderFetcherFeature, "max_extracted_order_field_length", 64};

// Regular expression pattern used to validate extracted order values.
inline constexpr base::FeatureParam<std::string> kOrderValueValidationPattern{
    &kOrderFetcherFeature, "order_value_validation_pattern",
    R"RE2(^\d[\d.,\p{Zs}'’]{0,31}$)RE2"};

// Timeout for fetching order information from the page.
inline constexpr base::FeatureParam<base::TimeDelta> kFetchOrderTimeout{
    &kOrderFetcherFeature, "fetch_order_timeout", base::Milliseconds(300)};

// Delay before the first retry when an extraction attempt finds nothing,
// e.g. a single-page app that had not yet rendered the order on the page.
inline constexpr base::FeatureParam<base::TimeDelta>
    kOrderPatternsRetryInitialDelay{&kOrderFetcherFeature,
                                    "order_patterns_retry_initial_delay",
                                    base::Milliseconds(50)};

// Maximum number of retries, each doubling the previous delay, so a slow-
// rendering single-page app still has several seconds to settle.
inline constexpr base::FeatureParam<size_t> kOrderPatternsMaxRetries{
    &kOrderFetcherFeature, "order_patterns_max_retries", 6};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCHER_FEATURE_H_
