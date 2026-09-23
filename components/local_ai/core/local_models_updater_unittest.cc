/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/local_models_updater.h"

#include <memory>
#include <vector>

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
#include "brave/components/local_ai/core/pref_names.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/prefs/testing_pref_service.h"
#include "components/update_client/update_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

namespace {
constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("BraveLocalAIModels");
constexpr char kComponentId[] = "ejhejjmaoaohpghnblcdcjilndkangfe";

class TestUpdaterStateObserver : public LocalModelsUpdaterState::Observer {
 public:
  void OnLocalModelsReady(const base::FilePath& install_dir) override {
    ++ready_count;
  }
  void OnLocalModelsUnavailable() override { ++unavailable_count; }

  int ready_count = 0;
  int unavailable_count = 0;
};

}  // namespace

class LocalModelsUpdaterUnitTest : public testing::Test {
 public:
  LocalModelsUpdaterUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    feature_list_.InitWithFeatures({history_embeddings::kHistoryEmbeddings},
                                   {});
  }

  ~LocalModelsUpdaterUnitTest() override = default;

  void SetUp() override {
    cus_ = std::make_unique<component_updater::MockComponentUpdateService>();
    local_state_ = std::make_unique<TestingPrefServiceSimple>();
    prefs::RegisterLocalStatePrefs(local_state_->registry());
    local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, true);
    auto component_dir =
        base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);
    install_dir_ = component_dir.Append(kComponentInstallDir);
  }

  void TearDown() override {
    // Clear singleton state to avoid polluting other test suites, and to drop
    // the registrar's pointers to the per-test cus and local state.
    ShutdownLocalModelsComponentRegistration();
    LocalModelsUpdaterState::GetInstance()->SetInstallDir(base::FilePath());
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
  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());
  run_loop.Run();
}

// Tests that ComponentReady sets up the install directory and the model dir.
TEST_F(LocalModelsUpdaterUnitTest, ComponentReady) {
  LocalModelsComponentInstallerPolicy policy(local_state_.get());
  policy.ComponentReady(base::Version("1.0.0"), install_dir_,
                        base::DictValue());

  auto* state = LocalModelsUpdaterState::GetInstance();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return state->GetInstallDir() == install_dir_; }));
  EXPECT_EQ(state->GetInstallDir(), install_dir_);
  EXPECT_EQ(state->GetEmbeddingGemmaLitertDir(),
            install_dir_.AppendASCII(kEmbeddingGemmaModelDir)
                .AppendASCII(kEmbeddingGemmaLitertDir));
}

// Tests that the component directory is deleted when the feature is disabled.
TEST_F(LocalModelsUpdaterUnitTest, DeleteComponent) {
  CreateDirectory(install_dir_);
  EXPECT_TRUE(PathExists(install_dir_));

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(history_embeddings::kHistoryEmbeddings);
  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  // Nothing was registered, so the directory is ours to remove.
  EXPECT_CALL(*cus_, UnregisterComponent(kComponentId))
      .Times(1)
      .WillOnce(testing::Return(false));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
  EXPECT_FALSE(PathExists(install_dir_));
}

// Tests that the component is not registered when the feature is disabled.
TEST_F(LocalModelsUpdaterUnitTest, NoRegisterWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(history_embeddings::kHistoryEmbeddings);

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(*cus_, UnregisterComponent(kComponentId))
      .Times(1)
      .WillOnce(testing::Return(false));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());
}

// Tests that the component is not registered when ComponentUpdateService is
// null.
TEST_F(LocalModelsUpdaterUnitTest, NoRegisterWhenCUSIsNull) {
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  ManageLocalModelsComponentRegistration(nullptr, local_state_.get());
}

// Tests that the component is not registered when the local AI master switch
// is off, and the component directory is deleted.
TEST_F(LocalModelsUpdaterUnitTest, NoRegisterWhenMasterSwitchOff) {
  CreateDirectory(install_dir_);
  EXPECT_TRUE(PathExists(install_dir_));

  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(*cus_, UnregisterComponent(kComponentId))
      .Times(1)
      .WillOnce(testing::Return(false));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
}

// Unregistration is deferred behind an in-flight update, so that update still
// installs and reports ready after the switch turned off.
TEST_F(LocalModelsUpdaterUnitTest, ComponentReadyIgnoredWhenMasterSwitchOff) {
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  LocalModelsComponentInstallerPolicy policy(local_state_.get());
  policy.ComponentReady(base::Version("1.0.0"), install_dir_,
                        base::DictValue());
  EXPECT_TRUE(LocalModelsUpdaterState::GetInstance()->GetInstallDir().empty());
}

