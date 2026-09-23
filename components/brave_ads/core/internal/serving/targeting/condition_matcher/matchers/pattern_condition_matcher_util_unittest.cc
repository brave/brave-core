/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/pattern_condition_matcher_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPatternConditionMatcherUtilTest : public test::TestBase {};

TEST_F(BraveAdsPatternConditionMatcherUtilTest, MatchPattern) {
  // Act & Assert
  EXPECT_TRUE(MatchPattern("foo.baz.bar", "foo?baz.*"));
}

TEST_F(BraveAdsPatternConditionMatcherUtilTest, MatchEmptyPattern) {
  // Act & Assert
  EXPECT_TRUE(MatchPattern("", ""));
}

TEST_F(BraveAdsPatternConditionMatcherUtilTest, MatchEscapedPattern) {
  // Act & Assert
  EXPECT_TRUE(MatchPattern(R"(*.bar.?)", R"(\*.bar.\?)"));
}

TEST_F(BraveAdsPatternConditionMatcherUtilTest, DoNotMatchPattern) {
  // Act & Assert
  EXPECT_FALSE(MatchPattern("foo.baz.bar", "bar.*.foo"));
}

}  // namespace brave_ads
