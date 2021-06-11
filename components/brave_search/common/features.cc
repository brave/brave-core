// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/common/features.h"

#include "base/feature_list.h"

namespace brave_search {
namespace features {

const base::Feature kBraveSearchDefaultAPIFeature{
    "BraveSearchDefaultAPI", base::FEATURE_ENABLED_BY_DEFAULT};

const base::FeatureParam<int> kBraveSearchDefaultAPIDailyLimit{
    &kBraveSearchDefaultAPIFeature, kBraveSearchDefaultAPIDailyLimitName, 3};

const base::FeatureParam<int> kBraveSearchDefaultAPITotalLimit{
    &kBraveSearchDefaultAPIFeature, kBraveSearchDefaultAPITotalLimitName, 10};

}  // namespace features
}  // namespace brave_search
