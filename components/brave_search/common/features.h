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

constexpr char kBraveSearchDefaultAPIDailyLimitName[] =
    "BraveSearchDefaultAPIDailyLimit";
constexpr char kBraveSearchDefaultAPITotalLimitName[] =
    "BraveSearchDefaultAPITotalLimit";

extern const base::Feature kBraveSearchDefaultAPIFeature;
extern const base::FeatureParam<int> kBraveSearchDefaultAPIDailyLimit;
extern const base::FeatureParam<int> kBraveSearchDefaultAPITotalLimit;

}  // namespace features
}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_FEATURES_H_
