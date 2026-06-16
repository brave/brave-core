// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/rule_data_reader.h"

#include <optional>
#include <string>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {

constexpr base::FilePath::StringViewType kUserScriptPath =
    FILE_PATH_LITERAL("user.js");
constexpr base::FilePath::StringViewType kPolicyScriptPath =
    FILE_PATH_LITERAL("policy.js");

constexpr char kExistingRuleName[] = "a";
constexpr char kNotExistingRuleName[] = "rule_with_wrong_script_path";

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
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);
    test_data_dir_base_ =
        test_data_dir.AppendASCII("brave/components/test/data/psst");
  }

  base::FilePath GetBasePath() { return test_data_dir_base_; }

 private:
  base::FilePath test_data_dir_base_;
};

TEST_F(RuleDataReaderUnitTest, LoadComponentScripts) {
  RuleDataReader crr(GetBasePath());
  auto scripts_path =
      GetBasePath()
          .Append(base::FilePath::FromUTF8Unsafe("scripts"))
          .Append(base::FilePath::FromUTF8Unsafe(kExistingRuleName));

  const auto user_script = base::FilePath(kUserScriptPath);
  const auto policy_script = base::FilePath(kPolicyScriptPath);

  auto user_script_content = crr.ReadUserScript(kExistingRuleName, user_script);
  ASSERT_TRUE(user_script_content);
  ASSERT_FALSE(user_script_content->empty());
  EXPECT_EQ(*user_script_content, ReadFile(scripts_path.Append(user_script)));

  auto policy_script_content =
      crr.ReadPolicyScript(kExistingRuleName, policy_script);
  ASSERT_TRUE(policy_script_content);
  ASSERT_FALSE(policy_script_content->empty());
  EXPECT_EQ(*policy_script_content,
            ReadFile(scripts_path.Append(policy_script)));
}

TEST_F(RuleDataReaderUnitTest, TryToLoadWrongWithComponentScriptPath) {
  RuleDataReader crr(GetBasePath());

  auto user_script =
      crr.ReadUserScript(kNotExistingRuleName, base::FilePath(kUserScriptPath));
  ASSERT_FALSE(user_script);

  auto policy_script = crr.ReadPolicyScript(kNotExistingRuleName,
                                            base::FilePath(kPolicyScriptPath));
  ASSERT_FALSE(policy_script);
}

}  // namespace psst
