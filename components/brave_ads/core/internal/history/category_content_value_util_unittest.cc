/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/category_content_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/category_content_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/history/category_content_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {
constexpr char kJson[] = R"({"category":"untargeted","optAction":0})";
}  // namespace

class BatAdsCategoryContentValueUtilTest : public UnitTestBase {};

TEST_F(BatAdsCategoryContentValueUtilTest, FromValue) {
  // Arrange
  const base::Value value = base::test::ParseJson(kJson);
  const base::Value::Dict* const dict = value.GetIfDict();
  ASSERT_TRUE(dict);

  // Act
  const CategoryContentInfo category_content = CategoryContentFromValue(*dict);

  // Assert
  const CategoryContentInfo expected_category_content =
      BuildCategoryContent(kSegment);
  EXPECT_EQ(expected_category_content, category_content);
}

TEST_F(BatAdsCategoryContentValueUtilTest, ToValue) {
  // Arrange
  const CategoryContentInfo category_content = BuildCategoryContent(kSegment);

  // Act
  const base::Value::Dict value = CategoryContentToValue(category_content);

  // Assert
  const base::Value expected_value = base::test::ParseJson(kJson);
  EXPECT_EQ(expected_value, value);
}

}  // namespace brave_ads
