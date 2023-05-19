/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversions_database_table.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_database_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsConversionsDatabaseTableTest : public UnitTestBase {
 protected:
  Conversions database_table_;
};

TEST_F(BraveAdsConversionsDatabaseTableTest, EmptySave) {
  // Arrange

  // Act
  SaveConversions({});

  // Assert
  database_table_.GetAll(
      base::BindOnce([](const bool success, const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(conversions.empty());
      }));
}

TEST_F(BraveAdsConversionsDatabaseTableTest, SaveConversions) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion_1;
  conversion_1.creative_set_id = kCreativeSetId;
  conversion_1.type = "postview";
  conversion_1.url_pattern = "https://www.brave.com/*";
  conversion_1.observation_window = base::Days(3);
  conversion_1.expire_at = Now() + conversion_1.observation_window;
  conversions.push_back(conversion_1);

  ConversionInfo conversion_2;
  conversion_2.creative_set_id = "4e83a23c-1194-40f8-8fdc-2f38d7ed75c8";
  conversion_2.type = "postclick";
  conversion_2.url_pattern = "https://www.brave.com/signup/*";
  conversion_2.observation_window = base::Days(30);
  conversion_2.expire_at = Now() + conversion_2.observation_window;
  conversions.push_back(conversion_2);

  // Act
  SaveConversions(conversions);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const ConversionList& expected_conversions, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_conversions, conversions));
      },
      conversions));
}

TEST_F(BraveAdsConversionsDatabaseTableTest, DoNotSaveDuplicateConversion) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = kCreativeSetId;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.brave.com/*";
  conversion.observation_window = base::Days(3);
  conversion.expire_at = Now() + conversion.observation_window;
  conversions.push_back(conversion);

  SaveConversions(conversions);

  // Act
  SaveConversions(conversions);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const ConversionList& expected_conversions, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversions, conversions);
      },
      conversions));
}

TEST_F(BraveAdsConversionsDatabaseTableTest, PurgeExpiredConversions) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion_1;
  conversion_1.creative_set_id = kCreativeSetId;
  conversion_1.type = "postview";
  conversion_1.url_pattern = "https://www.brave.com/*";
  conversion_1.observation_window = base::Days(7);
  conversion_1.expire_at = Now() + conversion_1.observation_window;
  conversions.push_back(conversion_1);

  ConversionInfo conversion_2;  // Should be purged
  conversion_2.creative_set_id = "4e83a23c-1194-40f8-8fdc-2f38d7ed75c8";
  conversion_2.type = "postclick";
  conversion_2.url_pattern = "https://www.brave.com/signup/*";
  conversion_2.observation_window = base::Days(3);
  conversion_2.expire_at = Now() + conversion_2.observation_window;
  conversions.push_back(conversion_2);

  SaveConversions(conversions);

  // Act
  AdvanceClockBy(base::Days(4));

  PurgeExpiredConversions();

  // Assert
  const ConversionList expected_conversions = {conversion_1};

  database_table_.GetAll(base::BindOnce(
      [](const ConversionList& expected_conversions, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_conversions, conversions));
      },
      expected_conversions));
}

TEST_F(BraveAdsConversionsDatabaseTableTest,
       SaveConversionWithMatchingCreativeSetIdAndType) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion_1;
  conversion_1.creative_set_id = kCreativeSetId;
  conversion_1.type = "postview";
  conversion_1.url_pattern = "https://www.brave.com/1";
  conversion_1.observation_window = base::Days(3);
  conversion_1.expire_at = Now() + conversion_1.observation_window;
  conversions.push_back(conversion_1);

  SaveConversions(conversions);

  // Act
  ConversionInfo conversion_2;  // Should supersede conversion_1
  conversion_2.creative_set_id = kCreativeSetId;
  conversion_2.type = "postview";
  conversion_2.url_pattern = "https://www.brave.com/2";
  conversion_2.observation_window = base::Days(30);
  conversion_2.expire_at = Now() + conversion_2.observation_window;
  conversions.push_back(conversion_2);

  SaveConversions(conversions);

  // Assert
  const ConversionList expected_conversions = {conversion_2};

  database_table_.GetAll(base::BindOnce(
      [](const ConversionList& expected_conversions, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversions, conversions);
      },
      expected_conversions));
}

TEST_F(BraveAdsConversionsDatabaseTableTest, TableName) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("creative_ad_conversions", database_table_.GetTableName());
}

}  // namespace brave_ads::database::table
