/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_test_constants.h"

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
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*creative_set_conversions=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetUnexpired(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       SaveCreativeSetConversions) {
  // Arrange
  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion_1 =
      test::BuildVerifiableCreativeSetConversion(
          test::kCreativeSetId, /*url_pattern=*/"https://www.brave.com/*",
          /*observation_window=*/base::Days(3),
          test::kVerifiableConversionAdvertiserPublicKeyBase64);
  creative_set_conversions.push_back(creative_set_conversion_1);

  const CreativeSetConversionInfo creative_set_conversion_2 =
      test::BuildCreativeSetConversion(
          test::kAnotherCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion_2);

  // Act
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Assert
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true, creative_set_conversions))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetUnexpired(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       DoNotSaveDuplicateConversion) {
  // Arrange
  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildVerifiableCreativeSetConversion(
          test::kCreativeSetId, /*url_pattern=*/"https://www.brave.com/*",
          /*observation_window=*/base::Days(3),
          test::kVerifiableConversionAdvertiserPublicKeyBase64);
  creative_set_conversions.push_back(creative_set_conversion);

  database::SaveCreativeSetConversions(creative_set_conversions);

  // Act
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Assert
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true, creative_set_conversions))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetUnexpired(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       PurgeExpiredConversions) {
  // Arrange
  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion_1 =
      test::BuildCreativeSetConversion(
          test::kCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/*",
          /*observation_window=*/base::Days(7));
  creative_set_conversions.push_back(creative_set_conversion_1);

  const CreativeSetConversionInfo creative_set_conversion_2 =
      test::BuildCreativeSetConversion(
          test::kAnotherCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(3));  // Should be purged
  creative_set_conversions.push_back(creative_set_conversion_2);

  database::SaveCreativeSetConversions(creative_set_conversions);

  AdvanceClockBy(base::Days(4));

  // Act
  database::PurgeExpiredCreativeSetConversions();

  // Assert
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback,
              Run(/*success=*/true,
                  CreativeSetConversionList{creative_set_conversion_1}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetUnexpired(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest,
       SaveConversionWithMatchingCreativeSetIdAndType) {
  // Arrange
  CreativeSetConversionList creative_set_conversions_1;

  const CreativeSetConversionInfo creative_set_conversion_1 =
      test::BuildVerifiableCreativeSetConversion(
          test::kCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/1",
          /*observation_window=*/base::Days(3),
          test::kVerifiableConversionAdvertiserPublicKeyBase64);
  creative_set_conversions_1.push_back(creative_set_conversion_1);

  database::SaveCreativeSetConversions(creative_set_conversions_1);

  CreativeSetConversionList creative_set_conversions_2;

  const CreativeSetConversionInfo creative_set_conversion_2 =
      test::BuildVerifiableCreativeSetConversion(
          test::kCreativeSetId,
          /*url_pattern=*/"https://www.brave.com/2",
          /*observation_window=*/base::Days(30),
          test::kVerifiableConversionAdvertiserPublicKeyBase64);
  creative_set_conversions_2.push_back(creative_set_conversion_2);

  // Act
  database::SaveCreativeSetConversions(creative_set_conversions_2);

  // Assert
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback,
              Run(/*success=*/true,
                  CreativeSetConversionList{creative_set_conversion_2}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetUnexpired(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeSetConversionDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("creative_set_conversions", database_table_.GetTableName());
}

}  // namespace brave_ads
