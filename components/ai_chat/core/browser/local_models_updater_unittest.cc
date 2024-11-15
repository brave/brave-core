/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/local_models_updater.h"

#include <memory>
#include <string>
#include <string_view>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_path_override.h"
#include "base/test/task_environment.h"
#include "base/threading/thread_restrictions.h"
#include "base/version.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("AIChatLocalModels");
constexpr base::FilePath::CharType kDeprecatedComponentInstallDir[] =
    FILE_PATH_LITERAL("LeoLocalModels");
constexpr const char kComponentId[] = "ejhejjmaoaohpghnblcdcjilndkangfe";
}  // namespace

class LocalModelsUpdaterUnitTest : public testing::Test {
 public:
  LocalModelsUpdaterUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    feature_list_.InitWithFeatures(
        {ai_chat::features::kAIChat, ai_chat::features::kPageContentRefine},
        {});
  }

  ~LocalModelsUpdaterUnitTest() override = default;

  void SetUp() override {
    cus_ = std::make_unique<component_updater::MockComponentUpdateService>();
    auto component_dir =
        base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);
    install_dir_ = component_dir.Append(kComponentInstallDir);
  }

  bool PathExists(const base::FilePath& file_path) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::PathExists(file_path);
  }

 protected:
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
  std::unique_ptr<component_updater::MockComponentUpdateService> cus_;
  base::test::TaskEnvironment task_environment_;
  base::FilePath install_dir_;

 private:
  base::ScopedPathOverride scoped_path_override_{
      component_updater::DIR_COMPONENT_USER};
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(LocalModelsUpdaterUnitTest, Register) {
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(1);
  ai_chat::ManageLocalModelsComponentRegistration(cus_.get());
  task_environment_.RunUntilIdle();
}

TEST_F(LocalModelsUpdaterUnitTest, ComponentReady) {
  auto policy =
      std::make_unique<ai_chat::LocalModelsComponentInstallerPolicy>();
  policy->ComponentReadyForTesting(base::Version("1.0.0"), install_dir_, {});
  EXPECT_EQ(ai_chat::LocalModelsUpdaterState::GetInstance()->GetInstallDir(),
            install_dir_);
}

TEST_F(LocalModelsUpdaterUnitTest, DeleteComponent) {
  for (const auto& disable_feature : std::vector<base::test::FeatureRef>(
           {ai_chat::features::kAIChat,
            ai_chat::features::kPageContentRefine})) {
    SCOPED_TRACE(disable_feature->name);
    base::CreateDirectory(install_dir_);
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndDisableFeature(*disable_feature);
    EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
    EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
        .Times(0);
    ai_chat::ManageLocalModelsComponentRegistration(cus_.get());
    EXPECT_FALSE(PathExists(install_dir_));
    task_environment_.RunUntilIdle();
  }
}

TEST_F(LocalModelsUpdaterUnitTest, DeprecatedComponentDir) {
  auto component_dir =
    base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);
  auto old_install_dir = component_dir.Append(kDeprecatedComponentInstallDir);
  base::CreateDirectory(old_install_dir);
  ai_chat::ManageLocalModelsComponentRegistration(cus_.get());
  EXPECT_FALSE(PathExists(old_install_dir));
  task_environment_.RunUntilIdle();
}
