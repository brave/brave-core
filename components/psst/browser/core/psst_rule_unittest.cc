// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_rule.h"

#include <cstddef>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace psst {

namespace {

constexpr char kPsstJsonFileContent[] = R"([
        {
            "name": "a",
            "include": [
                "https://a.com/*"
            ],
            "exclude": [
                "https://a.com/exclude/*"
            ],
            "version": 1,
            "user_script": "user.js",
            "policy_script": "policy.js"
        }
    ])";
constexpr char kPsstJsonFileContentNoExclude[] = R"([
        {
            "name": "b",
            "include": [
                "https://b.com/*"
            ],
            "exclude": [
            ],
            "version": 2,
            "user_script": "user_script.js",
            "policy_script": "policy_script.js"
        }
    ])";

}  // namespace

class PsstRuleUnitTest : public testing::Test {};

TEST_F(PsstRuleUnitTest, LoadRuleWithExclude) {
  const auto psst_rules_with_exclude =
      PsstRule::ParseRules(kPsstJsonFileContent);
  ASSERT_EQ(psst_rules_with_exclude->size(), 1u);

  EXPECT_EQ(psst_rules_with_exclude->front().Name(), "a");
  EXPECT_EQ(psst_rules_with_exclude->front().Version(), 1);
  EXPECT_EQ(psst_rules_with_exclude->front().UserScriptPath().value(),
            "user.js");
  EXPECT_EQ(psst_rules_with_exclude->front().PolicyScriptPath().value(),
            "policy.js");

  EXPECT_TRUE(psst_rules_with_exclude->front().ShouldInsertScript(
      GURL("https://a.com/page.html")));
  EXPECT_FALSE(psst_rules_with_exclude->front().ShouldInsertScript(
      GURL("https://a.com/exclude/page.html")));
}

TEST_F(PsstRuleUnitTest, LoadRuleNoExclude) {
  const auto psst_rules_with_exclude =
      PsstRule::ParseRules(kPsstJsonFileContentNoExclude);
  ASSERT_EQ(psst_rules_with_exclude->size(), 1u);

  EXPECT_EQ(psst_rules_with_exclude->front().Name(), "b");
  EXPECT_EQ(psst_rules_with_exclude->front().Version(), 2);
  EXPECT_EQ(psst_rules_with_exclude->front().UserScriptPath().value(),
            "user_script.js");
  EXPECT_EQ(psst_rules_with_exclude->front().PolicyScriptPath().value(),
            "policy_script.js");

  EXPECT_TRUE(psst_rules_with_exclude->front().ShouldInsertScript(
      GURL("https://b.com/page.html")));
  EXPECT_TRUE(psst_rules_with_exclude->front().ShouldInsertScript(
      GURL("https://b.com/exclude/page.html")));
}

}  // namespace psst
