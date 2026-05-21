// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_component_installer.h"

#include <memory>
#include <string_view>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/query_filter/browser/query_filter_data.h"
#include "brave/components/query_filter/common/constants.h"
#include "brave/components/query_filter/common/features.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/update_client/update_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// Sample query filter JSON which would be written to a file during setup
// and then read by the query filter data to prepopulate the default rules.
constexpr char kSampleQueryFilterJson[] = R"json(
  [
    {
      "include": ["*://*/*"],
      "exclude": [],
      "params": ["__hsfp", "gclid", "fbclid"]
    },
    {
      "include": [
        "*://*.youtube.com/*",
        "*://youtube.com/*",
        "*://youtu.be/*"
      ],
      "exclude": [],
      "params": ["si"]
    }
  ]
  )json";

class QueryFilterComponentInstallerTest : public testing::Test {
 public:
  QueryFilterComponentInstallerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    cus_ = std::make_unique<component_updater::MockComponentUpdateService>();
    feature_list_.InitWithFeatures(
        {query_filter::features::kQueryFilterComponent}, {});

    // Create a temporary directory to write the sample query filter JSON.
    ASSERT_TRUE(component_install_dir_.CreateUniqueTempDir());
  }

  void TearDown() override {
    query_filter::QueryFilterData::GetInstance()->ResetRulesForTesting();
  }

  const std::vector<query_filter::schema::Rule>& GetQueryFilterRules() const {
    return query_filter::QueryFilterData::GetInstance()->rules();
  }

  std::string GetQueryFilterVersion() const {
    return query_filter::QueryFilterData::GetInstance()->GetVersion();
  }

  void WriteToTestFile(std::string_view contents) {
    ASSERT_TRUE(base::WriteFile(component_install_dir_.GetPath().AppendASCII(
                                    query_filter::kQueryFilterJsonFile),
                                contents));
  }

  base::FilePath GetInstallDirectoryPath() const {
    return component_install_dir_.GetPath();
  }

 protected:
  std::unique_ptr<component_updater::MockComponentUpdateService> cus_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;

 private:
  base::ScopedTempDir component_install_dir_;
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::DEFAULT,
      base::test::TaskEnvironment::ThreadPoolExecutionMode::DEFAULT};
};

class ScopedFileLoadedCallbackForTesting {
 public:
  ScopedFileLoadedCallbackForTesting(
      component_updater::QueryFilterComponentInstallerPolicy& policy,
      base::OnceClosure callback)
      : policy_(policy), callback_(std::move(callback)) {
    policy_->SetOnFileLoadedCallbackForTesting(&callback_);
  }

  ~ScopedFileLoadedCallbackForTesting() {
    policy_->SetOnFileLoadedCallbackForTesting(nullptr);
  }

  ScopedFileLoadedCallbackForTesting(
      const ScopedFileLoadedCallbackForTesting&) = delete;
  ScopedFileLoadedCallbackForTesting& operator=(
      const ScopedFileLoadedCallbackForTesting&) = delete;

 private:
  raw_ref<component_updater::QueryFilterComponentInstallerPolicy> policy_;
  base::OnceClosure callback_;
};

// Tests covering disabled feature flag state.
TEST(QueryFilterComponentInstallerFeatureOffTest,
     TestNoRegisterWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(
      query_filter::features::kQueryFilterComponent);
  auto cus = std::make_unique<component_updater::MockComponentUpdateService>();
  brave_component_updater::MockOnDemandUpdater on_demand_updater;

  EXPECT_CALL(*cus, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(
      on_demand_updater,
      EnsureInstalled(query_filter::kQueryFilterComponentId, testing::_))
      .Times(0);

  component_updater::RegisterQueryFilterComponent(cus.get());
}

// Tests covering enabled feature flag state.
TEST_F(QueryFilterComponentInstallerTest,
       RegistersOnDemandInstallForComponentId) {
  base::test::TestFuture<void> future;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(
      on_demand_updater_,
      EnsureInstalled(query_filter::kQueryFilterComponentId, testing::_))
      .Times(1)
      .WillOnce([&future](const std::string& /*id*/,
                          component_updater::Callback callback) {
        std::move(callback).Run(update_client::Error::NONE);
        future.SetValue();
      });

  RegisterQueryFilterComponent(cus_.get());
  future.Get();
}

TEST_F(QueryFilterComponentInstallerTest, NoRegisterWhenCUSIsNull) {
  EXPECT_CALL(
      on_demand_updater_,
      EnsureInstalled(query_filter::kQueryFilterComponentId, testing::_))
      .Times(0);

  component_updater::RegisterQueryFilterComponent(nullptr);
}

TEST_F(QueryFilterComponentInstallerTest, TestVerifyInstallation) {
  WriteToTestFile(kSampleQueryFilterJson);

  EXPECT_TRUE(
      component_updater::QueryFilterComponentInstallerPolicy()
          .VerifyInstallation(base::DictValue(), GetInstallDirectoryPath()));
}

TEST_F(QueryFilterComponentInstallerTest, TestComponentReady) {
  // Test setup
  WriteToTestFile(kSampleQueryFilterJson);
  ASSERT_NE(nullptr, query_filter::QueryFilterData::GetInstance());
  // Target version to populate with
  base::Version version("1.0.0");
  // Verify the version is empty before the component is ready
  EXPECT_EQ("", GetQueryFilterVersion());
  // Verify also the rules are empty before the component is ready
  const auto& rules = GetQueryFilterRules();
  EXPECT_TRUE(rules.empty());

  // Initiate component ready which would load the json file and populate the
  // query filter data.
  base::test::TestFuture<void> future;
  component_updater::QueryFilterComponentInstallerPolicy policy;
  ScopedFileLoadedCallbackForTesting scoped(policy, future.GetCallback());
  policy.ComponentReady(version, GetInstallDirectoryPath(), base::DictValue());
  ASSERT_TRUE(future.Wait());

  // Verify the version and the rules are updated after the component is ready.
  EXPECT_EQ("1.0.0", GetQueryFilterVersion());
  const auto& new_rules = GetQueryFilterRules();
  EXPECT_EQ(2U, new_rules.size());
}

TEST_F(QueryFilterComponentInstallerTest,
       TestComponentReady_WithBadJson_DoesNotUpdateVersion) {
  // Test setup
  WriteToTestFile("{bad bad json}");
  ASSERT_NE(nullptr, query_filter::QueryFilterData::GetInstance());
  // Target version to populate with
  base::Version version("1.0.0");
  // Verify the version is empty before the component is ready
  EXPECT_EQ("", GetQueryFilterVersion());
  // Verify also the rules are empty before the component is ready
  const auto& rules = GetQueryFilterRules();
  EXPECT_TRUE(rules.empty());

  // Initiate component ready which would load the json file and populate the
  // query filter data.
  base::test::TestFuture<void> future;
  component_updater::QueryFilterComponentInstallerPolicy policy;
  ScopedFileLoadedCallbackForTesting scoped(policy, future.GetCallback());
  policy.ComponentReady(version, GetInstallDirectoryPath(), base::DictValue());
  ASSERT_TRUE(future.Wait());

  // Verify neither the version not the rules are updated.
  EXPECT_EQ("", GetQueryFilterVersion());
  const auto& new_rules = GetQueryFilterRules();
  EXPECT_TRUE(new_rules.empty());
}
