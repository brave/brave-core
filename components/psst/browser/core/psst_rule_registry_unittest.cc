// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_rule_registry.h"

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
std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}
}  // namespace

class MockRuleDataReader : public RuleDataReader {
 public:
  explicit MockRuleDataReader(const base::FilePath& component_path);
  ~MockRuleDataReader() override = default;

  MOCK_METHOD(std::optional<std::string>,
              ReadUserScript,
              (const PsstRule& rule),
              (override, const));
  MOCK_METHOD(std::optional<std::string>,
              ReadPolicyScript,
              (const PsstRule& rule),
              (override, const));
};
MockRuleDataReader::MockRuleDataReader(const base::FilePath& component_path)
    : RuleDataReader(component_path) {}

class MockPsstRuleRegistryImpl : public PsstRuleRegistryImpl {
 public:
  MOCK_METHOD(void, OnLoadRules, (const std::string& data), (override));
  MOCK_METHOD(std::unique_ptr<RuleDataReader>,
              CreateRuleDataReader,
              (),
              (override));
};

using OnRuleMatchedCallback = base::RepeatingCallback<void(
    const std::optional<MatchedRule>& matched_rule)>;

class PsstRuleRegistryUnitTest : public testing::Test {
 public:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kBravePsst);

    base::FilePath test_data_dir(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    test_data_dir_base_ = test_data_dir.AppendASCII("psst-component-data");

    rule_data_reader_ = std::make_unique<MockRuleDataReader>(
        base::FilePath(test_data_dir_base_));
    auto psst_rul_registry = std::make_unique<MockPsstRuleRegistryImpl>();
    psst_rul_registry_ = psst_rul_registry.get();
    EXPECT_CALL(*psst_rul_registry_, CreateRuleDataReader())
        .WillRepeatedly([&]() { return std::move(rule_data_reader_); });

    PsstRuleRegistryAccessor::GetInstance()->SetRegistryForTesting(
        std::move(psst_rul_registry));
  }

  MockRuleDataReader* GetMockRuleDataReader() {
    return rule_data_reader_.get();
  }

  base::FilePath GetTestDataDirBase() const { return test_data_dir_base_; }

  MockPsstRuleRegistryImpl* GetPsstRuleRegistry() { return psst_rul_registry_; }

  void MockOnLoadRules(base::RunLoop& run_loop) {
    EXPECT_CALL(*GetPsstRuleRegistry(), OnLoadRules(testing::_))
        .WillOnce(testing::Invoke([&](const std::string& data) {
          EXPECT_EQ(data, ReadFile(GetTestDataDirBase().Append(
                              base::FilePath::FromUTF8Unsafe("psst.json"))));
          (*GetPsstRuleRegistry()).PsstRuleRegistryImpl::OnLoadRules(data);
          run_loop.Quit();
        }));
  }

  void TestMatchedRuleLoading(const GURL& url,
                              const base::FilePath& component_path,
                              std::optional<std::string> user_script,
                              std::optional<std::string> policy_script,
                              OnRuleMatchedCallback on_rule_matched_callback) {
    auto* rule_data_reader = GetMockRuleDataReader();
    {
      base::RunLoop run_loop;
      EXPECT_CALL(*rule_data_reader, ReadUserScript)
          .Times(1)
          .WillOnce(testing::Invoke(
              [&](const PsstRule& rule) { return user_script; }));

      EXPECT_CALL(*rule_data_reader, ReadPolicyScript)
          .Times(1)
          .WillOnce(testing::Invoke(
              [&](const PsstRule& rule) { return policy_script; }));
      MockOnLoadRules(run_loop);
      PsstRuleRegistryAccessor::GetInstance()->Registry()->LoadRules(
          component_path);
      run_loop.Run();
    }
    base::RunLoop run_loop;
    base::MockCallback<
        base::OnceCallback<void(const std::optional<MatchedRule>&)>>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly([&](const std::optional<MatchedRule>& matched_rule) {
          on_rule_matched_callback.Run(matched_rule);
          run_loop.Quit();
        });

    PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
        url, mock_callback.Get());
    run_loop.Run();
  }

  void TestRuleLoading(const base::FilePath& component_path) {
    auto* rule_data_reader = GetMockRuleDataReader();
    base::RunLoop run_loop;
    EXPECT_CALL(*rule_data_reader, ReadUserScript).Times(0);
    EXPECT_CALL(*rule_data_reader, ReadPolicyScript).Times(0);
    EXPECT_CALL(*GetPsstRuleRegistry(), OnLoadRules(testing::_)).Times(0);
    PsstRuleRegistryAccessor::GetInstance()->Registry()->LoadRules(
        component_path);
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  base::FilePath test_data_dir_base_;
  std::unique_ptr<MockRuleDataReader> rule_data_reader_;
  raw_ptr<MockPsstRuleRegistryImpl> psst_rul_registry_{nullptr};
};

TEST_F(PsstRuleRegistryUnitTest, SimpleLoadRules) {
  TestMatchedRuleLoading(
      GURL("https://a.com"), GetTestDataDirBase(), "It is user script",
      "It is policy script",
      base::BindRepeating([](const std::optional<MatchedRule>& matched_rule) {
        ASSERT_TRUE(matched_rule);
        EXPECT_EQ(matched_rule->Name(), "a");
      }));
}

TEST_F(PsstRuleRegistryUnitTest, NoRulesLoaded) {
  TestMatchedRuleLoading(
      GURL("https://url.com"), GetTestDataDirBase(), std::nullopt, std::nullopt,
      base::BindRepeating([](const std::optional<MatchedRule>& matched_rule) {
        ASSERT_FALSE(matched_rule);
      }));
}

TEST_F(PsstRuleRegistryUnitTest, ComponentPathEmptyNoRulesLoaded) {
  TestRuleLoading(base::FilePath("") /* empty path */);
  TestRuleLoading(base::FilePath("non-existing-path") /* non-existing path */);
}

}  // namespace psst
