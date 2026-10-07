/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_ADS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_ADS_UTIL_H_

#include <string>
#include <string_view>

#include "base/containers/flat_set.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/condition_matcher_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

void ParseAndSaveNewTabPageAds(base::DictValue dict, ResultCallback callback);

std::optional<mojom::NewTabPageAdMetricType> ToMojomNewTabPageAdMetricType(
    std::string_view value);

std::string_view ToString(mojom::NewTabPageAdMetricType value);

// Collects the `id` component of every `[virtual]:ad_events|<id>|
// <confirmation_type>` condition matcher pref path in `condition_matchers`,
// for batching into a single `AdEvents::GetVirtualPrefs()` query rather than
// one query per creative.
base::flat_set<std::string> GetAdEventVirtualPrefQueryIds(
    const ConditionMatcherMap& condition_matchers);

// Same as above, aggregated across every creative in `creative_ads`.
base::flat_set<std::string> GetAdEventVirtualPrefQueryIds(
    const CreativeNewTabPageAdList& creative_ads);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_ADS_UTIL_H_
