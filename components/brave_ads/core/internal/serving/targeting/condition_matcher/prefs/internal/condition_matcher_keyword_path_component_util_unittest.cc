/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_keyword_path_component_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConditionMatcherKeywordPathComponentUtilTest
    : public test::TestBase {};

TEST_F(BraveAdsConditionMatcherKeywordPathComponentUtilTest,
       ParseValueForExactKeyword) {
  // Act & Assert
  EXPECT_THAT(MaybeParseKeywordPathComponentValue("keyword", "keyword"),
              ::testing::Optional(::testing::IsEmpty()));
}

TEST_F(BraveAdsConditionMatcherKeywordPathComponentUtilTest,
       ParseValueForKeywordWithEmptyValue) {
  // Act & Assert
  EXPECT_THAT(MaybeParseKeywordPathComponentValue("keyword=", "keyword"),
              ::testing::Optional(::testing::IsEmpty()));
}

TEST_F(BraveAdsConditionMatcherKeywordPathComponentUtilTest,
       ParseValueForKeywordWithValue) {
  // Act & Assert
  EXPECT_EQ("value",
            MaybeParseKeywordPathComponentValue("keyword=value", "keyword"));
}

TEST_F(BraveAdsConditionMatcherKeywordPathComponentUtilTest,
       DoNotParseValueWhenKeywordHasNoSeparator) {
  // Act & Assert
  EXPECT_FALSE(MaybeParseKeywordPathComponentValue("keywordvalue", "keyword"));
}

TEST_F(BraveAdsConditionMatcherKeywordPathComponentUtilTest,
       DoNotParseValueWhenKeywordDoesNotMatch) {
  // Act & Assert
  EXPECT_FALSE(
      MaybeParseKeywordPathComponentValue("other_keyword=value", "keyword"));
}

TEST_F(BraveAdsConditionMatcherKeywordPathComponentUtilTest,
       DoNotParseValueForEmptyPathComponent) {
  // Act & Assert
  EXPECT_FALSE(MaybeParseKeywordPathComponentValue("", "keyword"));
}

}  // namespace brave_ads
