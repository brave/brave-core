/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/test/order_extraction_patterns_ad_event_test_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"

namespace brave_ads::test {

AdEventInfo BuildAdEventForCampaignId(const std::string& campaign_id,
                                      base::Time created_at) {
  AdEventInfo ad_event;
  ad_event.campaign_id = campaign_id;
  ad_event.created_at = created_at;
  return ad_event;
}

}  // namespace brave_ads::test
