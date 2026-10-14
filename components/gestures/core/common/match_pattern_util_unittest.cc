// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/match_pattern_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(MatchPatternUtilTest, MatchRegexReturnsTrueForMatchingValidPattern) {
  EXPECT_TRUE(MatchRegex("https://foo.com/title2.html", "title2\\.html$"));
}

TEST(MatchPatternUtilTest, MatchRegexReturnsFalseForInvalidPattern) {
  EXPECT_FALSE(MatchRegex("https://foo.com/title2.html", "title2("));
}

TEST(MatchPatternUtilTest, MatchesAGlobPattern) {
  EXPECT_TRUE(MatchesPattern("https://foo.com/title2.html", "*title2*"));
}

TEST(MatchPatternUtilTest, DoesNotMatchAGlobPatternThatDoesNotFit) {
  EXPECT_FALSE(MatchesPattern("https://foo.com/title3.html", "*title2*"));
}

TEST(MatchPatternUtilTest, MatchesARegexPattern) {
  EXPECT_TRUE(MatchesPattern("https://foo.com/title2.html", "title2\\.html$"));
}

TEST(MatchPatternUtilTest, TreatsAnInvalidRegexAsNoMatch) {
  EXPECT_FALSE(MatchesPattern("https://foo.com/title2.html", "title2("));
}

}  // namespace gestures
