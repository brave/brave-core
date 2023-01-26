/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions_database_table.h"

#include <memory>

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_container_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/conversions/conversions_database_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::database::table {

namespace {

base::Time CalculateExpireAtTime(const int observation_window) {
  return Now() + base::Days(observation_window);
}

}  // namespace

class BatAdsConversionsDatabaseTableTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    database_table_ = std::make_unique<Conversions>();
  }

  std::unique_ptr<Conversions> database_table_;
};

TEST_F(BatAdsConversionsDatabaseTableTest, EmptySave) {
  // Arrange
  const ConversionList conversions;

  // Act
  SaveConversions(conversions);

  // Assert
  database_table_->GetAll(base::BindOnce(
      [](const ConversionList& expected_conversions, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_conversions, conversions));
      },
      conversions));
}

TEST_F(BatAdsConversionsDatabaseTableTest, SaveConversions) {
  // Arrange
  ConversionList conversions;

  ConversionInfo info_1;
  info_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.type = "postview";
  info_1.url_pattern = "https://www.brave.com/*";
  info_1.observation_window = 3;
  info_1.expire_at = CalculateExpireAtTime(info_1.observation_window);
  conversions.push_back(info_1);

  ConversionInfo info_2;
  info_2.creative_set_id = "eaa6224a-46a4-4c48-9c2b-c264c0067f04";
  info_2.type = "postclick";
  info_2.url_pattern = "https://www.brave.com/signup/*";
  info_2.observation_window = 30;
  info_2.expire_at = CalculateExpireAtTime(info_2.observation_window);
  conversions.push_back(info_2);

  // Act
  SaveConversions(conversions);

  // Assert
  const ConversionList expected_conversions = conversions;

  database_table_->GetAll(base::BindOnce(
      [](const ConversionList& expected_conversions, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_conversions, conversions));
      },
      expected_conversions));
}

TEST_F(BatAdsConversionsDatabaseTableTest, DoNotSaveDuplicateConversion) {
  // Arrange
  ConversionList conversions;

  ConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expire_at = CalculateExpireAtTime(info.observation_window);
  conversions.push_back(info);

  SaveConversions(conversions);

  // Act
  SaveConversions(conversions);

  // Assert
  const ConversionList expected_conversions = conversions;

  database_table_->GetAll(base::BindOnce(
      [](const ConversionList& expected_conversions, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_conversions, conversions));
      },
      expected_conversions));
}

TEST_F(BatAdsConversionsDatabaseTableTest, PurgeExpiredConversions) {
  // Arrange
  ConversionList conversions;

  ConversionInfo info_1;
  info_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.type = "postview";
  info_1.url_pattern = "https://www.brave.com/*";
  info_1.observation_window = 7;
  info_1.expire_at = CalculateExpireAtTime(info_1.observation_window);
  conversions.push_back(info_1);

  ConversionInfo info_2;  // Should be purged
  info_2.creative_set_id = "eaa6224a-46a4-4c48-9c2b-c264c0067f04";
  info_2.type = "postclick";
  info_2.url_pattern = "https://www.brave.com/signup/*";
  info_2.observation_window = 3;
  info_2.expire_at = CalculateExpireAtTime(info_2.observation_window);
  conversions.push_back(info_2);

  ConversionInfo info_3;
  info_3.creative_set_id = "8e9f0c2f-1640-463c-902d-ca711789287f";
  info_3.type = "postview";
  info_3.url_pattern = "https://www.brave.com/*";
  info_3.observation_window = 30;
  info_3.expire_at = CalculateExpireAtTime(info_3.observation_window);
  conversions.push_back(info_3);

  SaveConversions(conversions);

  // Act
  AdvanceClockBy(base::Days(4));

  PurgeExpiredConversions();

  // Assert
  ConversionList expected_conversions;
  expected_conversions.push_back(info_1);
  expected_conversions.push_back(info_3);

  database_table_->GetAll(base::BindOnce(
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

  ConversionInfo info_1;
  info_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.type = "postview";
  info_1.url_pattern = "https://www.brave.com/1";
  info_1.observation_window = 3;
  info_1.expire_at = CalculateExpireAtTime(info_1.observation_window);
  conversions.push_back(info_1);

  SaveConversions(conversions);

  // Act
  ConversionInfo info_2;  // Should supersede info_1
  info_2.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_2.type = "postview";
  info_2.url_pattern = "https://www.brave.com/2";
  info_2.observation_window = 30;
  info_2.expire_at = CalculateExpireAtTime(info_2.observation_window);
  conversions.push_back(info_2);

  SaveConversions(conversions);

  // Assert
  ConversionList expected_conversions;
  expected_conversions.push_back(info_2);

  database_table_->GetAll(base::BindOnce(
      [](const ConversionList& expected_conversions, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_conversions, conversions));
      },
      expected_conversions));
}

TEST_F(BatAdsConversionsDatabaseTableTest, TableName) {
  // Arrange

  // Act
  const std::string table_name = database_table_->GetTableName();

  // Assert
  const std::string expected_table_name = "creative_ad_conversions";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads::database::table
