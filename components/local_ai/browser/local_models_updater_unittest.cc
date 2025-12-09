/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/browser/local_models_updater.h"

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_path_override.h"
#include "base/test/task_environment.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/local_ai/common/features.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

namespace {
constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("BraveLocalAIModels");
constexpr char kComponentId[] = "ejhejjmaoaohpghnblcdcjilndkangfe";
}  // namespace

class LocalModelsUpdaterUnitTest : public testing::Test {
 public:
  LocalModelsUpdaterUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    feature_list_.InitWithFeatures({features::kLocalAIModels}, {});
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

  bool CreateDirectory(const base::FilePath& dir_path) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::CreateDirectory(dir_path);
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

// Tests that the component is registered when the feature is enabled.
TEST_F(LocalModelsUpdaterUnitTest, Register) {
  base::RunLoop run_loop;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(1)
      .WillOnce([quit = run_loop.QuitClosure()]() { quit.Run(); });
  ManageLocalModelsComponentRegistration(cus_.get());
  run_loop.Run();
}

// Tests that ComponentReady sets up the install directory and model paths.
TEST_F(LocalModelsUpdaterUnitTest, ComponentReady) {
  LocalModelsComponentInstallerPolicy policy;
  policy.ComponentReady(base::Version("1.0.0"), install_dir_,
                        base::Value::Dict());

  auto* state = LocalModelsUpdaterState::GetInstance();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return state->GetInstallDir() == install_dir_; }));
  EXPECT_EQ(state->GetInstallDir(), install_dir_);

  // Verify embeddinggemma paths are set correctly
  base::FilePath expected_model_dir =
      install_dir_.AppendASCII(kEmbeddingGemmaModelDir);
  EXPECT_EQ(state->GetEmbeddingGemmaModelDir(), expected_model_dir);

  base::FilePath expected_model =
      expected_model_dir.AppendASCII(kEmbeddingGemmaModelFile);
  EXPECT_EQ(state->GetEmbeddingGemmaModel(), expected_model);

  base::FilePath expected_config =
      expected_model_dir.AppendASCII(kEmbeddingGemmaConfigFile);
  EXPECT_EQ(state->GetEmbeddingGemmaConfig(), expected_config);

  base::FilePath expected_tokenizer =
      expected_model_dir.AppendASCII(kEmbeddingGemmaTokenizerFile);
  EXPECT_EQ(state->GetEmbeddingGemmaTokenizer(), expected_tokenizer);
}

// Tests that the component directory is deleted when the feature is disabled.
TEST_F(LocalModelsUpdaterUnitTest, DeleteComponent) {
  CreateDirectory(install_dir_);
  EXPECT_TRUE(PathExists(install_dir_));

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kLocalAIModels);
  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  ManageLocalModelsComponentRegistration(cus_.get());
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
  EXPECT_FALSE(PathExists(install_dir_));
}

// Tests that the component is not registered when the feature is disabled.
TEST_F(LocalModelsUpdaterUnitTest, NoRegisterWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kLocalAIModels);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  ManageLocalModelsComponentRegistration(cus_.get());
}

// Tests that the component is not registered when ComponentUpdateService is
// null.
TEST_F(LocalModelsUpdaterUnitTest, NoRegisterWhenCUSIsNull) {
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  ManageLocalModelsComponentRegistration(nullptr);
}

}  // namespace local_ai
