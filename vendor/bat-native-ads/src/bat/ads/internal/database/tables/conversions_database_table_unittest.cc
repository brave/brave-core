/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/conversions_database_table.h"

#include <cstdint>
#include <memory>

#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsConversionsDatabaseTableTest : public UnitTestBase {
 protected:
  BatAdsConversionsDatabaseTableTest()
      : database_table_(std::make_unique<database::table::Conversions>()) {}

  ~BatAdsConversionsDatabaseTableTest() override = default;

  void Save(const ConversionList& conversions) {
    database_table_->Save(conversions,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  void PurgeExpired() {
    database_table_->PurgeExpired(
        [](const bool success) { ASSERT_TRUE(success); });
  }

  int64_t CalculateExpiryTimestamp(const int observation_window) {
    base::Time time = base::Time::Now();
    time += base::TimeDelta::FromDays(observation_window);
    return static_cast<int64_t>(time.ToDoubleT());
  }

  std::unique_ptr<database::table::Conversions> database_table_;
};

TEST_F(BatAdsConversionsDatabaseTableTest, EmptySave) {
  // Arrange
  ConversionList conversions = {};

  // Act
  Save(conversions);

  // Assert
  const ConversionList expected_conversions = conversions;

  database_table_->GetAll(
      [&expected_conversions](const bool success,
                              const ConversionList& conversions) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_conversions, conversions));
      });
}

TEST_F(BatAdsConversionsDatabaseTableTest, SaveConversions) {
  // Arrange
  ConversionList conversions;

  ConversionInfo info_1;
  info_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.type = "postview";
  info_1.url_pattern = "https://www.brave.com/*";
  info_1.observation_window = 3;
  info_1.expiry_timestamp = CalculateExpiryTimestamp(info_1.observation_window);
  conversions.push_back(info_1);

  ConversionInfo info_2;
  info_2.creative_set_id = "eaa6224a-46a4-4c48-9c2b-c264c0067f04";
  info_2.type = "postclick";
  info_2.url_pattern = "https://www.brave.com/signup/*";
  info_2.observation_window = 30;
  info_2.expiry_timestamp = CalculateExpiryTimestamp(info_2.observation_window);
  conversions.push_back(info_2);

  // Act
  Save(conversions);

  // Assert
  const ConversionList expected_conversions = conversions;

  database_table_->GetAll(
      [&expected_conversions](const bool success,
                              const ConversionList& conversions) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_conversions, conversions));
      });
}

TEST_F(BatAdsConversionsDatabaseTableTest, DoNotSaveDuplicateConversion) {
  // Arrange;
  ConversionList conversions;

  ConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  conversions.push_back(info);

  Save(conversions);

  // Act
  Save(conversions);

  // Assert
  const ConversionList expected_conversions = conversions;

  database_table_->GetAll(
      [&expected_conversions](const bool success,
                              const ConversionList& conversions) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_conversions, conversions));
      });
}

TEST_F(BatAdsConversionsDatabaseTableTest, PurgeExpiredConversions) {
  // Arrange
  ConversionList conversions;

  ConversionInfo info_1;
  info_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.type = "postview";
  info_1.url_pattern = "https://www.brave.com/*";
  info_1.observation_window = 7;
  info_1.expiry_timestamp = CalculateExpiryTimestamp(info_1.observation_window);
  conversions.push_back(info_1);

  ConversionInfo info_2;  // Should be purged
  info_2.creative_set_id = "eaa6224a-46a4-4c48-9c2b-c264c0067f04";
  info_2.type = "postclick";
  info_2.url_pattern = "https://www.brave.com/signup/*";
  info_2.observation_window = 3;
  info_2.expiry_timestamp = CalculateExpiryTimestamp(info_2.observation_window);
  conversions.push_back(info_2);

  ConversionInfo info_3;
  info_3.creative_set_id = "8e9f0c2f-1640-463c-902d-ca711789287f";
  info_3.type = "postview";
  info_3.url_pattern = "https://www.brave.com/*";
  info_3.observation_window = 30;
  info_3.expiry_timestamp = CalculateExpiryTimestamp(info_3.observation_window);
  conversions.push_back(info_3);

  Save(conversions);

  // Act
  FastForwardClockBy(base::TimeDelta::FromDays(4));

  PurgeExpired();

  // Assert
  ConversionList expected_conversions;
  expected_conversions.push_back(info_1);
  expected_conversions.push_back(info_3);

  database_table_->GetAll(
      [&expected_conversions](const bool success,
                              const ConversionList& conversions) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_conversions, conversions));
      });
}

TEST_F(BatAdsConversionsDatabaseTableTest,
       SaveConversionWithMatchingCreativeSetIdAndTypeAndUrlPattern) {
  // Arrange
  ConversionList conversions;

  ConversionInfo info_1;
  info_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.type = "postview";
  info_1.url_pattern = "https://www.brave.com/*";
  info_1.observation_window = 3;
  info_1.expiry_timestamp = CalculateExpiryTimestamp(info_1.observation_window);
  conversions.push_back(info_1);

  Save(conversions);

  // Act
  ConversionInfo info_2;  // Should supersede info_1
  info_2.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_2.type = "postview";
  info_2.url_pattern = "https://www.brave.com/*";
  info_2.observation_window = 30;
  info_2.expiry_timestamp = CalculateExpiryTimestamp(info_2.observation_window);
  conversions.push_back(info_2);

  Save(conversions);

  // Assert
  ConversionList expected_conversions;
  expected_conversions.push_back(info_2);

  database_table_->GetAll(
      [&expected_conversions](const bool success,
                              const ConversionList& conversions) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_conversions, conversions));
      });
}

TEST_F(BatAdsConversionsDatabaseTableTest, TableName) {
  // Arrange

  // Act
  const std::string table_name = database_table_->get_table_name();

  // Assert
  const std::string expected_table_name = "creative_ad_conversions";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads
