/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"

#include <memory>

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
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

namespace {

constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("BraveOnDeviceSpeechModels");
constexpr char kComponentId[] = "nhkekccefdppopbldokibkoegppanbba";
// Model subdirectory within the install dir. Kept in sync with the
// `kModelDir` constant in on_device_speech_models_state.cc.
constexpr char kModelDir[] = "nemotron-speech-streaming-en-0.6b-int4-onnx";

}  // namespace

class OnDeviceSpeechModelsComponentInstallerUnitTest : public testing::Test {
 public:
  OnDeviceSpeechModelsComponentInstallerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    feature_list_.InitAndEnableFeature(kBraveOnDeviceSpeechRecognition);
  }

  ~OnDeviceSpeechModelsComponentInstallerUnitTest() override = default;

  void SetUp() override {
    cus_ = std::make_unique<component_updater::MockComponentUpdateService>();
    auto component_dir =
        base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);
    install_dir_ = component_dir.Append(kComponentInstallDir);
  }

  void TearDown() override {
    // Clear singleton state to avoid polluting other test suites.
    OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(base::FilePath());
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

// Tests that the component is registered and an on-demand install requested
// when the feature is enabled.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest, Register) {
  base::RunLoop run_loop;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(1)
      .WillOnce([quit = run_loop.QuitClosure()]() { quit.Run(); });
  RegisterOnDeviceSpeechModelsComponent(cus_.get());
  run_loop.Run();
}

// Tests that ComponentReady sets up the install directory and model dir.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest, ComponentReady) {
  OnDeviceSpeechModelsComponentInstallerPolicy policy;
  policy.ComponentReady(base::Version("1.0.0"), install_dir_,
                        base::DictValue());

  auto* state = OnDeviceSpeechModelsState::GetInstance();
  EXPECT_EQ(state->GetInstallDir(), install_dir_);
  EXPECT_EQ(state->GetModelDir(), install_dir_.AppendASCII(kModelDir));
}

// Tests that an empty install dir clears the model dir.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest, EmptyInstallDirClears) {
  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_FALSE(state->GetModelDir().empty());

  state->SetInstallDir(base::FilePath());
  EXPECT_TRUE(state->GetInstallDir().empty());
  EXPECT_TRUE(state->GetModelDir().empty());
}

// Tests that the component directory is deleted when the feature is disabled.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest, DeleteComponent) {
  CreateDirectory(install_dir_);
  EXPECT_TRUE(PathExists(install_dir_));

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);
  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  RegisterOnDeviceSpeechModelsComponent(cus_.get());
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
  EXPECT_FALSE(PathExists(install_dir_));
}

// Tests that the component is not registered when the feature is disabled.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       NoRegisterWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  RegisterOnDeviceSpeechModelsComponent(cus_.get());
}

// Tests that the component is not registered when ComponentUpdateService is
// null.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       NoRegisterWhenCUSIsNull) {
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  RegisterOnDeviceSpeechModelsComponent(nullptr);
}

}  // namespace local_ai
