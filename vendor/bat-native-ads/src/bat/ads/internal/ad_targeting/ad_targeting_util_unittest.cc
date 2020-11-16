/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting_util.h"

#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {

TEST(BatAdsAdTargetingUtilTest,
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

TEST(BatAdsAdTargetingUtilTest,
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

TEST(BatAdsAdTargetingUtilTest,
    SplitEmptyCategory) {
  // Arrange
  const std::string category = "";

  // Act
  const std::vector<std::string> categories = SplitCategory(category);

  // Assert
  const std::vector<std::string> expected_categories = {};

  EXPECT_EQ(expected_categories, categories);
}

TEST(BatAdsAdTargetingUtilTest,
    GetParentCategories) {
  // Arrange
  const std::vector<std::string> categories = {
    "technology & computing-software",
    "personal finance-personal finance",
    "automobiles"
  };

  // Act
  const CategoryList parent_categories = GetParentCategories(categories);

  // Assert
  const std::vector<std::string> expected_parent_categories = {
    "technology & computing",
    "personal finance",
    "automobiles"
  };

  EXPECT_EQ(expected_parent_categories, parent_categories);
}

TEST(BatAdsAdTargetingUtilTest,
    GetParentCategoriesForEmptyList) {
  // Arrange
  const std::vector<std::string> categories = {};

  // Act
  const CategoryList parent_categories = GetParentCategories(categories);

  // Assert
  const std::vector<std::string> expected_parent_categories = {};

  EXPECT_EQ(expected_parent_categories, parent_categories);
}

}  // namespace ad_targeting
}  // namespace ads
