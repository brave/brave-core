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

constexpr char kRules[] = R"([
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

constexpr char kRulesNoExclude[] = R"([
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

constexpr char kRulesWithSubdomain[] = R"([
      {
          "name": "a",
          "include": [
              "https://*.a.com/*"
          ],
          "exclude": [
              "https://a.com/exclude/*"
          ],
          "version": 1,
          "user_script": "user.js",
          "policy_script": "policy.js"
      }
  ])";

constexpr char kRulesMultiple[] = R"([
        {
            "name": "a",
            "include": [
                "https://a.com/*"
            ],
            "version": 1,
            "user_script": "user.js",
            "policy_script": "policy.js"
      },
      {
          "name": "b",
          "include": [
              "https://b.com/*"
          ],
          "exclude": [
                "https://b.com/exclude/*"
          ],
          "version": 2,
          "user_script": "user_script.js",
          "policy_script": "policy_script.js"
      }
  ])";

}  // namespace

class PsstRuleUnitTest : public testing::Test {};

// need test with more than one rule

TEST_F(PsstRuleUnitTest, ParseRulesWithExclude) {
  const auto psst_rules_with_exclude = PsstRule::ParseRules(kRules);
  ASSERT_EQ(psst_rules_with_exclude->size(), 1u);

  auto rule = psst_rules_with_exclude->front();

  EXPECT_EQ(rule.name(), "a");
  EXPECT_EQ(rule.version(), 1);
  EXPECT_EQ(rule.user_script_path().value(), FILE_PATH_LITERAL("user.js"));
  EXPECT_EQ(rule.policy_script_path().value(), FILE_PATH_LITERAL("policy.js"));
  // include rule
  EXPECT_TRUE(rule.ShouldInsertScript(GURL("https://a.com/page.html")));
  EXPECT_FALSE(rule.ShouldInsertScript(GURL("http://a.com/page.html")));
  EXPECT_FALSE(rule.ShouldInsertScript(GURL("https://b.a.com/page.html")));
  EXPECT_FALSE(rule.ShouldInsertScript(GURL("https://b.com/a.com")));

  // exclude rule
  EXPECT_FALSE(
      rule.ShouldInsertScript(GURL("https://a.com/exclude/page.html")));
  EXPECT_TRUE(
      rule.ShouldInsertScript(GURL("https://a.com/blah/exclude/page.html")));
}

TEST_F(PsstRuleUnitTest, ParseRulesNoExclude) {
  const auto psst_rules_with_exclude = PsstRule::ParseRules(kRulesNoExclude);
  ASSERT_EQ(psst_rules_with_exclude->size(), 1u);

  auto rule = psst_rules_with_exclude->front();
  EXPECT_EQ(rule.name(), "b");
  EXPECT_EQ(rule.version(), 2);
  EXPECT_EQ(rule.user_script_path().value(),
            FILE_PATH_LITERAL("user_script.js"));
  EXPECT_EQ(rule.policy_script_path().value(),
            FILE_PATH_LITERAL("policy_script.js"));

  EXPECT_TRUE(rule.ShouldInsertScript(GURL("https://b.com/page.html")));
  EXPECT_TRUE(rule.ShouldInsertScript(GURL("https://b.com/exclude/page.html")));
}

TEST_F(PsstRuleUnitTest, ParseRulesWithSubdomain) {
  const auto psst_rules_with_exclude =
      PsstRule::ParseRules(kRulesWithSubdomain);
  ASSERT_EQ(psst_rules_with_exclude->size(), 1u);

  auto rule = psst_rules_with_exclude->front();
  EXPECT_EQ(rule.name(), "a");
  EXPECT_EQ(rule.version(), 1);
  EXPECT_EQ(rule.user_script_path().value(), FILE_PATH_LITERAL("user.js"));
  EXPECT_EQ(rule.policy_script_path().value(), FILE_PATH_LITERAL("policy.js"));

  // include rule
  EXPECT_TRUE(rule.ShouldInsertScript(GURL("https://a.com/page.html")));
  EXPECT_TRUE(rule.ShouldInsertScript(GURL("https://b.a.com/page.html")));
  EXPECT_FALSE(rule.ShouldInsertScript(GURL("https://a.b.com/page.html")));

  // exclude rule
  EXPECT_TRUE(
      rule.ShouldInsertScript(GURL("https://b.a.com/exclude/page.html")));
  EXPECT_FALSE(
      rule.ShouldInsertScript(GURL("https://a.com/exclude/page.html")));
  EXPECT_TRUE(
      rule.ShouldInsertScript(GURL("https://a.com/blah/exclude/page.html")));
}

TEST_F(PsstRuleUnitTest, ParseRulesMultipleWithExclude) {
  const auto psst_rules_with_exclude = PsstRule::ParseRules(kRulesMultiple);
  ASSERT_EQ(psst_rules_with_exclude->size(), 2u);

  auto rule1 = (*psst_rules_with_exclude)[0];
  auto rule2 = (*psst_rules_with_exclude)[1];

  EXPECT_EQ(rule1.name(), "a");
  EXPECT_EQ(rule1.version(), 1);
  EXPECT_EQ(rule1.user_script_path().value(), FILE_PATH_LITERAL("user.js"));
  EXPECT_EQ(rule1.policy_script_path().value(), FILE_PATH_LITERAL("policy.js"));

  EXPECT_EQ(rule2.name(), "b");
  EXPECT_EQ(rule2.version(), 2);
  EXPECT_EQ(rule2.user_script_path().value(),
            FILE_PATH_LITERAL("user_script.js"));
  EXPECT_EQ(rule2.policy_script_path().value(),
            FILE_PATH_LITERAL("policy_script.js"));

  // rule1
  EXPECT_TRUE(rule1.ShouldInsertScript(GURL("https://a.com/page.html")));
  EXPECT_TRUE(
      rule1.ShouldInsertScript(GURL("https://a.com/exclude/page.html")));
  EXPECT_FALSE(rule1.ShouldInsertScript(GURL("https://b.com/page.html")));

  // rule2
  EXPECT_TRUE(rule2.ShouldInsertScript(GURL("https://b.com/page.html")));
  EXPECT_FALSE(
      rule2.ShouldInsertScript(GURL("https://b.com/exclude/page.html")));
  EXPECT_FALSE(rule2.ShouldInsertScript(GURL("https://a.com/page.html")));
}

TEST_F(PsstRuleUnitTest, ParseRulesInvalidContent) {
  // empty/non-existent file
  EXPECT_EQ(PsstRule::ParseRules(""), std::nullopt);
  // dictionary instead of array
  EXPECT_EQ(PsstRule::ParseRules("{}"), std::nullopt);
  // not valid json
  EXPECT_EQ(PsstRule::ParseRules("fdsa"), std::nullopt);
}

}  // namespace psst
