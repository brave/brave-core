// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/matched_rule.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/psst/browser/core/rule_data_reader.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {

constexpr char kPsstJsonFileContent[] = R"([
        {
            "name": "a",
            "include": [
                "https://a.com/*"
            ],
            "exclude": [
            ],
            "version": 1,
            "user_script": "user.js",
            "policy_script": "policy.js"
        }
    ])";

constexpr char kPsstJsonFileNoUserScriptContent[] = R"([
        {
            "name": "a",
            "include": [
                "https://a.com/*"
            ],
            "exclude": [
            ],
            "version": 1,
            "user_script": "",
            "policy_script": "policy.js"
        }
    ])";

constexpr char kPsstJsonFileNoPolicyScriptContent[] = R"([
        {
            "name": "a",
            "include": [
                "https://a.com/*"
            ],
            "exclude": [
            ],
            "version": 1,
            "user_script": "user.js",
            "policy_script": ""
        }
    ])";

std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}
}  // namespace

class MatchedRuleTest : public testing::Test {
 public:
  void SetUp() override {
    base::FilePath test_data_dir(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    test_data_dir_base_ = test_data_dir.AppendASCII("psst-component-data");
  }

  const base::FilePath& GetBasePath() { return test_data_dir_base_; }

  const base::FilePath GetScriptsPath() {
    return GetBasePath()
        .Append(base::FilePath::FromUTF8Unsafe("scripts"))
        .Append(base::FilePath::FromUTF8Unsafe("a"));
  }

 private:
  base::FilePath test_data_dir_base_;
};

TEST_F(MatchedRuleTest, LoadSimpleMatchedRule) {
  const auto psst_rules = PsstRule::ParseRules(kPsstJsonFileContent);
  ASSERT_EQ(psst_rules->size(), 1u);

  auto matched_rule = MatchedRule::Create(
      std::make_unique<RuleDataReader>(GetBasePath()), psst_rules->front());

  EXPECT_TRUE(matched_rule.has_value());

  const auto user_script = matched_rule->UserScript();
  ASSERT_FALSE(user_script.empty());
  EXPECT_EQ(user_script, ReadFile(GetScriptsPath().Append(
                             base::FilePath::FromUTF8Unsafe("user.js"))));
  const auto policy_script = matched_rule->PolicyScript();
  ASSERT_FALSE(policy_script.empty());
  EXPECT_EQ(policy_script, ReadFile(GetScriptsPath().Append(
                               base::FilePath::FromUTF8Unsafe("policy.js"))));
}

TEST_F(MatchedRuleTest, TryToLoadMatchedRuleWithNoUserOrPolicyScript) {
  const auto psst_rules_no_user_script =
      PsstRule::ParseRules(kPsstJsonFileNoUserScriptContent);
  ASSERT_EQ(psst_rules_no_user_script->size(), 1u);
  EXPECT_TRUE(psst_rules_no_user_script->front().UserScriptPath().empty());
  EXPECT_FALSE(psst_rules_no_user_script->front().PolicyScriptPath().empty());

  auto matched_rule_no_user_script =
      MatchedRule::Create(std::make_unique<RuleDataReader>(GetBasePath()),
                          psst_rules_no_user_script->front());
  EXPECT_FALSE(matched_rule_no_user_script.has_value());

  const auto psst_rules_no_policy_script =
      PsstRule::ParseRules(kPsstJsonFileNoPolicyScriptContent);
  ASSERT_EQ(psst_rules_no_policy_script->size(), 1u);
  EXPECT_FALSE(psst_rules_no_policy_script->front().UserScriptPath().empty());
  EXPECT_TRUE(psst_rules_no_policy_script->front().PolicyScriptPath().empty());

  auto matched_rule_no_policy_script =
      MatchedRule::Create(std::make_unique<RuleDataReader>(GetBasePath()),
                          psst_rules_no_policy_script->front());
  EXPECT_FALSE(matched_rule_no_policy_script.has_value());
}

}  // namespace psst
