// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_rule_registry.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/rule_data_reader.h"
#include "brave/components/psst/common/features.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace psst {

namespace {
// Test PSST rules file: brave/test/data/psst-component-data/psst.json
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
    scoped_feature_list_.InitAndEnableFeature(features::kEnablePsst);

    base::FilePath test_data_dir(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    test_data_dir_base_ = test_data_dir.AppendASCII("psst-component-data");
  }

  using LoadRulesTestCallback = base::MockCallback<base::OnceCallback<
      void(const std::string& data, const std::vector<PsstRule>& rules)>>;
  using CheckIfMatchTestCallback = base::MockCallback<
      base::OnceCallback<void(const std::optional<MatchedRule>&)>>;

  base::FilePath GetTestDataDirBase() const { return test_data_dir_base_; }
  base::FilePath GetScriptsTestDataDir() const {
    return GetTestDataDirBase().Append(
        base::FilePath::FromUTF8Unsafe("scripts"));
  }
  base::FilePath GetBrokenTestDataDirBase() const {
    return GetTestDataDirBase().Append(
        base::FilePath::FromUTF8Unsafe("wrong_psst"));
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  base::FilePath test_data_dir_base_;
};

TEST_F(PsstRuleRegistryUnitTest, LoadConcreteRule) {
  PsstRuleRegistry registry;
  {
    LoadRulesTestCallback mock_callback;
    registry.SetOnLoadCallbackForTest(mock_callback.Get());

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

    registry.LoadRules(GetTestDataDirBase());
    run_loop.Run();
  }

  const auto scripts_path =
      GetScriptsTestDataDir().Append(base::FilePath::FromUTF8Unsafe("a"));

  base::RunLoop run_loop;
  CheckIfMatchTestCallback mock_callback;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce([&](const std::optional<MatchedRule>& matched_rule) {
        ASSERT_TRUE(matched_rule);
        EXPECT_EQ(matched_rule->Name(), "a");
        EXPECT_EQ(matched_rule->UserScript(),
                  ReadFile(scripts_path.Append(
                      base::FilePath::FromUTF8Unsafe(kPsstUserScriptName))));
        EXPECT_EQ(matched_rule->PolicyScript(),
                  ReadFile(scripts_path.Append(
                      base::FilePath::FromUTF8Unsafe(kPsstPolicyScriptName))));
        run_loop.Quit();
      });

  registry.CheckIfMatch(GURL("https://a.com"), mock_callback.Get());
  run_loop.Run();
  ASSERT_EQ(registry.component_path_, GetTestDataDirBase());
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoading) {
  PsstRuleRegistry registry;
  LoadRulesTestCallback mock_callback;
  registry.SetOnLoadCallbackForTest(mock_callback.Get());

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

  registry.LoadRules(GetTestDataDirBase());
  run_loop.Run();
  ASSERT_EQ(registry.component_path_, GetTestDataDirBase());
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoadingEmptyPath) {
  PsstRuleRegistry registry;
  LoadRulesTestCallback mock_callback;
  registry.SetOnLoadCallbackForTest(mock_callback.Get());

  base::RunLoop run_loop;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce(
          [&](const std::string& data, const std::vector<PsstRule>& rules) {
            EXPECT_TRUE(rules.empty());
            EXPECT_TRUE(data.empty());
            run_loop.Quit();
          });

  registry.LoadRules(base::FilePath(FILE_PATH_LITERAL("")));
  run_loop.Run();
  ASSERT_TRUE(registry.component_path_.empty());
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoadingBrokenRulesFile) {
  PsstRuleRegistry registry;
  LoadRulesTestCallback mock_callback;
  registry.SetOnLoadCallbackForTest(mock_callback.Get());

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

  registry.LoadRules(GetBrokenTestDataDirBase());
  run_loop.Run();
  ASSERT_FALSE(registry.component_path_.empty());
}

TEST_F(PsstRuleRegistryUnitTest, RulesLoadingNonExistingPath) {
  const auto non_existing_path =
      base::FilePath(FILE_PATH_LITERAL("non-existing-path"));
  PsstRuleRegistry registry;
  LoadRulesTestCallback mock_callback;
  registry.SetOnLoadCallbackForTest(mock_callback.Get());

  base::RunLoop run_loop;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce(
          [&](const std::string& data, const std::vector<PsstRule>& rules) {
            EXPECT_TRUE(rules.empty());
            EXPECT_TRUE(data.empty());
            run_loop.Quit();
          });

  registry.LoadRules(non_existing_path);
  run_loop.Run();
  EXPECT_EQ(non_existing_path, registry.component_path_);
}

TEST_F(PsstRuleRegistryUnitTest, RuleReferencesToNotExistedPath) {
  PsstRuleRegistry registry;
  {
    LoadRulesTestCallback mock_callback;
    registry.SetOnLoadCallbackForTest(mock_callback.Get());

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

    registry.LoadRules(GetTestDataDirBase());
    run_loop.Run();
  }

  base::RunLoop run_loop;
  CheckIfMatchTestCallback mock_callback;
  EXPECT_CALL(mock_callback, Run)
      .Times(1)
      .WillOnce([&](const std::optional<MatchedRule>& matched_rule) {
        // Rule has not been loaded correctly(wrong scripts path), so it should
        // not be matched.
        ASSERT_FALSE(matched_rule);
        run_loop.Quit();
      });

  registry.CheckIfMatch(GURL("https://url.com"), mock_callback.Get());
  run_loop.Run();
  ASSERT_EQ(registry.component_path_, GetTestDataDirBase());
}

TEST_F(PsstRuleRegistryUnitTest, DoNotMatchRuleIfNotExists) {
  PsstRuleRegistry registry;
  {
    LoadRulesTestCallback mock_callback;
    registry.SetOnLoadCallbackForTest(mock_callback.Get());

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

    registry.LoadRules(GetTestDataDirBase());
    run_loop.Run();
  }

  CheckIfMatchTestCallback mock_callback;
  EXPECT_CALL(mock_callback, Run).Times(0);
  registry.CheckIfMatch(GURL("https://notexisted.com"), mock_callback.Get());
  ASSERT_EQ(registry.component_path_, GetTestDataDirBase());
}

}  // namespace psst
