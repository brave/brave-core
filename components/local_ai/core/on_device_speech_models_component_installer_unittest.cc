/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
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
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/update_client/crx_update_item.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"
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
    // The registrar watches the update service for the whole session, so hold
    // on to what it registered and let a test drive events through it.
    EXPECT_CALL(*cus_, AddObserver(testing::_))
        .Times(testing::AnyNumber())
        .WillRepeatedly([this](component_updater::ServiceObserver* observer) {
          cus_observer_ = observer;
        });
    EXPECT_CALL(*cus_, RemoveObserver(testing::_))
        .Times(testing::AnyNumber())
        .WillRepeatedly([this](component_updater::ServiceObserver* observer) {
          cus_observer_ = nullptr;
        });
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

  // Drives the registrar to where a request will park on the update service,
  // registered with a download already running. Download requests are answered
  // the way the update service answers them while one is already running,
  // which starts nothing and leaves the request waiting on
  // `ServiceObserver::OnEvent`. One registration and one download for the
  // registrar itself, plus one for each request the test makes after it,
  // because each waits for the registration before it to land.
  void StartWithADownloadRunning(int requests) {
    EXPECT_CALL(*cus_, RegisterComponent(testing::_))
        .Times(requests + 1)
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(on_demand_updater_,
                EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
        .Times(requests + 1)
        .WillRepeatedly([this](const std::string& id,
                               component_updater::Callback callback) {
          ++download_count_;
          std::move(callback).Run(update_client::Error::UPDATE_IN_PROGRESS);
        });
    ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                    local_state_.get());
    ASSERT_TRUE(WaitForDownloadRequests(1));
  }

  // A download is asked for once a registration lands, so this is how a test
  // waits for one registration to be over before asking for another. Each
  // request then starts a registration of its own rather than joining one.
  bool WaitForDownloadRequests(int count) {
    return base::test::RunUntil([&]() { return download_count_ == count; });
  }

  std::string ComponentId() {
    return std::string(kOnDeviceSpeechModelsComponentId);
  }

  // Delivered the way the update service delivers it, to whoever it
  // registered.
  void SendEvent(update_client::ComponentState state, const std::string& id) {
    update_client::CrxUpdateItem item;
    item.state = state;
    item.id = id;
    cus_observer_->OnEvent(item);
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
  raw_ptr<component_updater::ServiceObserver> cus_observer_ = nullptr;
  int download_count_ = 0;

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
    // Nothing was registered, so the files are ours to remove.
    EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
        .Times(1)
        .WillOnce(testing::Return(false));
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
  ASSERT_TRUE(CreateDirectory(install_dir_));
  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_TRUE(state->IsModelInstalled());

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
  // Wait for the registration to reach the service, so this is the settled
  // case and not the in-flight one
  // `ManageOnDeviceSpeechModelsComponentRegistration_TogglingWhileRegistrationPending`
  // covers.
  run_loop.Run();

  // The component was registered, so the update service owns removing the
  // files through `ComponentInstaller::Uninstall`. All we do is stop reporting
  // the model as installed.
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(1)
      .WillOnce(testing::Return(true));
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  EXPECT_FALSE(state->IsModelInstalled());

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

// The other direction, and the one a first run takes: the switch is off while
// components are registered, so nothing is registered and whatever was on disk
// goes, and it turns on once Brave Origin has verified the purchase.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    ManageOnDeviceSpeechModelsComponentRegistration_RegistersWhenLocalAIPrefTurnsOn) {
  ASSERT_TRUE(CreateDirectory(install_dir_));
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(1)
      .WillOnce(testing::Return(false));
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
  testing::Mock::VerifyAndClearExpectations(cus_.get());
  testing::Mock::VerifyAndClearExpectations(&on_demand_updater_);

  base::RunLoop run_loop;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(1)
      .WillOnce([quit = run_loop.QuitClosure()]() { quit.Run(); });
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, true);
  run_loop.Run();
}

