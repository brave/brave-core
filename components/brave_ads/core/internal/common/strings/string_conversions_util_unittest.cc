/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {

constexpr char kDelimiter[] = ",";
const std::vector<float> kTestVector = {1.2F, 2.3F, 3.4F, 4.5F, 5.6F};
}  // namespace

TEST(BatAdsStringConversionsUtilTest, TrueBoolToString) {
  // Arrange

  // Act
  const std::string value = BoolToString(true);

  // Assert
  EXPECT_EQ("true", value);
}

TEST(BatAdsStringConversionsUtilTest, FalseBoolToString) {
  // Arrange

  // Act
  const std::string value = BoolToString(false);

  // Assert
  EXPECT_EQ("false", value);
}

TEST(BatAdsStringConversionsUtilTest, DelimitedStringToVector) {
  // Arrange
  const std::string delimited_string = "1.2,2.3,3.4,4.5,5.6";

  // Act
  const std::vector<float> vector =
      DelimitedStringToVector(delimited_string, kDelimiter);

  // Assert
  for (size_t i = 0; i < vector.size(); i++) {
    EXPECT_NEAR(kTestVector[i], vector[i], 0.001F);
  }
}

TEST(BatAdsStringConversionsUtilTest, VectorToDelimitedString) {
  // Arrange

  // Act
  const std::string string = VectorToDelimitedString(kTestVector, kDelimiter);
  const std::vector<float> string_vector =
      DelimitedStringToVector(string, kDelimiter);

  // Assert
  const std::string expected_string = "1.2,2.3,3.4,4.5,5.6";
  const std::vector<float> expected_vector =
      DelimitedStringToVector(expected_string, kDelimiter);
  for (size_t i = 0; i < kTestVector.size(); i++) {
    EXPECT_NEAR(expected_vector[i], string_vector[i], 0.001F);
  }
}

TEST(BatAdsStringConversionsUtilTest, ReflexiveVectorToDelimitedString) {
  // Arrange

  // Act
  const std::string string = VectorToDelimitedString(kTestVector, kDelimiter);
  const std::vector<float> string_vector =
      DelimitedStringToVector(string, kDelimiter);

  // Assert
  for (size_t i = 0; i < kTestVector.size(); i++) {
    EXPECT_NEAR(kTestVector[i], string_vector[i], 0.001F);
  }
}

}  // namespace brave_ads
