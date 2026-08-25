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
    ShutdownOnDeviceSpeechModelsComponentRegistration();
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

  // Drives `ManageOnDeviceSpeechModelsComponentRegistration` against a model
  // that is installed and on disk, and asserts it registers nothing,
  // downloads nothing, and takes the model away.
  void ExpectNoRegistrationAndDeletedCopy() {
    CreateDirectory(install_dir_);
    ASSERT_TRUE(PathExists(install_dir_));
    auto* state = OnDeviceSpeechModelsState::GetInstance();
    state->SetInstallDir(install_dir_);
    ASSERT_TRUE(state->IsModelInstalled());

    EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
    EXPECT_CALL(on_demand_updater_,
                EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
        .Times(0);
    EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
        .WillRepeatedly(testing::Return(true));
    ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                    local_state_.get());
    // Reported gone before the files are, so nothing acts on a model whose
    // files are on their way out.
    EXPECT_FALSE(state->IsModelInstalled());
    ASSERT_TRUE(
        base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
  }

 private:
  base::ScopedPathOverride scoped_path_override_{
      component_updater::DIR_COMPONENT_USER};
  base::test::ScopedFeatureList feature_list_;
};

// `OnDeviceSpeechModelsComponentInstallerPolicy`, which the component
// updater calls into. `ComponentReady` reports an install landing, either
// at registration for a copy already on disk or once a download completes,
// and publishing it is what makes the model count as installed.

// Tests that a component is only a usable install once the model
// subdirectory is there. Verifying the directory rather than the file list is
// deliberate: the file list changes with every model, and a stale one would
// fail silently as "the model never installs".
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest, VerifyInstallation) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  OnDeviceSpeechModelsComponentInstallerPolicy policy(local_state_.get());
  ASSERT_TRUE(CreateDirectory(install_dir_));

  // A mis-packaged component should not report as installed.
  EXPECT_FALSE(policy.VerifyInstallation(base::DictValue(), install_dir_));

  ASSERT_TRUE(CreateDirectory(install_dir_.AppendASCII(kModelDirName)));
  EXPECT_TRUE(policy.VerifyInstallation(base::DictValue(), install_dir_));
}

// Tests that a component arriving publishes the install dir and the model dir
// derived from it.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest, ComponentReady) {
  OnDeviceSpeechModelsComponentInstallerPolicy policy(local_state_.get());

  policy.ComponentReady(base::Version("1.0.0"), install_dir_,
                        base::DictValue());

  auto* state = OnDeviceSpeechModelsState::GetInstance();
  EXPECT_EQ(state->GetInstallDir(), install_dir_);
  EXPECT_EQ(state->GetModelDir(), install_dir_.AppendASCII(kModelDirName));
}

// Tests the early return for an empty dir.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       ComponentReady_EmptyDirDoesNotRemove) {
  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_TRUE(state->IsModelInstalled());

  OnDeviceSpeechModelsComponentInstallerPolicy policy(local_state_.get());
  policy.ComponentReady(base::Version("1.0.0"), base::FilePath(),
                        base::DictValue());

  EXPECT_EQ(install_dir_, state->GetInstallDir());
  EXPECT_TRUE(state->IsModelInstalled());
}

// Tests that a download already in flight when the feature goes off does not
// land as an install.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       ComponentReady_NoInstallWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);

  OnDeviceSpeechModelsComponentInstallerPolicy policy(local_state_.get());
  policy.ComponentReady(base::Version("1.0.0"), install_dir_,
                        base::DictValue());

  EXPECT_FALSE(OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled());
}

// Tests the same for the master switch. Unregistering cannot cancel a download
// already in flight, so this is the last place to refuse it.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       ComponentReady_NoInstallWhenLocalAIDisabled) {
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  OnDeviceSpeechModelsComponentInstallerPolicy policy(local_state_.get());
  policy.ComponentReady(base::Version("1.0.0"), install_dir_,
                        base::DictValue());

  EXPECT_FALSE(OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled());
}

// `ManageOnDeviceSpeechModelsComponentRegistration`, which we call while
// components are registered at startup and again whenever the master
// switch changes. Removing the model is only ever done from here.

// Tests that the allowed branch reaches
// `MaybeRegisterOnDeviceSpeechModelsComponent`, so the component is registered
// and the download requested.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       ManageOnDeviceSpeechModelsComponentRegistration_Registers) {
  base::RunLoop run_loop;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(1)
      .WillOnce([quit = run_loop.QuitClosure()]() { quit.Run(); });

  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());
  run_loop.Run();
}

// Tests that `ManageOnDeviceSpeechModelsComponentRegistration` removes the
// model when Brave's own feature flag is off, one of the two conditions
// `IsComponentAllowed` requires.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    ManageOnDeviceSpeechModelsComponentRegistration_RemovesWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);

  ExpectNoRegistrationAndDeletedCopy();
}

