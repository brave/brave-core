// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_search {
namespace features {

inline constexpr char kBraveSearchDefaultAPIDailyLimitName[] =
    "BraveSearchDefaultAPIDailyLimit";
inline constexpr char kBraveSearchDefaultAPITotalLimitName[] =
    "BraveSearchDefaultAPITotalLimit";

BASE_DECLARE_FEATURE(kBraveSearchDefaultAPIFeature);
extern const base::FeatureParam<int> kBraveSearchDefaultAPIDailyLimit;
extern const base::FeatureParam<int> kBraveSearchDefaultAPITotalLimit;

// If enabled, the initial search page and subsequent redirected pages will
// all be rendered, instead of just the initial page.
BASE_DECLARE_FEATURE(kBackupResultsFullRender);
// The amount of requests required to reach the actual search engine
// results page. This count includes the original request and the subsequent
// redirects.
extern const base::FeatureParam<int> kBackupResultsFullRenderMaxRequests;

bool IsBackupResultsFullRenderEnabled();

// Provides request configuration for the backup results service.
BASE_DECLARE_FEATURE(kBackupResults);
// JSON-serialized headers object {"Header-Name": "value"} to include in
// SimpleURLLoader requests.
extern const base::FeatureParam<std::string> kBackupResultsHeaders;
// UA string override for WebContents and User-Agent header in SimpleURLLoader
// requests.
extern const base::FeatureParam<std::string> kBackupResultsUAOverride;
// Base64-encoded pickled blink::UserAgentMetadata. Only used if
// kBackupResultsUAOverride is also provided.
extern const base::FeatureParam<std::string> kBackupResultsUAMetadata;
// Maximum number of backup results fetches allowed per day. -1 means no limit.
extern const base::FeatureParam<int> kBackupResultsMaxDailyRequests;

}  // namespace features
}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_FEATURES_H_
