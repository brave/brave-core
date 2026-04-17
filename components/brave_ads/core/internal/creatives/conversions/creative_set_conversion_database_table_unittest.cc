/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/test/creative_set_conversion_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeSetConversionDatabaseTableTest : public test::TestBase {
 protected:
  database::table::CreativeSetConversions database_table_;
};

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest, EmptySave) {
  // Act
  database::SaveCreativeSetConversions({});

  // Assert
  base::test::TestFuture<bool, CreativeSetConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CreativeSetConversionList&>());
  const auto [success, creative_set_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(creative_set_conversions, ::testing::IsEmpty());
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       SaveCreativeSetConversions) {
  // Arrange
  const CreativeSetConversionInfo creative_set_conversion_1 =
      test::BuildCreativeSetConversion(
          test::kCreativeSetId, /*url_pattern=*/"https://www.brave.com/*",
          /*observation_window=*/base::Days(3));

  const CreativeSetConversionInfo creative_set_conversion_2 =
      test::BuildCreativeSetConversion(
          test::kAnotherCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));

  // Act
  database::SaveCreativeSetConversions(
      {creative_set_conversion_1, creative_set_conversion_2});

  // Assert
  base::test::TestFuture<bool, CreativeSetConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CreativeSetConversionList&>());
  const auto [success, creative_set_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(creative_set_conversions,
              ::testing::ElementsAre(creative_set_conversion_1,
                                     creative_set_conversion_2));
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       DoNotSaveDuplicateConversion) {
  // Arrange
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          test::kCreativeSetId, /*url_pattern=*/"https://www.brave.com/*",
          /*observation_window=*/base::Days(3));

  database::SaveCreativeSetConversions({creative_set_conversion});

  // Act
  database::SaveCreativeSetConversions({creative_set_conversion});

  // Assert
  base::test::TestFuture<bool, CreativeSetConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CreativeSetConversionList&>());
  const auto [success, creative_set_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(creative_set_conversions,
              ::testing::ElementsAre(creative_set_conversion));
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       PurgeExpiredConversions) {
  // Arrange
  const CreativeSetConversionInfo creative_set_conversion_1 =
      test::BuildCreativeSetConversion(
          test::kCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/*",
          /*observation_window=*/base::Days(7));

  const CreativeSetConversionInfo creative_set_conversion_2 =
      test::BuildCreativeSetConversion(
          test::kAnotherCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(3));  // Should be purged

  database::SaveCreativeSetConversions(
      {creative_set_conversion_1, creative_set_conversion_2});

  AdvanceClockBy(base::Days(4));

  // Act
  database::PurgeExpiredCreativeSetConversions();

  // Assert
  base::test::TestFuture<bool, CreativeSetConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CreativeSetConversionList&>());
  const auto [success, creative_set_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(creative_set_conversions,
              ::testing::ElementsAre(creative_set_conversion_1));
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       SaveConversionWithMatchingCreativeSetIdAndType) {
  // Arrange
  const CreativeSetConversionInfo creative_set_conversion_1 =
      test::BuildCreativeSetConversion(
          test::kCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/1",
          /*observation_window=*/base::Days(3));

  database::SaveCreativeSetConversions({creative_set_conversion_1});

  const CreativeSetConversionInfo creative_set_conversion_2 =
      test::BuildCreativeSetConversion(
          test::kCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/2",
          /*observation_window=*/base::Days(30));

  // Act
  database::SaveCreativeSetConversions({creative_set_conversion_2});

  // Assert
  base::test::TestFuture<bool, CreativeSetConversionList> test_future;
  database_table_.GetUnexpired(
      test_future.GetCallback<bool, const CreativeSetConversionList&>());
  const auto [success, creative_set_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(creative_set_conversions,
              ::testing::ElementsAre(creative_set_conversion_2));
}

}  // namespace brave_ads
