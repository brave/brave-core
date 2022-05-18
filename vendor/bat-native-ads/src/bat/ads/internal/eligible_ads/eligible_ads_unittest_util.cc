/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/eligible_ads_unittest_util.h"

#include "bat/ads/ad_type.h"
#include "bat/ads/internal/ad_events/ad_events_database_table_unittest_util.h"
#include "bat/ads/internal/deprecated/client/client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ads {

void ResetEligibleAds(const AdType& type) {
  Client::Get()->ResetAllSeenAdsForType(type);

  Client::Get()->ResetAllSeenAdvertisersForType(type);

  database::table::ad_events::Reset(
      [](const bool success) { ASSERT_TRUE(success); });
}

}  // namespace ads
