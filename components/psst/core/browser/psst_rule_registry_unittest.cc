// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cstddef>
#include <memory>
#include <optional>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback_forward.h"
#include "base/path_service.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/psst/core/browser/matched_rule.h"
#include "brave/components/psst/core/browser/psst_rule_registry_impl.h"
#include "brave/components/psst/core/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace psst {

namespace {
// Test PSST rules file: brave/components/test/data/psst/psst.json
constexpr size_t kTestPsstRulesCount = 3;
constexpr char kPsstUserScriptName[] = "user.js";
constexpr char kPsstPolicyScriptName[] = "policy.js";
constexpr char kPsstJsonFileName[] = "psst.json";

std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}
}  // namespace

using OnRuleMatchedCallback = base::RepeatingCallback<void(
    const std::optional<MatchedRule>& matched_rule)>;

class PsstRuleRegistryUnitTest : public testing::Test {
 public:
  void SetUp() override {
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);
    test_data_dir_base_ =
        test_data_dir.AppendASCII("brave/components/test/data/psst");
    scoped_feature_list_.InitAndEnableFeature(features::kEnablePsst);
  }

  using LoadRulesTestFuture =
      base::test::TestFuture<std::string, std::vector<PsstRule>>;
  using CheckIfMatchTestCallback = base::MockCallback<
      base::OnceCallback<void(std::unique_ptr<MatchedRule>)>>;
  using NoMatchedRulesTestCallback = base::MockCallback<base::OnceClosure>;

  base::FilePath GetTestDataDirBase() const { return test_data_dir_base_; }
  base::FilePath GetScriptsTestDataDir() const {
    return GetTestDataDirBase().Append(
        base::FilePath::FromUTF8Unsafe("scripts"));
  }
  base::FilePath GetBrokenTestDataDirBase() const {
    return GetTestDataDirBase().Append(
        base::FilePath::FromUTF8Unsafe("wrong_psst"));
  }
  PsstRuleRegistryImpl& psst_rule_registry() { return registry_; }

 private:
  PsstRuleRegistryImpl registry_;
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  base::FilePath test_data_dir_base_;
};