// Tests that `ManageOnDeviceSpeechModelsComponentRegistration` removes the
// model when the Local AI master switch is off, the other condition
// `IsComponentAllowed` requires. Brave Origin manages this one, so it can turn
// off long after startup.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    ManageOnDeviceSpeechModelsComponentRegistration_RemovesWhenLocalAIDisabled) {
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  ExpectNoRegistrationAndDeletedCopy();
}

// Tests that the model is removed even where the component updater may not be
// used. Removing it is not an update, and --disable-component-update says
// nothing about whether the user still wants an on-device model.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    ManageOnDeviceSpeechModelsComponentRegistration_RemovesEvenWhenComponentUpdateDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdater(/*is_component_update_disabled=*/true,
                                &on_demand_updater_);

  ExpectNoRegistrationAndDeletedCopy();
}

// Tests that the pref is followed for the rest of the session rather than read
// once, so a model already registered and downloaded is taken away when the
// switch turns off later. Brave Origin resolves it asynchronously, so it can
// land well after components are registered.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    ManageOnDeviceSpeechModelsComponentRegistration_RemovesWhenLocalAIPrefChanges) {
  CreateDirectory(install_dir_);
  ASSERT_TRUE(PathExists(install_dir_));
  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_TRUE(state->IsModelInstalled());

  base::RunLoop run_loop;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .WillOnce([quit = run_loop.QuitClosure()]() { quit.Run(); });
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());
  // Wait for the registration to reach the service, so this is the settled
  // case and not the in-flight one covered below.
  run_loop.Run();

  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(1)
      .WillOnce(testing::Return(true));
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  EXPECT_FALSE(state->IsModelInstalled());
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !PathExists(install_dir_); }));

  // Shutting down drops the references the registrar holds, which is what
  // keeps them from dangling once the browser process tears down. The switch
  // is not followed after that.
  testing::Mock::VerifyAndClearExpectations(cus_.get());
  ShutdownOnDeviceSpeechModelsComponentRegistration();
  state->SetInstallDir(install_dir_);
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, true);
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  EXPECT_TRUE(state->IsModelInstalled());
}

// `MaybeRegisterOnDeviceSpeechModelsComponent`, for every condition it
// weighs. Called directly rather than through the registrar, which reaches
// only the rows where the master switch is on and discards the outcome.

// Feature off: nothing registered. Also the row that pins the two things
// every other row relies on, that the caller is always answered from a task,
// and that refusing leaves a model already installed alone.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_NoRegisterWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);
  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_TRUE(state->IsModelInstalled());

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);

  base::test::TestFuture<update_client::Error> result;
  MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get(),
                                             result.GetCallback());
  EXPECT_FALSE(result.IsReady());
  EXPECT_EQ(update_client::Error::INVALID_ARGUMENT, result.Get());
  EXPECT_TRUE(state->IsModelInstalled());
}

// Master switch off: nothing registered. The registrar never asks for this
// row, because it removes the model instead of reaching here, so `install()`
// is the only way in.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_NoRegisterWhenLocalAIDisabled) {
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);

  base::test::TestFuture<update_client::Error> result;
  MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get(),
                                             result.GetCallback());
  EXPECT_EQ(update_client::Error::INVALID_ARGUMENT, result.Get());
}

// No update service to register with: nothing registered.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_NoRegisterWhenNoUpdateService) {
  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);

  base::test::TestFuture<update_client::Error> result;
  MaybeRegisterOnDeviceSpeechModelsComponent(nullptr, local_state_.get(),
                                             result.GetCallback());
  EXPECT_EQ(update_client::Error::INVALID_ARGUMENT, result.Get());
}

// Allowed: registered, and the download requested, with its outcome reported
// back to whoever asked.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_RegistersAndRequestsDownload) {
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
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

// Component updates turned off: still registered, because registering is what
// publishes a copy already on disk, but the download is refused. Asking for it
// is what that switch forbids, and what trips a DCHECK in BraveOnDemandUpdater.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_RegistersWithoutDownloadWhenComponentUpdateDisabled) {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdater(/*is_component_update_disabled=*/true,
                                &on_demand_updater_);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);

  base::test::TestFuture<update_client::Error> result;
  MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get(),
                                             result.GetCallback());
  EXPECT_EQ(update_client::Error::INVALID_ARGUMENT, result.Get());
}

// Switch turned off while the registration was in flight: registered, then
// taken back out here. The unregister that turning the switch off runs finds
// nothing, because the component only reaches the update service once the
// registration lands.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_UnregistersWhenSwitchedOffDuringRegistration) {
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(1)
      .WillOnce(testing::Return(true));

  base::test::TestFuture<update_client::Error> result;
  MaybeRegisterOnDeviceSpeechModelsComponent(cus_.get(), local_state_.get(),
                                             result.GetCallback());
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  EXPECT_EQ(update_client::Error::INVALID_ARGUMENT, result.Get());
}
}  // namespace local_ai
