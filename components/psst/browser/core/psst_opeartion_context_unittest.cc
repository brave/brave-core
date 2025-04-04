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
#include "gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"
#include "url/url_util.h"

using base::test::ParseJson;

namespace psst {

class PsstOperationContextUnitTest : public testing::Test {
 public:
  std::unique_ptr<PsstOperationContext> TestLoadContext(
      const std::string& name,
      const std::string& user_script,
      const std::string& policy_script,
      int version,
      base::Value user_script_result) {
    MatchedRule rule(name, user_script, policy_script, version);
    return PsstOperationContext::LoadContext(user_script_result, rule);
  }
};

TEST_F(PsstOperationContextUnitTest, LoadContext) {
  constexpr char rule_name[] = "test rule name";

  EXPECT_FALSE(TestLoadContext(rule_name, "user script", "policy script", 1,
                               ParseJson(R"([])")));
  EXPECT_FALSE(TestLoadContext(rule_name, "user script", "policy script", 1,
                               ParseJson(R"({})")));
  EXPECT_FALSE(TestLoadContext(rule_name, "user script", "policy script", 1,
                               ParseJson(R"({"user": "value"})")));
  EXPECT_FALSE(
      TestLoadContext(rule_name, "user script", "policy script", 1,
                      ParseJson(R"({"share_experience_link": "value"})")));

  constexpr char user_id[] = "user12345";
  const std::u16string prepopulated_text(u"UTF16 Text to post. イロハニホヘト");
  constexpr char share_experience_link[] = "http://test.link/post=$1";

  url::RawCanonOutputT<char> buffer;
  url::EncodeURIComponent(base::UTF16ToUTF8(prepopulated_text), &buffer);
  std::string output_prepopulated(buffer.data(), buffer.length());

  auto user_result = ParseJson(base::ReplaceStringPlaceholders(
      R"({
    "user": "$1",
    "share_experience_link": "$2",
    "tasks": [
        {
            "url": "https://x.com/settings/location",
            "description": "Disable attaching location information to posts"
        },
        {
            "url": "https://x.com/settings/data_sharing_with_business_partners",
            "description": "Disable sharing additional information with X’s business partners."
        },
        {
            "url": "https://x.com/settings/off_twitter_activity",
            "description": "Disable personalization based on your inferred identity"
        },
        {
            "url": "https://x.com/settings/ads_preferences",
            "description": "Disable personalized ads"
        }
    ]
})",
      {user_id, share_experience_link}, nullptr));
  const auto context = TestLoadContext(
      rule_name, "user script", "policy script", 1, std::move(user_result));
  EXPECT_TRUE(context && context->IsValid());
  EXPECT_EQ(context->GetUserId(), user_id);
  EXPECT_EQ(context->GetRuleName(), rule_name);

  const auto link = context->GetShareLink(prepopulated_text);
  EXPECT_TRUE(link);
  EXPECT_EQ(base::ReplaceStringPlaceholders(share_experience_link,
                                            {output_prepopulated}, nullptr),
            link);
}

}  // namespace psst
