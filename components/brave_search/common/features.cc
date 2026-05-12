// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/common/features.h"

#include <string>

#include "base/feature_list.h"

namespace brave_search::features {

BASE_FEATURE(kBraveSearchDefaultAPIFeature,
             "BraveSearchDefaultAPI",
             base::FEATURE_ENABLED_BY_DEFAULT);

const base::FeatureParam<int> kBraveSearchDefaultAPIDailyLimit{
    &kBraveSearchDefaultAPIFeature, kBraveSearchDefaultAPIDailyLimitName, 3};

const base::FeatureParam<int> kBraveSearchDefaultAPITotalLimit{
    &kBraveSearchDefaultAPIFeature, kBraveSearchDefaultAPITotalLimitName, 10};

BASE_FEATURE(kBackupResultsFullRender,
             "BraveSearchBackupResultsFullRender",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<int> kBackupResultsFullRenderMaxRequests{
    &kBackupResultsFullRender, "MaxRequests", 2};

bool IsBackupResultsFullRenderEnabled() {
  return base::FeatureList::IsEnabled(kBackupResultsFullRender);
}

BASE_FEATURE(kBackupResults,
             "BraveSearchBackupResults",
             base::FEATURE_ENABLED_BY_DEFAULT);

const base::FeatureParam<std::string> kBackupResultsHeaders{&kBackupResults,
                                                            "headers", ""};

const base::FeatureParam<std::string> kBackupResultsUAOverride{
    &kBackupResults, "ua_override", ""};

const base::FeatureParam<std::string> kBackupResultsUAMetadata{
    &kBackupResults, "ua_metadata", ""};

const base::FeatureParam<int> kBackupResultsMaxDailyRequests{
    &kBackupResults, "max_daily_requests", -1};

}  // namespace brave_search::features
