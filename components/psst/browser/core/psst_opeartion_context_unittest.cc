// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_opeartion_context.h"

#include <memory>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/url_util.h"

using base::test::ParseJson;

namespace psst {

namespace {

constexpr char kUserScriptResult[] = R"({
    "user": "$1",
    "tasks": [
        {
            "url": "https://x.com/settings/location",
            "description": "Disable attaching location information to posts"
        },
        {
            "url": "https://x.com/settings/data_sharing_with_business_partners",
            "description": "Disable sharing additional information partners."
        },
        {
            "url": "https://x.com/settings/off_twitter_activity",
            "description": "Disable personalization based on your identity"
        },
        {
            "url": "https://x.com/settings/ads_preferences",
            "description": "Disable personalized ads"
        }
    ]
})";

}  // namespace

class PsstOperationContextUnitTest : public testing::Test {
 public:
  void SetUserScriptResult(PsstOperationContext& context,
                           const std::string& name,
                           const std::string& user_script,
                           const std::string& policy_script,
                           int version,
                           base::Value user_script_result) {
    MatchedRule rule(name, user_script, policy_script, version);
    context.SetUserScriptResult(user_script_result, rule);
  }
};

TEST_F(PsstOperationContextUnitTest, LoadContext) {
  constexpr char rule_name[] = "test rule name";
  {
    PsstOperationContext context;
    SetUserScriptResult(context, rule_name, "user script", "policy script", 1,
                        ParseJson(R"([])"));
    EXPECT_FALSE(context.IsUserScriptExecuted());
    EXPECT_FALSE(context.IsPolicyScriptExecuted());

    SetUserScriptResult(context, rule_name, "user script", "policy script", 1,
                        ParseJson(R"({})"));
    EXPECT_FALSE(context.IsUserScriptExecuted());
    EXPECT_FALSE(context.IsPolicyScriptExecuted());

    SetUserScriptResult(context, rule_name, "user script", "policy script", 1,
                        ParseJson(R"({"user": "value"})"));
    EXPECT_TRUE(context.IsUserScriptExecuted());
    EXPECT_FALSE(context.IsPolicyScriptExecuted());
  }

  constexpr char user_id[] = "user12345";
  auto user_result = ParseJson(
      base::ReplaceStringPlaceholders(kUserScriptResult, {user_id}, nullptr));
  PsstOperationContext context;
  SetUserScriptResult(context, rule_name, "user script", "policy script", 1,
                      std::move(user_result));
  EXPECT_TRUE(context.IsUserScriptExecuted());
  EXPECT_FALSE(context.IsPolicyScriptExecuted());
  EXPECT_EQ(context.GetUserId(), user_id);
  EXPECT_EQ(context.GetRuleName(), rule_name);
}

}  // namespace psst
