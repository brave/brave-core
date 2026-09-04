/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INTERNALS_ADS_INTERNALS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INTERNALS_ADS_INTERNALS_INFO_H_

#include <cstddef>
#include <string>

#include "base/containers/flat_set.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"

namespace brave_ads {

// Accumulates results across `BuildAdsInternals`' sequential fetch chain in
// ads_internals_util.cc, passed by value/move between callbacks rather than
// as ever-growing positional parameters.
struct AdsInternalsInfo final {
  CreativeSetConversionList creative_set_conversions;
  // Which creative set IDs belong to each ad format, used to attach an "Ad
  // Type" to each creative set conversion above (which, unlike an ad event,
  // isn't itself tied to a specific ad format). There's no local database
  // table for search result ads to look up, so that format is inferred by
  // elimination: present in neither set below.
  base::flat_set<std::string> notification_ad_creative_set_ids;
  base::flat_set<std::string> new_tab_page_ad_creative_set_ids;
  AdEventList ad_events;
  base::ListValue confirmation_queue;
  base::ListValue payment_tokens;
  size_t active_notification_ad_count = 0;
  size_t active_new_tab_page_ad_count = 0;
  base::ListValue active_notification_ad_campaigns;
  base::ListValue active_new_tab_page_ad_campaigns;
  base::ListValue condition_matchers;
  base::ListValue transactions;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INTERNALS_ADS_INTERNALS_INFO_H_
