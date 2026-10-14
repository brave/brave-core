// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/tab_pattern_match_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(TabPatternMatchUtilTest, MatchRegexMatchesAValidPattern) {
  EXPECT_TRUE(MatchRegex("https://example.com/title2.html", "title2\\.html$"));
}

TEST(TabPatternMatchUtilTest, MatchRegexReturnsFalseForAnInvalidPattern) {
  EXPECT_FALSE(MatchRegex("https://example.com/title2.html", "title2("));
}

TEST(TabPatternMatchUtilTest, MatchesAGlobPattern) {
  EXPECT_TRUE(MatchesPattern("https://example.com/title2.html", "*title2*"));
}

TEST(TabPatternMatchUtilTest, DoesNotMatchAGlobPatternThatDoesNotFit) {
  EXPECT_FALSE(MatchesPattern("https://example.com/title3.html", "*title2*"));
}

TEST(TabPatternMatchUtilTest, MatchesARegexPattern) {
  EXPECT_TRUE(
      MatchesPattern("https://example.com/title2.html", "title2\\.html$"));
}

TEST(TabPatternMatchUtilTest, TreatsAnInvalidRegexAsNoMatch) {
  EXPECT_FALSE(MatchesPattern("https://example.com/title2.html", "title2("));
}

}  // namespace gestures
