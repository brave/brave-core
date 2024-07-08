/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/category_content_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/history/category_content_util.h"
#include "brave/components/brave_ads/core/public/history/category_content_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kJson[] =
    R"(
        {
          "category": "untargeted",
          "optAction": 0
        })";

}  // namespace

class BraveAdsCategoryContentValueUtilTest : public UnitTestBase {};

TEST_F(BraveAdsCategoryContentValueUtilTest, FromValue) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict(kJson);

  // Act
  const CategoryContentInfo category_content = CategoryContentFromValue(dict);

  // Assert
  EXPECT_EQ(BuildCategoryContent(test::kSegment), category_content);
}

TEST_F(BraveAdsCategoryContentValueUtilTest, ToValue) {
  // Arrange
  const CategoryContentInfo category_content =
      BuildCategoryContent(test::kSegment);

  // Act
  const base::Value::Dict dict = CategoryContentToValue(category_content);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(kJson), dict);
}

}  // namespace brave_ads
