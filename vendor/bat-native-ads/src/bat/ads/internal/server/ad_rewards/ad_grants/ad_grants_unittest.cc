/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/ad_rewards/ad_grants/ad_grants.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsAdGrantsTest,
    InvalidJson) {
  // Arrange
  const std::string json = "{FOOBAR}";

  AdGrants ad_grants;

  // Act
  const bool success = ad_grants.SetFromJson(json);

  // Assert
  EXPECT_FALSE(success);
}

TEST(BatAdsAdGrantsTest,
    DoubleForAmount) {
  // Arrange
  const std::string json = R"(
    {
      "type" : "ads",
      "amount" : "5.0",
      "lastClaim" : "2019-06-13T12:14:46.150Z"
    }
  )";

  AdGrants ad_grants;
  ad_grants.SetFromJson(json);

  // Act
  const double balance = ad_grants.GetBalance();

  // Assert
  EXPECT_EQ(5.0, balance);
}

TEST(BatAdsAdGrantsTest,
    IntegerForAmount) {
  // Arrange
  const std::string json = R"(
    {
      "type" : "ads",
      "amount" : "5",
      "lastClaim" : "2019-06-13T12:14:46.150Z"
    }
  )";

  AdGrants ad_grants;
  ad_grants.SetFromJson(json);

  // Act
  const double balance = ad_grants.GetBalance();

  // Assert
  EXPECT_EQ(5.0, balance);
}

TEST(BatAdsAdGrantsTest,
     InvalidStringForAmount) {
  // Arrange
  const std::string json = R"(
    {
      "type" : "ads",
      "amount" : "INVALID",
      "lastClaim" : "2019-06-13T12:14:46.150Z"
    }
  )";

  AdGrants ad_grants;
  ad_grants.SetFromJson(json);

  // Act
  const double balance = ad_grants.GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

TEST(BatAdsAdGrantsTest,
    InvalidTypeForAmount) {
  // Arrange
  const std::string json = R"(
    {
      "type" : "ads",
      "amount" : 1,
      "lastClaim" : "2019-06-13T12:14:46.150Z"
    }
  )";

  AdGrants ad_grants;
  ad_grants.SetFromJson(json);

  // Act
  const double balance = ad_grants.GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

}  // namespace ads
