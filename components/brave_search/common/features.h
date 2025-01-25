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

}  // namespace features
}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_FEATURES_H_
