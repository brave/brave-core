/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversions_database_table.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_database_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::database::table {

namespace {

base::Time CalculateExpireAtTime(const int observation_window) {
  return Now() + base::Days(observation_window);
}

}  // namespace

class BatAdsConversionsDatabaseTableTest : public UnitTestBase {
 protected:
  Conversions database_table_;
};

TEST_F(BatAdsConversionsDatabaseTableTest, EmptySave) {
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

TEST_F(BatAdsConversionsDatabaseTableTest, SaveConversions) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion_1;
  conversion_1.creative_set_id = "340c927f-696e-4060-9933-3eafc56c3f31";
  conversion_1.type = "postview";
  conversion_1.url_pattern = "https://www.brave.com/*";
  conversion_1.observation_window = 3;
  conversion_1.expire_at =
      CalculateExpireAtTime(conversion_1.observation_window);
  conversions.push_back(conversion_1);

  ConversionInfo conversion_2;
  conversion_2.creative_set_id = "eaa6224a-46a4-4c48-9c2b-c264c0067f04";
  conversion_2.type = "postclick";
  conversion_2.url_pattern = "https://www.brave.com/signup/*";
  conversion_2.observation_window = 30;
  conversion_2.expire_at =
      CalculateExpireAtTime(conversion_2.observation_window);
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

TEST_F(BatAdsConversionsDatabaseTableTest, DoNotSaveDuplicateConversion) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "340c927f-696e-4060-9933-3eafc56c3f31";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.brave.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
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

TEST_F(BatAdsConversionsDatabaseTableTest, PurgeExpiredConversions) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion_1;
  conversion_1.creative_set_id = "340c927f-696e-4060-9933-3eafc56c3f31";
  conversion_1.type = "postview";
  conversion_1.url_pattern = "https://www.brave.com/*";
  conversion_1.observation_window = 7;
  conversion_1.expire_at =
      CalculateExpireAtTime(conversion_1.observation_window);
  conversions.push_back(conversion_1);

  ConversionInfo conversion_2;  // Should be purged
  conversion_2.creative_set_id = "eaa6224a-46a4-4c48-9c2b-c264c0067f04";
  conversion_2.type = "postclick";
  conversion_2.url_pattern = "https://www.brave.com/signup/*";
  conversion_2.observation_window = 3;
  conversion_2.expire_at =
      CalculateExpireAtTime(conversion_2.observation_window);
  conversions.push_back(conversion_2);

  ConversionInfo conversion_3;
  conversion_3.creative_set_id = "8e9f0c2f-1640-463c-902d-ca711789287f";
  conversion_3.type = "postview";
  conversion_3.url_pattern = "https://www.brave.com/*";
  conversion_3.observation_window = 30;
  conversion_3.expire_at =
      CalculateExpireAtTime(conversion_3.observation_window);
  conversions.push_back(conversion_3);

  SaveConversions(conversions);

  // Act
  AdvanceClockBy(base::Days(4));

  PurgeExpiredConversions();

  // Assert
  const ConversionList expected_conversions = {conversion_1, conversion_3};

  database_table_.GetAll(base::BindOnce(
      [](const ConversionList& expected_conversions, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_conversions, conversions));
      },
      expected_conversions));
}

TEST_F(BatAdsConversionsDatabaseTableTest,
       SaveConversionWithMatchingCreativeSetIdAndType) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion_1;
  conversion_1.creative_set_id = "340c927f-696e-4060-9933-3eafc56c3f31";
  conversion_1.type = "postview";
  conversion_1.url_pattern = "https://www.brave.com/1";
  conversion_1.observation_window = 3;
  conversion_1.expire_at =
      CalculateExpireAtTime(conversion_1.observation_window);
  conversions.push_back(conversion_1);

  SaveConversions(conversions);

  // Act
  ConversionInfo conversion_2;  // Should supersede conversion_1
  conversion_2.creative_set_id = "340c927f-696e-4060-9933-3eafc56c3f31";
  conversion_2.type = "postview";
  conversion_2.url_pattern = "https://www.brave.com/2";
  conversion_2.observation_window = 30;
  conversion_2.expire_at =
      CalculateExpireAtTime(conversion_2.observation_window);
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

TEST_F(BatAdsConversionsDatabaseTableTest, TableName) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("creative_ad_conversions", database_table_.GetTableName());
}

}  // namespace brave_ads::database::table
