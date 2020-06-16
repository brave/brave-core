/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification/classification_util.h"

#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace classification {

TEST(BatAdsClassificationUtilTest,
    SplitParentChildCategory) {
  // Arrange
  const std::string category = "parent-child";

  // Act
  const std::vector<std::string> categories = SplitCategory(category);

  // Assert
  const std::vector<std::string> expected_categories = {
    "parent",
    "child"
  };

  EXPECT_EQ(expected_categories, categories);
}

TEST(BatAdsClassificationUtilTest,
    SplitParentCategory) {
  // Arrange
  const std::string category = "parent";

  // Act
  const std::vector<std::string> categories = SplitCategory(category);

  // Assert
  const std::vector<std::string> expected_categories = {
    "parent"
  };

  EXPECT_EQ(expected_categories, categories);
}

TEST(BatAdsClassificationUtilTest,
    SplitEmptyCategory) {
  // Arrange
  const std::string category = "";

  // Act
  const std::vector<std::string> categories = SplitCategory(category);

  // Assert
  const std::vector<std::string> expected_categories = {};

  EXPECT_EQ(expected_categories, categories);
}

}  // namespace classification
}  // namespace ads