// Toggling the switch while a registration is in flight must not start a
// second one. The component is absent from the update service for that whole
// window, so a second registration would replace the first and the removal the
// off-transition ran would take the files the on-transition is installing.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    ManageOnDeviceSpeechModelsComponentRegistration_TogglingWhileRegistrationPending) {
  ASSERT_TRUE(CreateDirectory(install_dir_));
  base::RunLoop run_loop;
  // Nothing is registered for the whole toggle, because the registration
  // reaches the service only once `ComponentInstaller` has read the manifest.
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(1)
      .WillOnce([quit = run_loop.QuitClosure()]() { quit.Run(); });

  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, true);
  run_loop.Run();
  EXPECT_TRUE(PathExists(install_dir_));

  // Turning the switch off removes the files on the installer's own task
  // runner, which a second registration would have been queued ahead of. So
  // seeing them go is what makes the single `RegisterComponent` expectation
  // proof that no second registration was started, rather than that none had
  // landed yet.
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
}

// Registration and uninstall share the installer's task runner, so a fresh
// instance per registration would let a queued uninstall run afterwards and
// delete what the next registration installed.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       ManageOnDeviceSpeechModelsComponentRegistration_ReusesInstaller) {
  std::vector<scoped_refptr<update_client::CrxInstaller>> installers;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .WillRepeatedly(
          [&installers](
              const component_updater::ComponentRegistration& registration) {
            installers.push_back(registration.installer);
            return true;
          });
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(testing::AnyNumber());

  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());
  ASSERT_TRUE(base::test::RunUntil([&]() { return installers.size() == 1u; }));

  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, true);
  ASSERT_TRUE(base::test::RunUntil([&]() { return installers.size() == 2u; }));

  EXPECT_EQ(installers[0], installers[1]);
}

// `MaybeRegisterOnDeviceSpeechModelsComponent`, for every condition it
// weighs. It asks the same registrar, so each of these runs it after
// `ManageOnDeviceSpeechModelsComponentRegistration` has handed over the update
// service and the local state. Every request is answered, so a test waits for
// the answer rather than for the absence of one. A model arriving is published
// as the install dir on top of that.

// Feature off: nothing registered. Also the row that pins the two things
// every other row relies on, that the answer arrives from a task, and that
// refusing leaves a model already installed alone.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_NoRegisterWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(1)
      .WillOnce(testing::Return(false));
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());

  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_TRUE(state->IsModelInstalled());

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  EXPECT_FALSE(future.IsReady());

  EXPECT_FALSE(future.Get());
  EXPECT_TRUE(state->IsModelInstalled());
}

// Master switch off: nothing registered.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_NoRegisterWhenLocalAIDisabled) {
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(1)
      .WillOnce(testing::Return(false));
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  EXPECT_FALSE(future.Get());
}

// Asked before `ManageOnDeviceSpeechModelsComponentRegistration` has run, so
// the registrar has nothing to register with yet: refused rather than left
// waiting for the session to hand it one.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       MaybeRegisterOnDeviceSpeechModelsComponent_NoRegisterBeforeStart) {
  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  EXPECT_FALSE(future.Get());
}

// No update service to register with: nothing registered.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_NoRegisterWhenNoUpdateService) {
  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);
  ManageOnDeviceSpeechModelsComponentRegistration(nullptr, local_state_.get());

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  EXPECT_FALSE(future.Get());
}

// Asking once that registration has settled: registered again, which is
// harmless because the update service replaces the registration it already
// had, and the download requested. `ComponentReady` publishes the install dir
// before the request is answered, which is what makes it a success.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_RegistersAndRequestsDownload) {
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
  testing::Mock::VerifyAndClearExpectations(cus_.get());
  testing::Mock::VerifyAndClearExpectations(&on_demand_updater_);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(1)
      .WillOnce([this](const std::string& id,
                       component_updater::Callback callback) {
        OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(install_dir_);
        std::move(callback).Run(update_client::Error::NONE);
      });

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  EXPECT_TRUE(future.Get());
  EXPECT_TRUE(OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled());
}

