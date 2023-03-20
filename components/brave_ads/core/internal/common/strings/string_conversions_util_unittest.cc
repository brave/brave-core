/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

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

TEST(BatAdsStringConversionsUtilTest, ConvertDelimitedStringToVector) {
  // Arrange
  const std::string string = "1.2,2.3,3.4,4.5,5.6";
  const std::string delimiter = ",";

  // Act
  const std::vector<float> vector =
      ConvertDelimitedStringToVector(string, delimiter);

  // Assert
  const std::vector<float> expected_vector = {1.2F, 2.3F, 3.4F, 4.5F, 5.6F};
  for (size_t i = 0; i < vector.size(); i++) {
    EXPECT_NEAR(expected_vector[i], vector[i], 0.001F);
  }
}

TEST(BatAdsStringConversionsUtilTest, ConvertVectorToDelimitedString) {
  // Arrange
  const std::vector<float> vector = {1.2F, 2.3F, 3.4F, 4.5F, 5.6F};
  const std::string delimiter = ",";

  // Act
  const std::string string = ConvertVectorToDelimitedString(vector, delimiter);
  const std::vector<float> string_vector =
      ConvertDelimitedStringToVector(string, delimiter);

  // Assert
  const std::string expected_string = "1.2,2.3,3.4,4.5,5.6";
  const std::vector<float> expected_vector =
      ConvertDelimitedStringToVector(expected_string, delimiter);
  for (size_t i = 0; i < vector.size(); i++) {
    EXPECT_NEAR(expected_vector[i], string_vector[i], 0.001F);
  }
}

TEST(BatAdsStringConversionsUtilTest, ReflexiveConvertVectorToDelimitedString) {
  // Arrange
  const std::vector<float> vector = {1.2F, 2.3F, 3.4F, 4.5F, 5.6F};
  const std::string delimiter = ",";

  // Act
  const std::string string = ConvertVectorToDelimitedString(vector, delimiter);
  const std::vector<float> string_vector =
      ConvertDelimitedStringToVector(string, delimiter);

  // Assert
  for (size_t i = 0; i < vector.size(); i++) {
    EXPECT_NEAR(vector[i], string_vector[i], 0.001F);
  }
}

}  // namespace brave_ads
