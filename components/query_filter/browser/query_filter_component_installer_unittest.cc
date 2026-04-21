// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_component_installer.h"

#include <memory>

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
namespace {
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

}  // namespace

class QueryFilterComponentInstallerTest : public testing::Test {
 public:
  QueryFilterComponentInstallerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    cus_ = std::make_unique<component_updater::MockComponentUpdateService>();
    feature_list_.InitWithFeatures(
        {query_filter::features::kQueryFilterComponent}, {});
  }

  void TearDown() override {
    query_filter::QueryFilterData::GetInstance()->ResetRulesForTesting();
  }

 protected:
  std::unique_ptr<component_updater::MockComponentUpdateService> cus_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;

 private:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::DEFAULT,
      base::test::TaskEnvironment::ThreadPoolExecutionMode::DEFAULT};
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
  // Setup: create a temporary directory and write the sample query filter
  // JSON
  base::ScopedTempDir component_install_dir_;
  ASSERT_TRUE(component_install_dir_.CreateUniqueTempDir());
  ASSERT_TRUE(base::WriteFile(component_install_dir_.GetPath().AppendASCII(
                                  query_filter::kQueryFilterJsonFile),
                              kSampleQueryFilterJson));

  EXPECT_TRUE(component_updater::QueryFilterComponentInstallerPolicy()
                  .VerifyInstallation(base::DictValue(),
                                      component_install_dir_.GetPath()));
}

TEST_F(QueryFilterComponentInstallerTest, TestComponentReady) {
  // Setup: create a temporary directory and write the sample query filter
  // JSON
  base::ScopedTempDir component_install_dir_;
  ASSERT_TRUE(component_install_dir_.CreateUniqueTempDir());
  ASSERT_TRUE(base::WriteFile(component_install_dir_.GetPath().AppendASCII(
                                  query_filter::kQueryFilterJsonFile),
                              kSampleQueryFilterJson));

  ASSERT_NE(nullptr, query_filter::QueryFilterData::GetInstance());

  // Target version to populate with
  base::Version version("1.0.0");
  // Verify the version is empty before the component is ready
  EXPECT_EQ("", query_filter::QueryFilterData::GetInstance()->GetVersion());
  // Verify also the rules are empty before the component is ready
  EXPECT_TRUE(query_filter::QueryFilterData::GetInstance()->rules().empty());

  // Initiate component ready which would load the json file and populate the
  // query filter data.
  component_updater::QueryFilterComponentInstallerPolicy policy;
  policy.ComponentReady(version, component_install_dir_.GetPath(),
                        base::DictValue());

  // Verify the version and the rules are updated after the component is ready
  EXPECT_TRUE(base::test::RunUntil([&]() {
    return query_filter::QueryFilterData::GetInstance()->GetVersion() ==
           "1.0.0";
  }));
  EXPECT_EQ(2U, query_filter::QueryFilterData::GetInstance()->rules().size());
}
