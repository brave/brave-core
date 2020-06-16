/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/ad_grants.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

namespace confirmations {

TEST(BatConfirmationsAdGrantsTest,
    InvalidJson) {
  // Arrange
  const std::string json = "{FOOBAR}";

  AdGrants ad_grants;

  // Act
  const bool is_valid = ad_grants.SetFromJson(json);

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST(BatConfirmationsAdGrantsTest,
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

TEST(BatConfirmationsAdGrantsTest,
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

TEST(BatConfirmationsAdGrantsTest,
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

TEST(BatConfirmationsAdGrantsTest,
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

}  // namespace confirmations
