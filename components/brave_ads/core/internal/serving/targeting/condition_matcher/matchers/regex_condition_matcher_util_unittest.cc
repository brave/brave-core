/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/regex_condition_matcher_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRegexConditionMatcherUtilTest : public test::TestBase {};

TEST_F(BraveAdsRegexConditionMatcherUtilTest, MatchRegex) {
  // Act & Assert
  EXPECT_TRUE(MatchRegex("foo.baz.bar", "(foo|bar)"));
}

TEST_F(BraveAdsRegexConditionMatcherUtilTest, MatchEmptyRegex) {
  // Act & Assert
  EXPECT_TRUE(MatchRegex("", ""));
}

TEST_F(BraveAdsRegexConditionMatcherUtilTest, DoNotMatchRegex) {
  // Act & Assert
  EXPECT_FALSE(MatchRegex("foo.baz.bar", "(waldo|fred)"));
}

TEST_F(BraveAdsRegexConditionMatcherUtilTest, DoNotMatchMalformedRegex) {
  // Act & Assert
  EXPECT_FALSE(MatchRegex("foo.baz.bar", "* ?"));
}

}  // namespace brave_ads
