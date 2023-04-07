/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/category_content_util.h"

#include "brave/components/brave_ads/core/category_content_action_types.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {
constexpr char kSegment[] = "segment";
}  // namespace

class BatAdsCategoryContentUtilTest : public UnitTestBase {};

TEST_F(BatAdsCategoryContentUtilTest, Build) {
  // Arrange

  // Act
  const CategoryContentInfo category_content = BuildCategoryContent(kSegment);

  // Assert
  CategoryContentInfo expected_category_content;
  expected_category_content.category = kSegment;
  expected_category_content.opt_action_type =
      CategoryContentOptActionType::kNone;

  EXPECT_EQ(expected_category_content, category_content);
}

}  // namespace brave_ads
