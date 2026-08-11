/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_path_override.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

namespace {

constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("BraveOnDeviceSpeechModels");
constexpr char kComponentId[] = "nhkekccefdppopbldokibkoegppanbba";

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
    local_state_ = std::make_unique<TestingPrefServiceSimple>();
    prefs::RegisterLocalStatePrefs(local_state_->registry());
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
  std::unique_ptr<TestingPrefServiceSimple> local_state_;
  base::test::TaskEnvironment task_environment_;
  base::FilePath install_dir_;

  // Asserts that nothing is registered, nothing is downloaded, and any copy
  // already on disk is removed. This is the "switched off" half of the guard.
  void ExpectNoRegistrationAndDeletedCopy() {
    CreateDirectory(install_dir_);
    ASSERT_TRUE(PathExists(install_dir_));

    EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
    EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
        .Times(0);
    base::test::TestFuture<update_client::Error> result;
    MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get(),
                                               result.GetCallback());
    EXPECT_EQ(update_client::Error::INVALID_ARGUMENT, result.Get());
    ASSERT_TRUE(
        base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
    EXPECT_FALSE(PathExists(install_dir_));
  }

  // The same, except that a copy already on disk is left alone. This is the
  // "cannot act" half of the guard: being unable to register says nothing
  // about whether the user wants the model.
  void ExpectNoRegistrationAndPreservedCopy(
      component_updater::ComponentUpdateService* cus,
      PrefService* local_state) {
    CreateDirectory(install_dir_);
    ASSERT_TRUE(PathExists(install_dir_));

    EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
    EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
        .Times(0);
    base::test::TestFuture<update_client::Error> result;
    MaybeRegisterOnDeviceSpeechModelsComponent(cus, local_state,
                                               result.GetCallback());
    EXPECT_EQ(update_client::Error::INVALID_ARGUMENT, result.Get());

    // A delete would be posted to the thread pool, so drain it before
    // concluding that nothing removed the copy.
    task_environment_.RunUntilIdle();
    EXPECT_TRUE(PathExists(install_dir_));
  }

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
  MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get());
  run_loop.Run();
}

// Tests that ComponentReady sets up the install directory and model dir.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest, ComponentReady) {
  OnDeviceSpeechModelsComponentInstallerPolicy policy;
  policy.ComponentReady(base::Version("1.0.0"), install_dir_,
                        base::DictValue());

  auto* state = OnDeviceSpeechModelsState::GetInstance();
  EXPECT_EQ(state->GetInstallDir(), install_dir_);
  EXPECT_EQ(state->GetModelDir(), install_dir_.AppendASCII(kModelDirName));
}

// Tests that a component is only a usable install once the model
// subdirectory is there. Verifying the directory rather than the file list is
// deliberate: the file list changes with every model, and a stale one would
// fail silently as "the model never installs".
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       VerifyInstallationRequiresModelDir) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  OnDeviceSpeechModelsComponentInstallerPolicy policy;
  ASSERT_TRUE(CreateDirectory(install_dir_));

  // A mis-packaged component used to report itself installed here and only
  // fail much later at model load.
  EXPECT_FALSE(policy.VerifyInstallation(base::DictValue(), install_dir_));

  ASSERT_TRUE(CreateDirectory(install_dir_.AppendASCII(kModelDirName)));
  EXPECT_TRUE(policy.VerifyInstallation(base::DictValue(), install_dir_));
}

// Tests that an empty install dir is not treated as a component arriving.
// Removal is published by clearing the state, not by ComponentReady.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       ComponentReadyIgnoresEmptyInstallDir) {
  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_TRUE(state->IsModelInstalled());

  OnDeviceSpeechModelsComponentInstallerPolicy policy;
  policy.ComponentReady(base::Version("1.0.0"), base::FilePath(),
                        base::DictValue());

  EXPECT_EQ(install_dir_, state->GetInstallDir());
  EXPECT_TRUE(state->IsModelInstalled());
}

// Tests that an empty install dir clears the model dir.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest, EmptyInstallDirClears) {
  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_FALSE(state->GetModelDir().empty());
  ASSERT_TRUE(state->IsModelInstalled());

  state->SetInstallDir(base::FilePath());
  EXPECT_TRUE(state->GetInstallDir().empty());
  EXPECT_TRUE(state->GetModelDir().empty());
  EXPECT_FALSE(state->IsModelInstalled());
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
  MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get());
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
  base::test::TestFuture<update_client::Error> result;
  MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get(),
                                             result.GetCallback());
  // Answered from a task even on the paths that reject before registering.
  EXPECT_FALSE(result.IsReady());
  EXPECT_EQ(update_client::Error::INVALID_ARGUMENT, result.Get());
}

// Tests that the outcome of the download reaches the caller that asked for it.
// A caller with a reply parked on this has no other way to hear how it ended.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       RegisterReportsDownloadOutcome) {
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(1)
      .WillOnce(
          [](const std::string& id, component_updater::Callback callback) {
            std::move(callback).Run(update_client::Error::UPDATE_IN_PROGRESS);
          });

  base::test::TestFuture<update_client::Error> result;
  MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get(),
                                             result.GetCallback());
  EXPECT_EQ(update_client::Error::UPDATE_IN_PROGRESS, result.Get());
}

// Tests that the component is not registered when ComponentUpdateService is
// null, and that a copy already on disk survives it.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       NoRegisterWhenCUSIsNull) {
  ExpectNoRegistrationAndPreservedCopy(nullptr, local_state_.get());
}

// Tests the same for a null local state, which is also rejected before the
// switched-off clause dereferences it.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       NoRegisterWhenLocalStateIsNull) {
  ExpectNoRegistrationAndPreservedCopy(cus_.get(), nullptr);
}

// Tests that a user who turned component updates off keeps the model they
// already downloaded. Registering here would also trip a DCHECK in
// BraveOnDemandUpdater, which browser tests reach because the test launcher
// passes --disable-component-update.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       NoRegisterWhenComponentUpdateDisabled) {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdater(/*is_component_update_disabled=*/true,
                                &on_demand_updater_);

  ExpectNoRegistrationAndPreservedCopy(cus_.get(), local_state_.get());
}

// Tests that the Local AI umbrella deregisters and deletes the model too.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       NoRegisterWhenLocalAIDisabled) {
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  ExpectNoRegistrationAndDeletedCopy();
}

// Tests that being switched off stops the model being reported as installed,
// rather than leaving the state pointing at files on their way out. The delete
// is posted, so the state cannot wait for the files to actually be gone.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       SwitchedOffStopsReportingInstalled) {
  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_TRUE(state->IsModelInstalled());

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);
  MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get());

  EXPECT_FALSE(state->IsModelInstalled());
}

// Tests that being unable to act wins over being switched off, so a user who
// turned component updates off keeps the model even once the feature is gone.
// Reversing the two clauses passes every test above and silently deletes it.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       SwitchedOffDoesNotDeleteWhenComponentUpdateDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdater(/*is_component_update_disabled=*/true,
                                &on_demand_updater_);

  ExpectNoRegistrationAndPreservedCopy(cus_.get(), local_state_.get());
}

}  // namespace local_ai
