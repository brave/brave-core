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
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry_impl.h"
#include "brave/components/psst/common/features.h"
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

  using LoadRulesTestCallback = base::MockCallback<base::OnceCallback<
      void(const std::string& data, const std::vector<PsstRule>& rules)>>;
  using CheckIfMatchTestCallback = base::MockCallback<
      base::OnceCallback<void(std::unique_ptr<MatchedRule>)>>;

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
    LoadRulesTestCallback mock_callback;
    base::RunLoop run_loop;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillOnce([&](const std::string& data,
                      const std::vector<PsstRule>& rules) {
          EXPECT_EQ(rules.size(), kTestPsstRulesCount);
          EXPECT_EQ(data,
                    ReadFile(GetTestDataDirBase().Append(
                        base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
          run_loop.Quit();
        });

    psst_rule_registry().LoadRules(GetTestDataDirBase(), mock_callback.Get());
    run_loop.Run();
  }

  const auto scripts_path =
      GetScriptsTestDataDir().Append(base::FilePath::FromUTF8Unsafe("a"));

  base::RunLoop run_loop;
  CheckIfMatchTestCallback mock_callback;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce([&](std::unique_ptr<MatchedRule> matched_rule) {
        ASSERT_TRUE(matched_rule);
        EXPECT_EQ(matched_rule->name(), "a");
        EXPECT_EQ(matched_rule->user_script(),
                  ReadFile(scripts_path.Append(
                      base::FilePath::FromUTF8Unsafe(kPsstUserScriptName))));
        EXPECT_EQ(matched_rule->policy_script(),
                  ReadFile(scripts_path.Append(
                      base::FilePath::FromUTF8Unsafe(kPsstPolicyScriptName))));
        run_loop.Quit();
      });

  psst_rule_registry().CheckIfMatch(GURL("https://a.test"),
                                    mock_callback.Get());
  run_loop.Run();
}

TEST_F(PsstRuleRegistryUnitTest, CheckIfMatchWithNoRulesLoaded) {
  CheckIfMatchTestCallback mock_callback;
  EXPECT_CALL(mock_callback, Run).Times(0);
  psst_rule_registry().CheckIfMatch(GURL("https://a.test"),
                                    mock_callback.Get());
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoading) {
  LoadRulesTestCallback mock_callback;
  base::RunLoop run_loop;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce(
          [&](const std::string& data, const std::vector<PsstRule>& rules) {
            EXPECT_EQ(rules.size(), kTestPsstRulesCount);
            EXPECT_EQ(data,
                      ReadFile(GetTestDataDirBase().Append(
                          base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
            run_loop.Quit();
          });

  psst_rule_registry().LoadRules(GetTestDataDirBase(), mock_callback.Get());
  run_loop.Run();
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoadingEmptyPath) {
  LoadRulesTestCallback mock_callback;
  base::RunLoop run_loop;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce(
          [&](const std::string& data, const std::vector<PsstRule>& rules) {
            EXPECT_TRUE(rules.empty());
            EXPECT_TRUE(data.empty());
            run_loop.Quit();
          });

  psst_rule_registry().LoadRules(base::FilePath(FILE_PATH_LITERAL("")),
                                 mock_callback.Get());
  run_loop.Run();
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoadingBrokenRulesFile) {
  LoadRulesTestCallback mock_callback;
  base::RunLoop run_loop;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce(
          [&](const std::string& data, const std::vector<PsstRule>& rules) {
            EXPECT_TRUE(rules.empty());
            EXPECT_EQ(data,
                      ReadFile(GetBrokenTestDataDirBase().Append(
                          base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
            run_loop.Quit();
          });

  psst_rule_registry().LoadRules(GetBrokenTestDataDirBase(),
                                 mock_callback.Get());
  run_loop.Run();
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoadingNonExistingPath) {
  const auto non_existing_path =
      base::FilePath(FILE_PATH_LITERAL("non-existing-path"));
  LoadRulesTestCallback mock_callback;
  base::RunLoop run_loop;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce(
          [&](const std::string& data, const std::vector<PsstRule>& rules) {
            EXPECT_TRUE(rules.empty());
            EXPECT_TRUE(data.empty());
            run_loop.Quit();
          });

  psst_rule_registry().LoadRules(non_existing_path, mock_callback.Get());
  run_loop.Run();
}

TEST_F(PsstRuleRegistryUnitTest, RuleReferencesToNotExistedPath) {
  {
    LoadRulesTestCallback mock_callback;
    base::RunLoop run_loop;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillOnce([&](const std::string& data,
                      const std::vector<PsstRule>& rules) {
          EXPECT_EQ(rules.size(), kTestPsstRulesCount);
          EXPECT_EQ(data,
                    ReadFile(GetTestDataDirBase().Append(
                        base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
          run_loop.Quit();
        });

    psst_rule_registry().LoadRules(GetTestDataDirBase(), mock_callback.Get());
    run_loop.Run();
  }

  base::RunLoop run_loop;
  CheckIfMatchTestCallback mock_callback;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce([&](std::unique_ptr<MatchedRule> matched_rule) {
        // Rule has not been loaded correctly(wrong scripts path), so it should
        // not be matched.
        ASSERT_FALSE(matched_rule);
        run_loop.Quit();
      });

  psst_rule_registry().CheckIfMatch(GURL("https://url.test"),
                                    mock_callback.Get());
  run_loop.Run();
}

TEST_F(PsstRuleRegistryUnitTest, DoNotMatchRuleIfNotExists) {
  {
    LoadRulesTestCallback mock_callback;
    base::RunLoop run_loop;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillOnce([&](const std::string& data,
                      const std::vector<PsstRule>& rules) {
          EXPECT_EQ(rules.size(), kTestPsstRulesCount);
          EXPECT_EQ(data,
                    ReadFile(GetTestDataDirBase().Append(
                        base::FilePath::FromUTF8Unsafe(kPsstJsonFileName))));
          run_loop.Quit();
        });

    psst_rule_registry().LoadRules(GetTestDataDirBase(), mock_callback.Get());
    run_loop.Run();
  }

  CheckIfMatchTestCallback mock_callback;
  EXPECT_CALL(mock_callback, Run).Times(0);
  psst_rule_registry().CheckIfMatch(GURL("https://notexisted.test"),
                                    mock_callback.Get());
}

}  // namespace psst
