/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeSetConversionDatabaseTableTest : public UnitTestBase {
 protected:
  database::table::CreativeSetConversions database_table_;
};

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest, EmptySave) {
  // Act
  database::SaveCreativeSetConversions({});

  // Assert
  base::MockCallback<database::table::GetConversionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*creative_set_conversions=*/::testing::IsEmpty()));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       SaveCreativeSetConversions) {
  // Arrange
  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion_1 =
      test::BuildVerifiableCreativeSetConversion(
          kCreativeSetId, /*url_pattern=*/"https://www.brave.com/*",
          /*observation_window=*/base::Days(3),
          kVerifiableConversionAdvertiserPublicKey);
  creative_set_conversions.push_back(creative_set_conversion_1);

  const CreativeSetConversionInfo creative_set_conversion_2 =
      test::BuildCreativeSetConversion(
          /*creative_set_id=*/"4e83a23c-1194-40f8-8fdc-2f38d7ed75c8",
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion_2);

  // Act
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Assert
  base::MockCallback<database::table::GetConversionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, creative_set_conversions));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       DoNotSaveDuplicateConversion) {
  // Arrange
  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildVerifiableCreativeSetConversion(
          kCreativeSetId, /*url_pattern=*/"https://www.brave.com/*",
          /*observation_window=*/base::Days(3),
          kVerifiableConversionAdvertiserPublicKey);
  creative_set_conversions.push_back(creative_set_conversion);

  database::SaveCreativeSetConversions(creative_set_conversions);

  // Act
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Assert
  base::MockCallback<database::table::GetConversionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, creative_set_conversions));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       PurgeExpiredConversions) {
  // Arrange
  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion_1 =
      test::BuildCreativeSetConversion(
          kCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/*",
          /*observation_window=*/base::Days(7));
  creative_set_conversions.push_back(creative_set_conversion_1);

  const CreativeSetConversionInfo creative_set_conversion_2 =
      test::BuildCreativeSetConversion(
          /*creative_set_id=*/"4e83a23c-1194-40f8-8fdc-2f38d7ed75c8",
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(3));  // Should be purged
  creative_set_conversions.push_back(creative_set_conversion_2);

  database::SaveCreativeSetConversions(creative_set_conversions);

  AdvanceClockBy(base::Days(4));

  // Act
  database::PurgeExpiredCreativeSetConversions();

  // Assert
  base::MockCallback<database::table::GetConversionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, CreativeSetConversionList{
                                                  creative_set_conversion_1}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       SaveConversionWithMatchingCreativeSetIdAndType) {
  // Arrange
  CreativeSetConversionList creative_set_conversions_1;

  const CreativeSetConversionInfo creative_set_conversion_1 =
      test::BuildVerifiableCreativeSetConversion(
          kCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/1",
          /*observation_window=*/base::Days(3),
          kVerifiableConversionAdvertiserPublicKey);
  creative_set_conversions_1.push_back(creative_set_conversion_1);

  database::SaveCreativeSetConversions(creative_set_conversions_1);

  CreativeSetConversionList creative_set_conversions_2;

  const CreativeSetConversionInfo creative_set_conversion_2 =
      test::BuildVerifiableCreativeSetConversion(
          kCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/2",
          /*observation_window=*/base::Days(30),
          kVerifiableConversionAdvertiserPublicKey);
  creative_set_conversions_2.push_back(creative_set_conversion_2);

  // Act
  database::SaveCreativeSetConversions(creative_set_conversions_2);

  // Assert
  base::MockCallback<database::table::GetConversionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, CreativeSetConversionList{
                                                  creative_set_conversion_2}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("creative_set_conversions", database_table_.GetTableName());
}

}  // namespace brave_ads
