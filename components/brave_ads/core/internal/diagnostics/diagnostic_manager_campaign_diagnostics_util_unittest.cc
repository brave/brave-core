/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager_campaign_diagnostics_util.h"

#include "base/functional/bind.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/test/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/test/ad_event_builder_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

void RecordServedImpression(const CreativeNewTabPageAdInfo& creative_ad,
                            base::Time created_at) {
  base::test::TestFuture<bool> test_future;
  RecordAdEvent(test::BuildAdEvent(creative_ad, mojom::AdType::kNewTabPageAd,
                                   mojom::ConfirmationType::kServedImpression,
                                   created_at, /*use_random_uuids=*/true),
                test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());
}

const base::DictValue& GetOnlyCreativeSet(const base::ListValue& campaigns) {
  const base::DictValue& campaign = campaigns.front().GetDict();
  const base::ListValue* const creative_sets =
      campaign.FindList("Creative Sets");
  CHECK(creative_sets);
  return creative_sets->front().GetDict();
}

}  // namespace

class BraveAdsDiagnosticManagerCampaignDiagnosticsUtilTest
    : public test::TestBase {};

// Regression test for `CountServedImpressions` being restructured to group
// served impression timestamps by campaign/creative set ID in a single pass
// over `ad_events`, instead of rescanning the full event history once per
// frequency-cap window.
TEST_F(BraveAdsDiagnosticManagerCampaignDiagnosticsUtilTest,
       CountsServedImpressionsPerFrequencyCapWindow) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("Wednesday, 15 July 2026"));

  CreativeNewTabPageAdInfo creative_ad = test::BuildCreativeNewTabPageAd(
      CreativeNewTabPageAdWallpaperType::kImage, /*use_random_uuids=*/false);
  test::SaveCreativeNewTabPageAds({creative_ad});

  // Within the last day, week, and month.
  RecordServedImpression(creative_ad, test::Now() - base::Hours(12));
  // Outside the last day, but within the last week and month.
  RecordServedImpression(creative_ad, test::Now() - base::Days(3));
  // Outside the last day and week, but within the last month.
  RecordServedImpression(creative_ad, test::Now() - base::Days(10));
  // Outside the last day, week, and month; only counted towards the total.
  RecordServedImpression(creative_ad, test::Now() - base::Days(40));

  // Act
  base::test::TestFuture<bool, size_t, base::ListValue> test_future;
  DiagnosticManager::GetNewTabPageAdCampaigns(test_future.GetCallback());
  const auto [success, active_ad_count, campaigns] = test_future.Take();
  ASSERT_TRUE(success);
  ASSERT_EQ(1u, active_ad_count);

  const base::DictValue& campaign_dict = campaigns.front().GetDict();
  const base::DictValue& creative_set_dict = GetOnlyCreativeSet(campaigns);

  // Assert
  EXPECT_EQ(1, creative_set_dict.FindInt("Per Day Served"));
  EXPECT_EQ(2, creative_set_dict.FindInt("Per Week Served"));
  EXPECT_EQ(3, creative_set_dict.FindInt("Per Month Served"));
  EXPECT_EQ(4, creative_set_dict.FindInt("Total Max Served"));
  EXPECT_EQ(1, campaign_dict.FindInt("Daily Cap Served"));
}

TEST_F(BraveAdsDiagnosticManagerCampaignDiagnosticsUtilTest,
       ZeroServedImpressionsForCampaignWithNoAdEvents) {
  // Arrange
  CreativeNewTabPageAdInfo creative_ad = test::BuildCreativeNewTabPageAd(
      CreativeNewTabPageAdWallpaperType::kImage, /*use_random_uuids=*/false);
  test::SaveCreativeNewTabPageAds({creative_ad});

  // Act
  base::test::TestFuture<bool, size_t, base::ListValue> test_future;
  DiagnosticManager::GetNewTabPageAdCampaigns(test_future.GetCallback());
  const auto [success, active_ad_count, campaigns] = test_future.Take();
  ASSERT_TRUE(success);
  ASSERT_EQ(1u, active_ad_count);

  const base::DictValue& campaign_dict = campaigns.front().GetDict();
  const base::DictValue& creative_set_dict = GetOnlyCreativeSet(campaigns);

  // Assert
  EXPECT_EQ(0, creative_set_dict.FindInt("Per Day Served"));
  EXPECT_EQ(0, creative_set_dict.FindInt("Per Week Served"));
  EXPECT_EQ(0, creative_set_dict.FindInt("Per Month Served"));
  EXPECT_EQ(0, creative_set_dict.FindInt("Total Max Served"));
  EXPECT_EQ(0, campaign_dict.FindInt("Daily Cap Served"));
}

}  // namespace brave_ads