// Tests that observers are told when the models go away, so they can drop
// anything derived from the install dir.
TEST_F(LocalModelsUpdaterUnitTest, ObserverNotifiedWhenModelsGoAway) {
  auto* state = LocalModelsUpdaterState::GetInstance();
  TestUpdaterStateObserver observer;
  state->AddObserver(&observer);

  state->SetInstallDir(install_dir_);
  EXPECT_EQ(observer.ready_count, 1);
  EXPECT_EQ(observer.unavailable_count, 0);

  state->SetInstallDir(base::FilePath());
  EXPECT_EQ(observer.ready_count, 1);
  EXPECT_EQ(observer.unavailable_count, 1);

  state->RemoveObserver(&observer);
}

// The switch reaches its managed value only after components are registered,
// so turning off has to tear an already-registered component back down.
TEST_F(LocalModelsUpdaterUnitTest, UnregisterWhenMasterSwitchTurnsOff) {
  base::RunLoop run_loop;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(1)
      .WillOnce([quit = run_loop.QuitClosure()]() { quit.Run(); });
  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());
  run_loop.Run();

  LocalModelsUpdaterState::GetInstance()->SetInstallDir(install_dir_);

  // The component was registered, so ComponentInstaller::Uninstall() owns
  // removing the files; we only drop the recorded install dir.
  EXPECT_CALL(*cus_, UnregisterComponent(kComponentId))
      .Times(1)
      .WillOnce(testing::Return(true));
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  EXPECT_TRUE(LocalModelsUpdaterState::GetInstance()->GetInstallDir().empty());
}

// Tests that a registration abandoned because the master switch turned off
// before it landed still cleans the component directory up.
TEST_F(LocalModelsUpdaterUnitTest, AbandonedRegistrationCleansUpDirectory) {
  CreateDirectory(install_dir_);
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  // Twice: the pref change finds a registration pending and leaves the
  // directory alone, then the abandoned registration cleans up.
  EXPECT_CALL(*cus_, UnregisterComponent(kComponentId))
      .Times(2)
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);

  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !PathExists(install_dir_); }));
}

// The switch can turn off between RegisterComponent() and our callback, when
// there is nothing yet to unregister, so the teardown has to happen again.
TEST_F(LocalModelsUpdaterUnitTest,
       UnregisterWhenMasterSwitchTurnsOffWhileRegistering) {
  int unregister_count = 0;
  EXPECT_CALL(*cus_, UnregisterComponent(kComponentId))
      .WillRepeatedly([&unregister_count]() {
        ++unregister_count;
        return true;
      });
  // ComponentInstaller::FinishRegistration() registers before it runs our
  // callback, so this is the moment the switch can turn off unnoticed.
  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).WillOnce([this]() {
    local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
    return true;
  });
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(0);
  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());

  // One from the pref change, one from the registration landing afterwards.
  ASSERT_TRUE(base::test::RunUntil([&]() { return unregister_count == 2; }));
}

// Toggling while a registration is in flight must not remove the directory it
// may still publish, nor start a second registration.
TEST_F(LocalModelsUpdaterUnitTest, SwitchTogglesWhileRegistrationPending) {
  CreateDirectory(install_dir_);
  base::RunLoop run_loop;
  // Nothing is registered for the whole toggle: the registration only reaches
  // the service once ComponentInstaller finishes reading the manifest.
  EXPECT_CALL(*cus_, UnregisterComponent(kComponentId))
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(1)
      .WillOnce([quit = run_loop.QuitClosure()]() { quit.Run(); });

  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, true);
  run_loop.Run();

  EXPECT_TRUE(PathExists(install_dir_));
}

// Registration and uninstall share the installer's task runner. A fresh
// instance per registration would let a queued uninstall run afterwards and
// delete what was just installed.
TEST_F(LocalModelsUpdaterUnitTest, ReusesInstallerAcrossReregistration) {
  std::vector<scoped_refptr<update_client::CrxInstaller>> installers;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .WillRepeatedly(
          [&installers](
              const component_updater::ComponentRegistration& registration) {
            installers.push_back(registration.installer);
            return true;
          });
  EXPECT_CALL(*cus_, UnregisterComponent(kComponentId))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(testing::AnyNumber());

  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());
  ASSERT_TRUE(base::test::RunUntil([&]() { return installers.size() == 1u; }));

  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, true);
  ASSERT_TRUE(base::test::RunUntil([&]() { return installers.size() == 2u; }));

  EXPECT_EQ(installers[0], installers[1]);
}

// Tests that the component is registered when the local AI master switch is
// turned on after startup.
TEST_F(LocalModelsUpdaterUnitTest, RegisterWhenMasterSwitchTurnsOn) {
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, false);
  EXPECT_CALL(*cus_, UnregisterComponent(kComponentId))
      .Times(1)
      .WillOnce(testing::Return(false));
  ManageLocalModelsComponentRegistration(cus_.get(), local_state_.get());

  base::RunLoop run_loop;
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(kComponentId, testing::_))
      .Times(1)
      .WillOnce([quit = run_loop.QuitClosure()]() { quit.Run(); });
  local_state_->SetBoolean(prefs::kBraveLocalAIEnabled, true);
  run_loop.Run();
}

}  // namespace local_ai
