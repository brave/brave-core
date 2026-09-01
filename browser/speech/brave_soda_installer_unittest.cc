/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/brave_soda_installer.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_future.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "components/soda/constants.h"
#include "components/update_client/update_client_errors.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speech {

namespace {

// OnDeviceSpeechRecognitionImpl is the real observer, and settles the reply
// parked by SpeechRecognition.install() from exactly these notifications.
class MockSodaObserver : public SodaInstaller::Observer {
 public:
  MOCK_METHOD(void, OnSodaInstalled, (LanguageCode language_code), (override));
  MOCK_METHOD(void,
              OnSodaInstallError,
              (LanguageCode language_code, SodaInstaller::ErrorCode error_code),
              (override));
  MOCK_METHOD(void,
              OnSodaProgress,
              (LanguageCode language_code, int progress),
              (override));
};

}  // namespace

class BraveSodaInstallerUnitTest : public testing::Test {
 public:
  BraveSodaInstallerUnitTest() {
    feature_list_.InitAndEnableFeature(
        local_ai::kBraveOnDeviceSpeechRecognition);
  }

  // Starts the registrar the way startup does, so every test runs with one.
  // Nothing publishes a model, so a request settles as a failure unless a test
  // installs one.
  void SetUp() override {
    ON_CALL(cus_, RegisterComponent).WillByDefault(testing::Return(true));

    // Starting the registrar asks for a download of its own. Waiting for it
    // means a test's own request starts a registration rather than joining
    // this one.
    base::test::TestFuture<void> started;
    EXPECT_CALL(
        on_demand_updater_,
        EnsureInstalled(local_ai::kOnDeviceSpeechModelsComponentId, testing::_))
        .WillOnce([&started](const std::string& id,
                             component_updater::Callback callback) {
          std::move(callback).Run(update_client::Error::UPDATE_CHECK_ERROR);
          started.SetValue();
        });
    local_ai::ManageOnDeviceSpeechModelsComponentRegistration(&cus_,
                                                              local_state());
    ASSERT_TRUE(started.Wait());
  }

  void TearDown() override {
    installer_.reset();
    // Both singletons outlive the fixture: the registrar holds this test's
    // update service, and a model left behind leaks into the next test.
    local_ai::ShutdownOnDeviceSpeechModelsComponentRegistration();
    RemoveModel();
  }

 protected:
  // Deferred so a test can install a model before the observer is attached,
  // keeping setup notifications out of what the strict mock sees.
  void CreateInstaller() {
    installer_ = std::make_unique<BraveSodaInstaller>();
    installer_->AddObserver(&observer_);
  }

  PrefService* local_state() {
    return TestingBrowserProcess::GetGlobal()->local_state();
  }

  // What ComponentReady publishes once a model is on disk.
  void InstallModel() {
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  }

  void RemoveModel() {
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath());
  }

  testing::StrictMock<MockSodaObserver> observer_;
  std::unique_ptr<BraveSodaInstaller> installer_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
  testing::NiceMock<component_updater::MockComponentUpdateService> cus_;
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  // `ComponentInstaller::Register` creates the directory the component would
  // install into. Without this it creates one in the real component directory.
  base::ScopedPathOverride component_dir_override_{
      component_updater::DIR_COMPONENT_USER};
};

// The list `IsLanguageInstallable` gates `install()` on, and that
// `InstallLanguage` refuses anything outside.
TEST_F(BraveSodaInstallerUnitTest, OffersEnglishOnly) {
  CreateInstaller();
  EXPECT_THAT(installer_->GetLiveCaptionEnabledLanguages(),
              testing::ElementsAre(GetLanguageName(LanguageCode::kEnUs)));
  EXPECT_EQ(installer_->GetLiveCaptionEnabledLanguages(),
            installer_->GetAvailableLanguages());
}

// Brave ships no SODA library and no language pack, so the paths upstream
// would load them from stay empty.
TEST_F(BraveSodaInstallerUnitTest, PublishesNoSodaPaths) {
  CreateInstaller();
  EXPECT_TRUE(installer_->GetSodaBinaryPath().empty());
  EXPECT_TRUE(installer_->GetLanguagePath(GetLanguageName(LanguageCode::kEnUs))
                  .empty());
}

TEST_F(BraveSodaInstallerUnitTest, ModelArrivingReportsInstalled) {
  CreateInstaller();
  ASSERT_FALSE(installer_->IsSodaInstalled(LanguageCode::kEnUs));

  EXPECT_CALL(observer_, OnSodaInstalled(LanguageCode::kEnUs)).Times(1);
  InstallModel();

  EXPECT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
}

TEST_F(BraveSodaInstallerUnitTest, ModelRemovalClearsInstalled) {
  InstallModel();
  CreateInstaller();
  ASSERT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));

  RemoveModel();

  EXPECT_FALSE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
}

TEST_F(BraveSodaInstallerUnitTest, InstallLanguageIgnoresOtherLanguages) {
  CreateInstaller();
  bool requested = false;
  installer_->SetModelInstallRequestedCallbackForTesting(
      base::BindLambdaForTesting([&] { requested = true; }));

  installer_->InstallLanguage("fr-FR", local_state());

  EXPECT_FALSE(requested);
}

// A model already on disk stops the call there rather than asking for a
// download of what is installed. Nothing is reported, because nothing changed.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageWithAModelDoesNothing) {
  InstallModel();
  CreateInstaller();
  ASSERT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
  bool requested = false;
  installer_->SetModelInstallRequestedCallbackForTesting(
      base::BindLambdaForTesting([&] { requested = true; }));

  installer_->InstallLanguage("en-US", local_state());

  EXPECT_FALSE(requested);
}

// A request that ends without a model is reported as a failure rather than
// left to hang, since install() has already parked its reply.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageWithoutAModelReportsAnError) {
  CreateInstaller();
  EXPECT_CALL(
      on_demand_updater_,
      EnsureInstalled(local_ai::kOnDeviceSpeechModelsComponentId, testing::_))
      .WillOnce(
          [](const std::string& id, component_updater::Callback callback) {
            std::move(callback).Run(update_client::Error::UPDATE_CHECK_ERROR);
          });
  base::test::TestFuture<void> reported;
  EXPECT_CALL(observer_,
              OnSodaInstallError(LanguageCode::kEnUs,
                                 SodaInstaller::ErrorCode::kUnspecifiedError))
      .WillOnce([&] { reported.SetValue(); });

  installer_->InstallLanguage("en-US", local_state());

  EXPECT_TRUE(reported.Wait());
}

// Both `OnSpeechModelDirChanged` and `OnSpeechModelInstallFinished` run for one
// successful install. Only the first calls `NotifyOnSodaInstalled`, and only
// once, so `install()` is not told twice that the same model installed.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageReportsInstalledOnce) {
  CreateInstaller();
  EXPECT_CALL(observer_, OnSodaInstalled(LanguageCode::kEnUs)).Times(1);
  base::test::TestFuture<bool> finished;
  installer_->SetModelInstallFinishedCallbackForTesting(
      finished.GetRepeatingCallback());
  EXPECT_CALL(
      on_demand_updater_,
      EnsureInstalled(local_ai::kOnDeviceSpeechModelsComponentId, testing::_))
      .WillOnce(
          [&](const std::string& id, component_updater::Callback callback) {
            // Installed before the request is answered, so the answer is a
            // success and `OnSpeechModelInstallFinished` runs with `true`.
            InstallModel();
            std::move(callback).Run(update_client::Error::NONE);
          });

  installer_->InstallLanguage("en-US", local_state());

  EXPECT_TRUE(finished.Get());
}

}  // namespace speech