// Two requests made before a registration lands wait on that one registration
// and are answered together, rather than each starting one of its own.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_RequestsWaitingAreAnsweredTogether) {
  // One registration and one download for both requests, not one of each per
  // request: they join what `ManageOnDeviceSpeechModelsComponentRegistration`
  // already started.
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(1)
      .WillOnce([this](const std::string& id,
                       component_updater::Callback callback) {
        OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(install_dir_);
        std::move(callback).Run(update_client::Error::NONE);
      });
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());

  base::test::TestFuture<bool> first;
  base::test::TestFuture<bool> second;
  MaybeRegisterOnDeviceSpeechModelsComponent(first.GetCallback());
  MaybeRegisterOnDeviceSpeechModelsComponent(second.GetCallback());

  EXPECT_TRUE(first.Get());
  EXPECT_TRUE(second.Get());
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
  // The update service is watched to hear how a download went, so asking for
  // none leaves nothing to hear.
  EXPECT_CALL(*cus_, AddObserver(testing::_)).Times(0);
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  EXPECT_FALSE(future.Get());
}

// Switch turned off while the registration was in flight: registered, then
// taken back out here, and the files removed here too. Both unregisters find
// nothing, because the component only reaches the update service once the
// registration lands, so the removal is ours to finish.
TEST_F(
    OnDeviceSpeechModelsComponentInstallerUnitTest,
    MaybeRegisterOnDeviceSpeechModelsComponent_UnregistersWhenSwitchedOffDuringRegistration) {
  ASSERT_TRUE(CreateDirectory(install_dir_));
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);
  // Twice: the pref change finds a registration pending and leaves the files
  // alone, then the abandoned registration cleans up.
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(2)
      .WillRepeatedly(testing::Return(false));
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  EXPECT_FALSE(future.Get());
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
}

// Switch turned off and back on while a registration was pending: finished by
// the registration that lands, which succeeds. Reporting a failure when it
// turned off would fail an install the on transition then completes.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       MaybeRegisterOnDeviceSpeechModelsComponent_TogglingWhileWaiting) {
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(1)
      .WillOnce(testing::Return(false));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(1)
      .WillOnce([this](const std::string& id,
                       component_updater::Callback callback) {
        OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(install_dir_);
        std::move(callback).Run(update_client::Error::NONE);
      });
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, true);

  EXPECT_TRUE(future.Get());
  EXPECT_TRUE(OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled());
}

// Shutting down while a registration is in flight leaves the model where it
// is, and answers the request with it. Letting go of the update service is not
// what takes a model away, so a request waiting on one already here is not a
// failure either.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       MaybeRegisterOnDeviceSpeechModelsComponent_ShutdownDuringRegistration) {
  auto* state = OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(install_dir_);
  ASSERT_TRUE(state->IsModelInstalled());

  base::test::TestFuture<void> registered;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce([callback = registered.GetCallback()]() mutable {
        std::move(callback).Run();
        return true;
      });
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(0);
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(0);
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  ShutdownOnDeviceSpeechModelsComponentRegistration();

  EXPECT_TRUE(future.Get());
  // The registration still lands, on the installer the shutdown let go of.
  EXPECT_TRUE(registered.Wait());
  EXPECT_TRUE(state->IsModelInstalled());
}

// The update service, which reports how a download that was already running
// ended. A request made then starts nothing, so this is the one outcome its
// own answer cannot carry.

