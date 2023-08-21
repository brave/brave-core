/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_UNITTEST_UTIL_H_

#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"

namespace brave_ads {

mojom::SearchResultAdInfoPtr BuildSearchResultAdForTesting(
    bool should_use_random_uuids);
mojom::SearchResultAdInfoPtr BuildSearchResultAdWithConversionForTesting(
    bool should_use_random_uuids);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_UNITTEST_UTIL_H_
