// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/rule_data_reader.h"

#include <optional>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace psst {

namespace {
std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}
}  // namespace

class RuleDataReaderUnitTest : public testing::Test {
 public:
  void SetUp() override {
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir_base_ = test_data_dir.AppendASCII("psst-component-data");

    auto psst_rules_content = ReadFile(test_data_dir_base_.Append("psst.json"));
    ASSERT_FALSE(psst_rules_content.empty());

    psst_rules_ = PsstRule::ParseRules(psst_rules_content);
    ASSERT_EQ(psst_rules_->size(), 2U);
  }

  base::FilePath GetBasePath() { return test_data_dir_base_; }

  MatchedRule GetMatchedRule() {
    return MatchedRule("a", "user.js", "policy.js", 1);
  }

  PsstRule& GetBasicRule() { return (*psst_rules_)[0]; }
  PsstRule& GetWrongNameRule() { return (*psst_rules_)[1]; }

 private:
  base::FilePath test_data_dir_base_;
  std::optional<std::vector<PsstRule>> psst_rules_;
};

TEST_F(RuleDataReaderUnitTest, LoadComponentScripts) {
  RuleDataReader crr(GetBasePath());
  auto scripts_path = GetBasePath().Append("scripts").Append("a");

  auto user_script = crr.ReadUserScript(GetBasicRule());
  ASSERT_TRUE(user_script);
  ASSERT_FALSE(user_script->empty());
  EXPECT_EQ(*user_script, ReadFile(scripts_path.Append("user.js")));

  auto policy_script = crr.ReadPolicyScript(GetBasicRule());
  ASSERT_TRUE(policy_script);
  ASSERT_FALSE(policy_script->empty());
  EXPECT_EQ(*policy_script, ReadFile(scripts_path.Append("policy.js")));
}

TEST_F(RuleDataReaderUnitTest, TryToLoadWrongWithComponentScriptPath) {
  RuleDataReader crr(GetBasePath());

  auto user_script = crr.ReadUserScript(GetWrongNameRule());
  ASSERT_FALSE(user_script);

  auto policy_script = crr.ReadPolicyScript(GetWrongNameRule());
  ASSERT_FALSE(policy_script);

}

}  // namespace psst
