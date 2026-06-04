// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

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
// If true, seeds navigation history and loads the root URL first, then
// loads the actual target URL after a randomized delay following
// the root page's load completion.
extern const base::FeatureParam<bool> kBackupResultsLoadAfterRestore;
// Randomized delay range before loading the target URL after root page restore.
// Used when low_latency_required is false.
extern const base::FeatureParam<base::TimeDelta>
    kBackupResultsLoadAfterRestoreDelayMin;
extern const base::FeatureParam<base::TimeDelta>
    kBackupResultsLoadAfterRestoreDelayMax;
// Randomized delay range used when low_latency_required is true.
extern const base::FeatureParam<base::TimeDelta>
    kBackupResultsLoadAfterRestoreLowDelayMin;
extern const base::FeatureParam<base::TimeDelta>
    kBackupResultsLoadAfterRestoreLowDelayMax;
// If true, allows fetch-style requests made from the page through the URL
// loader throttle.
extern const base::FeatureParam<bool> kBackupResultsAllowFetches;
// If true, allows cosmetic asset requests (images, fonts, icons, etc.)
// through the URL loader throttle.
extern const base::FeatureParam<bool> kBackupResultsAllowCosmeticAssets;
// If true, allows requests that don't match any known category through
// the URL loader throttle.
extern const base::FeatureParam<bool> kBackupResultsAllowUnclassifiedRequests;

}  // namespace features
}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_FEATURES_H_
