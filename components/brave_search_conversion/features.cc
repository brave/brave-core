/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search_conversion/features.h"

namespace brave_search_conversion::features {

// Brave search promotion match located at last low in omnibox popup.
// This type seems more like ads banner.
BASE_FEATURE(kOmniboxBanner,
             "BraveSearchOmniboxBanner",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<std::string> kBannerType{&kOmniboxBanner,
                                                  kBannerTypeParamName, ""};

BASE_FEATURE(kOmniboxDDGBanner,
             "BraveSearchOmniboxDDGBanner",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kOmniboxPromotionButton,
             "BraveSearchPromotionOmniboxButton",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Brave search promotion at NTP.
BASE_FEATURE(kNTP, "BraveSearchNTP", base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_search_conversion::features
