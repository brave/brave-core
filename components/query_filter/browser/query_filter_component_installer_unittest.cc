// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_component_installer.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/query_filter/common/features.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/update_client/update_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace query_filter {

inline constexpr char kQueryFilterComponentId[] =
    "cemdlagocoimleflkfkjoihojfainiho";

class QueryFilterComponentInstallerTest : public testing::Test {
 public:
  QueryFilterComponentInstallerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    feature_list_.InitWithFeatures({features::kQueryFilterComponent}, {});
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
  std::unique_ptr<component_updater::MockComponentUpdateService> cus_ =
      std::make_unique<component_updater::MockComponentUpdateService>();

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(QueryFilterComponentInstallerTest,
       RegistersOnDemandInstallForComponentId) {
  base::test::TestFuture<void> future;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kQueryFilterComponentId, testing::_))
      .Times(1)
      .WillOnce([&future](const std::string& /*id*/,
                          component_updater::Callback callback) {
        std::move(callback).Run(update_client::Error::NONE);
        future.SetValue();
      });

  RegisterQueryFilterComponent(cus_.get());
  future.Get();
}

TEST_F(QueryFilterComponentInstallerTest, NoRegisterWhenFeatureDisabled) {
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(features::kQueryFilterComponent);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kQueryFilterComponentId, testing::_))
      .Times(0);

  RegisterQueryFilterComponent(cus_.get());
}

TEST_F(QueryFilterComponentInstallerTest, NoRegisterWhenCUSIsNull) {
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kQueryFilterComponentId, testing::_))
      .Times(0);

  RegisterQueryFilterComponent(nullptr);
}

}  // namespace query_filter
