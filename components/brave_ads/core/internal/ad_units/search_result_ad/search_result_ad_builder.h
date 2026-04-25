/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_BUILDER_H_

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct SearchResultAdInfo;

SearchResultAdInfo FromMojomBuildSearchResultAd(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_BUILDER_H_
