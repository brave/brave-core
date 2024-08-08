/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"

#include <vector>

#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"
#include "brave/components/brave_ads/core/public/history/ad_history_feature.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

AdHistoryList GetHighestRankedPlacementAdHistory(
    const std::vector<AdHistoryList>& ad_histories) {
  AdHistoryList highest_ranked_placement_ad_history;

  for (const auto& ad_history : ad_histories) {
    if (!ad_history.empty()) {
      highest_ranked_placement_ad_history.push_back(ad_history.back());
    }
  }

  return highest_ranked_placement_ad_history;
}

}  // namespace

class BraveAdsAdHistoryDatabaseTableTest : public test::TestBase {
 protected:
  database::table::AdHistory database_table_;
};

TEST_F(BraveAdsAdHistoryDatabaseTableTest, Save) {
  // Arrange
  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/true);

  // Act
  SaveAdHistory(ad_history);

  // Assert
  base::MockCallback<GetAdHistoryCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Optional(
                            ::testing::UnorderedElementsAreArray(ad_history))));
  database_table_.GetForDateRange(/*from_time=*/test::DistantPast(),
                                  /*to_time=*/test::DistantFuture(),
                                  callback.Get());
}

TEST_F(BraveAdsAdHistoryDatabaseTableTest, SaveEmpty) {
  // Act
  SaveAdHistory({});

  // Assert
  base::MockCallback<GetAdHistoryCallback> callback;
  EXPECT_CALL(callback,
              Run(/*ad_history=*/::testing::Optional(::testing::IsEmpty())));
  database_table_.GetForDateRange(/*from_time=*/test::DistantPast(),
                                  /*to_time=*/test::DistantFuture(),
                                  callback.Get());
}

TEST_F(BraveAdsAdHistoryDatabaseTableTest, SaveInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked,
       ConfirmationType::kViewedImpression},
      /*should_generate_random_uuids=*/true);

  // Act
  SaveAdHistory(ad_history);

  // Assert
  base::MockCallback<GetAdHistoryCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Optional(
                            ::testing::UnorderedElementsAreArray(ad_history))));
  database_table_.GetForDateRange(/*from_time=*/test::DistantPast(),
                                  /*to_time=*/test::DistantFuture(),
                                  callback.Get());
}

TEST_F(BraveAdsAdHistoryDatabaseTableTest, GetForDateRange) {
  // Arrange
  const AdHistoryList ad_history_1 = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/true);
  SaveAdHistory(ad_history_1);

  AdvanceClockBy(base::Days(2));

  const base::Time from_time = test::Now() - base::Days(1);

  const AdHistoryList ad_history_2 = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/true);
  SaveAdHistory(ad_history_2);

  // Act & Assert
  base::MockCallback<GetAdHistoryCallback> callback;
  EXPECT_CALL(callback,
              Run(::testing::Optional(
                  ::testing::UnorderedElementsAreArray(ad_history_2))));
  database_table_.GetForDateRange(from_time, /*to_time=*/test::DistantFuture(),
                                  callback.Get());
}

TEST_F(BraveAdsAdHistoryDatabaseTableTest,
       GetHighestRankedPlacementsForDateRange) {
  // Arrange
  const AdHistoryList ad_history_1 = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/false);
  SaveAdHistory(ad_history_1);

  AdvanceClockBy(base::Days(2));

  const base::Time from_time = test::Now();

  const AdHistoryList ad_history_2 = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/false);
  SaveAdHistory(ad_history_2);

  const AdHistoryList ad_history_3 = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd, {ConfirmationType::kViewedImpression},
      /*should_generate_random_uuids=*/false);
  SaveAdHistory(ad_history_3);

  const AdHistoryList ad_history_4 = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kDismissed},
      /*should_generate_random_uuids=*/false);
  SaveAdHistory(ad_history_4);

  // Act & Assert
  const AdHistoryList expected_ad_history = GetHighestRankedPlacementAdHistory(
      {ad_history_2, ad_history_3, ad_history_4});
  ASSERT_THAT(expected_ad_history, ::testing::SizeIs(3));

  base::MockCallback<GetAdHistoryCallback> callback;
  EXPECT_CALL(callback,
              Run(::testing::Optional(
                  ::testing::UnorderedElementsAreArray(expected_ad_history))));
  database_table_.GetHighestRankedPlacementsForDateRange(
      from_time, /*to_time=*/test::DistantFuture(), callback.Get());
}

TEST_F(BraveAdsAdHistoryDatabaseTableTest, GetForCreativeInstanceId) {
  // Arrange
  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/false);
  SaveAdHistory(ad_history);

  // Act & Assert
  base::MockCallback<GetAdHistoryCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Optional(
                            ::testing::UnorderedElementsAreArray(ad_history))));
  database_table_.GetForCreativeInstanceId(test::kCreativeInstanceId,
                                           callback.Get());
}

TEST_F(BraveAdsAdHistoryDatabaseTableTest,
       DoNotGetForMissingCreativeInstanceId) {
  // Arrange
  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/true);
  SaveAdHistory(ad_history);

  // Act & Assert
  base::MockCallback<GetAdHistoryCallback> callback;
  EXPECT_CALL(callback,
              Run(/*ad_history=*/::testing::Optional(::testing::IsEmpty())));
  database_table_.GetForCreativeInstanceId(test::kCreativeInstanceId,
                                           callback.Get());
}

TEST_F(BraveAdsAdHistoryDatabaseTableTest, PurgeExpired) {
  // Arrange
  const AdHistoryList ad_history_1 = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/true);
  SaveAdHistory(ad_history_1);

  AdvanceClockBy(kAdHistoryRetentionPeriod.Get());

  const AdHistoryList ad_history_2 = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/true);
  SaveAdHistory(ad_history_2);

  // Act & Assert
  base::MockCallback<ResultCallback> purge_expired_callback;
  EXPECT_CALL(purge_expired_callback, Run(/*success=*/true));
  database_table_.PurgeExpired(purge_expired_callback.Get());

  base::MockCallback<GetAdHistoryCallback> callback;
  EXPECT_CALL(callback,
              Run(::testing::Optional(
                  ::testing::UnorderedElementsAreArray(ad_history_2))));
  database_table_.GetForDateRange(/*from_time=*/test::DistantPast(),
                                  /*to_time=*/test::DistantFuture(),
                                  callback.Get());
}

TEST_F(BraveAdsAdHistoryDatabaseTableTest, DoNotPurgeOnTheCuspOfExpiration) {
  // Arrange
  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      AdType::kNotificationAd,
      {ConfirmationType::kViewedImpression, ConfirmationType::kClicked},
      /*should_generate_random_uuids=*/true);
  SaveAdHistory(ad_history);

  AdvanceClockBy(kAdHistoryRetentionPeriod.Get() - base::Milliseconds(1));

  // Act & Assert
  base::MockCallback<ResultCallback> purge_expired_callback;
  EXPECT_CALL(purge_expired_callback, Run(/*success=*/true));
  database_table_.PurgeExpired(purge_expired_callback.Get());

  base::MockCallback<GetAdHistoryCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Optional(
                            ::testing::UnorderedElementsAreArray(ad_history))));
  database_table_.GetForDateRange(/*from_time=*/test::DistantPast(),
                                  /*to_time=*/test::DistantFuture(),
                                  callback.Get());
}

TEST_F(BraveAdsAdHistoryDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("ad_history", database_table_.GetTableName());
}

}  // namespace brave_ads