TEST_F(PsstRuleRegistryUnitTest, LoadConcreteRule) {
  {
    LoadRulesTestFuture load_future;
    psst_rule_registry().LoadRules(
        GetTestDataDirBase(),
        load_future
            .GetCallback<const std::string&, const std::vector<PsstRule>&>());

    EXPECT_EQ(load_future.Get<1>().size(), kTestPsstRulesCount);
    EXPECT_EQ(load_future.Get<0>(),
              ReadFile(GetTestDataDirBase().Append(
                  base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
  }

  const auto scripts_path =
      GetScriptsTestDataDir().Append(base::FilePath::FromUTF8Unsafe("a"));

  base::test::TestFuture<std::unique_ptr<MatchedRule>> match_future;
  NoMatchedRulesTestCallback no_matched_rules_mock_calback;
  EXPECT_CALL(no_matched_rules_mock_calback, Run).Times(0);

  psst_rule_registry().CheckIfMatch(GURL("https://a.test"),
                                    match_future.GetCallback(),
                                    no_matched_rules_mock_calback.Get());

  std::unique_ptr<MatchedRule> matched_rule = match_future.Take();
  ASSERT_TRUE(matched_rule);
  EXPECT_EQ(matched_rule->name(), "a");
  EXPECT_EQ(matched_rule->user_script(),
            ReadFile(scripts_path.Append(
                base::FilePath::FromUTF8Unsafe(kPsstUserScriptName))));
  EXPECT_EQ(matched_rule->policy_script(),
            ReadFile(scripts_path.Append(
                base::FilePath::FromUTF8Unsafe(kPsstPolicyScriptName))));
}

TEST_F(PsstRuleRegistryUnitTest, CheckIfMatchWithNoRulesLoaded) {
  CheckIfMatchTestCallback mock_callback;
  NoMatchedRulesTestCallback no_matched_rules_mock_calback;
  EXPECT_CALL(mock_callback, Run).Times(0);
  EXPECT_CALL(no_matched_rules_mock_calback, Run).Times(1);

  psst_rule_registry().CheckIfMatch(GURL("https://a.test"), mock_callback.Get(),
                                    no_matched_rules_mock_calback.Get());
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoading) {
  LoadRulesTestFuture future;
  psst_rule_registry().LoadRules(
      GetTestDataDirBase(),
      future.GetCallback<const std::string&, const std::vector<PsstRule>&>());

  EXPECT_EQ(future.Get<1>().size(), kTestPsstRulesCount);
  EXPECT_EQ(future.Get<0>(),
            ReadFile(GetTestDataDirBase().Append(
                base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoadingEmptyPath) {
  LoadRulesTestFuture future;
  psst_rule_registry().LoadRules(
      base::FilePath(FILE_PATH_LITERAL("")),
      future.GetCallback<const std::string&, const std::vector<PsstRule>&>());

  EXPECT_TRUE(future.Get<1>().empty());
  EXPECT_TRUE(future.Get<0>().empty());
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoadingBrokenRulesFile) {
  LoadRulesTestFuture future;
  psst_rule_registry().LoadRules(
      GetBrokenTestDataDirBase(),
      future.GetCallback<const std::string&, const std::vector<PsstRule>&>());

  EXPECT_TRUE(future.Get<1>().empty());
  EXPECT_EQ(future.Get<0>(),
            ReadFile(GetBrokenTestDataDirBase().Append(
                base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoadingNonExistingPath) {
  const auto non_existing_path =
      base::FilePath(FILE_PATH_LITERAL("non-existing-path"));
  LoadRulesTestFuture future;
  psst_rule_registry().LoadRules(
      non_existing_path,
      future.GetCallback<const std::string&, const std::vector<PsstRule>&>());

  EXPECT_TRUE(future.Get<1>().empty());
  EXPECT_TRUE(future.Get<0>().empty());
}

TEST_F(PsstRuleRegistryUnitTest, RuleReferencesToNotExistedPath) {
  {
    LoadRulesTestFuture load_future;
    psst_rule_registry().LoadRules(
        GetTestDataDirBase(),
        load_future
            .GetCallback<const std::string&, const std::vector<PsstRule>&>());

    EXPECT_EQ(load_future.Get<1>().size(), kTestPsstRulesCount);
    EXPECT_EQ(load_future.Get<0>(),
              ReadFile(GetTestDataDirBase().Append(
                  base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
  }

  base::test::TestFuture<std::unique_ptr<MatchedRule>> match_future;
  NoMatchedRulesTestCallback no_matched_rules_mock_calback;
  EXPECT_CALL(no_matched_rules_mock_calback, Run).Times(0);

  psst_rule_registry().CheckIfMatch(GURL("https://url.test"),
                                    match_future.GetCallback(),
                                    no_matched_rules_mock_calback.Get());

  // Rule has not been loaded correctly(wrong scripts path), so it should
  // not be matched.
  ASSERT_FALSE(match_future.Take());
}

TEST_F(PsstRuleRegistryUnitTest, DoNotMatchRuleIfNotExists) {
  {
    LoadRulesTestFuture load_future;
    psst_rule_registry().LoadRules(
        GetTestDataDirBase(),
        load_future
            .GetCallback<const std::string&, const std::vector<PsstRule>&>());

    EXPECT_EQ(load_future.Get<1>().size(), kTestPsstRulesCount);
    EXPECT_EQ(load_future.Get<0>(),
              ReadFile(GetTestDataDirBase().Append(
                  base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
  }

  CheckIfMatchTestCallback mock_callback;
  NoMatchedRulesTestCallback no_matched_rules_mock_calback;
  EXPECT_CALL(no_matched_rules_mock_calback, Run).Times(1);
  EXPECT_CALL(mock_callback, Run).Times(0);
  psst_rule_registry().CheckIfMatch(GURL("https://notexisted.test"),
                                    mock_callback.Get(),
                                    no_matched_rules_mock_calback.Get());
}

}  // namespace psst
