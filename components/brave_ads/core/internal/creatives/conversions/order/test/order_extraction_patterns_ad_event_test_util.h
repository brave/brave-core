/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_TEST_ORDER_EXTRACTION_PATTERNS_AD_EVENT_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_TEST_ORDER_EXTRACTION_PATTERNS_AD_EVENT_TEST_UTIL_H_

#include <string>

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct AdEventInfo;

namespace test {

// Lighter-weight than `test::BuildAdEvent`, since order-extraction matching
// only cares about the campaign ID and timing, not ad type/confirmation type.
AdEventInfo BuildAdEventForCampaignId(const std::string& campaign_id,
                                      base::Time created_at);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_TEST_ORDER_EXTRACTION_PATTERNS_AD_EVENT_TEST_UTIL_H_
