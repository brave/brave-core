/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_table.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/test/campaign_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/test/ad_event_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCampaignConversionDatabaseTableTest : public test::TestBase {
 protected:
  database::table::CampaignConversions database_table_;
};

TEST_F(BraveAdsCampaignConversionDatabaseTableTest, EmptySave) {
  // Act
  database::SaveCampaignConversions({});

  // Assert
  base::test::TestFuture<bool, CampaignConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CampaignConversionList&>());
  const auto [success, campaign_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(campaign_conversions, ::testing::IsEmpty());
}

TEST_F(BraveAdsCampaignConversionDatabaseTableTest, SaveCampaignConversions) {
  // Arrange
  const CampaignConversionInfo campaign_conversion_1 =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://www.brave.com/*",
                                    /*observation_window=*/base::Days(3));

  const CampaignConversionInfo campaign_conversion_2 =
      test::BuildCampaignConversion(
          test::kAnotherCampaignId,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));

  // Act
  database::SaveCampaignConversions(
      {campaign_conversion_1, campaign_conversion_2});

  // Assert
  base::test::TestFuture<bool, CampaignConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CampaignConversionList&>());
  const auto [success, campaign_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(
      campaign_conversions,
      ::testing::ElementsAre(campaign_conversion_1, campaign_conversion_2));
}

TEST_F(BraveAdsCampaignConversionDatabaseTableTest,
       DoNotSaveDuplicateConversion) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://www.brave.com/*",
                                    /*observation_window=*/base::Days(3));

  database::SaveCampaignConversions({campaign_conversion});

  // Act
  database::SaveCampaignConversions({campaign_conversion});

  // Assert
  base::test::TestFuture<bool, CampaignConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CampaignConversionList&>());
  const auto [success, campaign_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(campaign_conversions,
              ::testing::ElementsAre(campaign_conversion));
}

TEST_F(BraveAdsCampaignConversionDatabaseTableTest, PurgeExpiredConversions) {
  // Arrange
  const CampaignConversionInfo campaign_conversion_1 =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://www.brave.com/*",
                                    /*observation_window=*/base::Days(7));

  const CampaignConversionInfo campaign_conversion_2 =
      test::BuildCampaignConversion(
          test::kAnotherCampaignId,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(3));  // Should be purged

  database::SaveCampaignConversions(
      {campaign_conversion_1, campaign_conversion_2});

  AdvanceClockBy(base::Days(4));

  // Act
  database::PurgeExpiredCampaignConversions();

  // Assert
  base::test::TestFuture<bool, CampaignConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CampaignConversionList&>());
  const auto [success, campaign_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(campaign_conversions,
              ::testing::ElementsAre(campaign_conversion_1));
}

TEST_F(BraveAdsCampaignConversionDatabaseTableTest,
       SaveConversionWithMatchingCampaignIdAndType) {
  // Arrange
  const CampaignConversionInfo campaign_conversion_1 =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://www.brave.com/1",
                                    /*observation_window=*/base::Days(3));

  database::SaveCampaignConversions({campaign_conversion_1});

  const CampaignConversionInfo campaign_conversion_2 =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://www.brave.com/2",
                                    /*observation_window=*/base::Days(30));

  // Act
  database::SaveCampaignConversions({campaign_conversion_2});

  // Assert
  base::test::TestFuture<bool, CampaignConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CampaignConversionList&>());
  const auto [success, campaign_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(campaign_conversions,
              ::testing::ElementsAre(campaign_conversion_2));
}

TEST_F(BraveAdsCampaignConversionDatabaseTableTest,
       GetActiveDoesNotReturnDuplicatesWhenMultipleAdEventsMatch) {
  // Arrange
  test::BuildAndSaveCampaignConversion(
      test::kCampaignId, /*url_pattern=*/"https://www.brave.com/*",
      /*observation_window=*/base::Days(3));

  const AdInfo ad =
      test::BuildAd(mojom::AdType::kNotificationAd, /*use_random_uuids=*/false);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kViewedImpression,
                       /*count=*/3);

  // Act
  base::test::TestFuture<bool, CampaignConversionList> test_future;
  database_table_.GetActive(
      test_future.GetCallback<bool, const CampaignConversionList&>());
  const auto [success, campaign_conversions] = test_future.Take();

  // Assert
  EXPECT_TRUE(success);
  EXPECT_THAT(campaign_conversions, ::testing::SizeIs(1));
}

}  // namespace brave_ads