// Tests that every state an update cannot leave settles a request waiting on
// it, and that nothing else does. Which state it is says nothing on its own: a
// failed download, an update that landed without publishing, and a server with
// nothing to offer all leave the request without a model.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       TerminalStateSettlesARequestWaitingOnTheUpdate) {
  // One request per terminal state below, each parking on a download of its
  // own.
  StartWithADownloadRunning(/*requests=*/3);

  int downloads = 1;
  for (auto update_state : {update_client::ComponentState::kUpdateError,
                            update_client::ComponentState::kUpdated,
                            update_client::ComponentState::kUpToDate}) {
    SCOPED_TRACE(testing::Message()
                 << "update state: " << static_cast<int>(update_state));
    base::test::TestFuture<bool> future;
    MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
    ASSERT_TRUE(WaitForDownloadRequests(++downloads));

    // Another component reaching a terminal state is not ours to settle, and
    // ours is not there yet while it is still on its way. Settling stops
    // watching the update service before it answers anything, so still
    // watching is what says neither of these settled the request. The answer
    // itself arrives from a task, so it has not landed here either way.
    SendEvent(update_client::ComponentState::kUpdateError,
              "abcdefghijklmnopabcdefghijklmnop");
    SendEvent(update_client::ComponentState::kDownloading, ComponentId());
    EXPECT_NE(nullptr, cus_observer_);

    SendEvent(update_state, ComponentId());
    EXPECT_FALSE(future.Get());
  }
}

// Tests the same state settling a request as a success once the download it
// waited on published a model. What the state is never decides that on its own.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       TerminalStateSettlesARequestWithTheModelThatArrived) {
  StartWithADownloadRunning(/*requests=*/1);

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  ASSERT_TRUE(WaitForDownloadRequests(2));

  // What `ComponentReady` publishes when the download lands, which the update
  // service reports as over right after.
  OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(install_dir_);
  SendEvent(update_client::ComponentState::kUpdated, ComponentId());

  EXPECT_TRUE(future.Get());
}

// Tests that turning the master switch off answers a request that was waiting
// on a download, and stops watching the update service for it. Nothing else
// would answer it, because the model it waited for is being taken away, and
// how that download ends is no longer anyone's news.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       SwitchingOffSettlesARequestWaitingOnADownload) {
  // The component was registered, so the update service owns removing the
  // files and this test never touches disk.
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(1)
      .WillOnce(testing::Return(true));
  StartWithADownloadRunning(/*requests=*/1);

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  ASSERT_TRUE(WaitForDownloadRequests(2));
  ASSERT_NE(nullptr, cus_observer_);

  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  EXPECT_FALSE(future.Get());
  EXPECT_EQ(nullptr, cus_observer_);
}

// Tests that shutting down settles a request that was waiting on a download,
// and stops watching the update service. Nothing unregisters here, because
// letting go of the update service is not what takes a model away, and a watch
// left on it would outlive the service it points at.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       ShutdownSettlesARequestWaitingOnADownload) {
  EXPECT_CALL(*cus_, UnregisterComponent(kOnDeviceSpeechModelsComponentId))
      .Times(0);
  StartWithADownloadRunning(/*requests=*/1);

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());
  ASSERT_TRUE(WaitForDownloadRequests(2));
  ASSERT_NE(nullptr, cus_observer_);

  ShutdownOnDeviceSpeechModelsComponentRegistration();

  EXPECT_FALSE(future.Get());
  EXPECT_EQ(nullptr, cus_observer_);
}

// Tests that a download that failed settles the request, and answers it
// without a model. The download was started for this request, so its own
// answer is what carries the failure. The update service reports the failure
// too, but by then there is nothing left waiting to hear it.
TEST_F(OnDeviceSpeechModelsComponentInstallerUnitTest,
       DownloadFailureSettlesTheRequest) {
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(2)
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(on_demand_updater_,
              EnsureInstalled(kOnDeviceSpeechModelsComponentId, testing::_))
      .Times(2)
      .WillRepeatedly(
          [this](const std::string& id, component_updater::Callback callback) {
            ++download_count_;
            std::move(callback).Run(update_client::Error::UPDATE_CHECK_ERROR);
          });
  ManageOnDeviceSpeechModelsComponentRegistration(cus_.get(),
                                                  local_state_.get());
  ASSERT_TRUE(WaitForDownloadRequests(1));

  base::test::TestFuture<bool> future;
  MaybeRegisterOnDeviceSpeechModelsComponent(future.GetCallback());

  EXPECT_FALSE(future.Get());
  EXPECT_FALSE(OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled());
  // Watched only for a request waiting on a download, so nothing is left
  // watching once none is.
  EXPECT_EQ(nullptr, cus_observer_);
}

}  // namespace local_ai
