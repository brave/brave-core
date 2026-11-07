/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/brave_soda_installer.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_path_override.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "components/soda/constants.h"
#include "components/soda/soda_installer.h"
#include "components/update_client/update_client_errors.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speech {

namespace {

using local_ai::OnDeviceSpeechModel;

constexpr OnDeviceSpeechModel kEnglish = OnDeviceSpeechModel::kEnglish;
constexpr OnDeviceSpeechModel kMultilingual =
    OnDeviceSpeechModel::kMultilingual;

int MultilingualLocaleCount() {
  return std::ranges::count(GetBraveOnDeviceSpeechLocales(),
                            BraveOnDeviceSpeechModelKind::kMultilingual,
                            &BraveOnDeviceSpeechLocale::model);
}

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
    feature_list_.InitWithFeatures({local_ai::kBraveOnDeviceSpeechRecognition,
                                    local_ai::kBraveOnDeviceSpeechMultilingual},
                                   {});
  }

  // Starts the registrar the way startup does, so every test runs with one.
  // Nothing publishes a model, so a request settles as a failure unless a test
  // installs one.
  void SetUp() override {
    ON_CALL(cus_, RegisterComponent).WillByDefault(testing::Return(true));
    // An enabled model, so the registrar registers it and the installer's
    // requests reach the download.
    local_state()->SetBoolean(
        local_ai::prefs::kOnDeviceSpeechEnglishModelEnabled, true);

    // Starting the registrar asks for a download of its own. Waiting for it
    // means a test's own request starts a registration rather than joining
    // this one.
    base::test::TestFuture<void> started;
    EXPECT_CALL(
        on_demand_updater_,
        EnsureInstalled(local_ai::kOnDeviceSpeechEnglishModelComponentId,
                        testing::_))
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
    RemoveModel(kEnglish);
    RemoveModel(kMultilingual);
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
  void InstallModel(OnDeviceSpeechModel model = kEnglish) {
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        model, base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  }

  void RemoveModel(OnDeviceSpeechModel model = kEnglish) {
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        model, base::FilePath());
  }

  // Answers the next download request, and expects to be asked once. A success
  // publishes the model first, as a real download does before the request it
  // answers is settled.
  void AnswerDownloadWith(update_client::Error error) {
    EXPECT_CALL(
        on_demand_updater_,
        EnsureInstalled(local_ai::kOnDeviceSpeechEnglishModelComponentId,
                        testing::_))
        .WillOnce([this, error](const std::string& id,
                                component_updater::Callback callback) {
          if (error == update_client::Error::NONE) {
            InstallModel();
          }
          std::move(callback).Run(error);
        });
  }

  base::test::ScopedFeatureList feature_list_;
  // `ComponentInstaller::Register` creates the directory the component would
  // install into. Without this it creates one in the real component directory.
  base::ScopedPathOverride component_dir_override_{
      component_updater::DIR_COMPONENT_USER};
  testing::StrictMock<MockSodaObserver> observer_;
  std::unique_ptr<BraveSodaInstaller> installer_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
  testing::NiceMock<component_updater::MockComponentUpdateService> cus_;
  // Declared last, since its destructor still runs the thread pool work a test
  // left pending, and that work creates the component directory.
  base::test::TaskEnvironment task_environment_;
};

// The list `IsLanguageInstallable` gates `install()` on, and that
// `InstallLanguage` refuses anything outside.
TEST_F(BraveSodaInstallerUnitTest, OffersSupportedLanguages) {
  CreateInstaller();
  const std::vector<std::string> languages =
      installer_->GetLiveCaptionEnabledLanguages();
  EXPECT_EQ(GetBraveOnDeviceSpeechLocales().size(), languages.size());
  // Both models, and locales with SODA's and with Brave's own codes.
  EXPECT_THAT(languages, testing::IsSupersetOf({"en-US", "de-DE", "uk-UA"}));
  EXPECT_EQ(languages, installer_->GetAvailableLanguages());
}

TEST_F(BraveSodaInstallerUnitTest,
       OffersEnglishOnlyWithMultilingualFeatureOff) {
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(local_ai::kBraveOnDeviceSpeechMultilingual);
  CreateInstaller();

  EXPECT_THAT(installer_->GetLiveCaptionEnabledLanguages(),
              testing::ElementsAre("en-US"));
}

TEST_F(BraveSodaInstallerUnitTest, OffersNothingWithLocalAIOff) {
  local_state()->SetBoolean(local_ai::prefs::kBraveLocalAIEnabled, false);
  CreateInstaller();

  EXPECT_TRUE(installer_->GetLiveCaptionEnabledLanguages().empty());
  EXPECT_TRUE(installer_->GetAvailableLanguages().empty());
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

  EXPECT_CALL(observer_, OnSodaInstalled(LanguageCode::kEnUs));
  InstallModel();

  EXPECT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
  EXPECT_FALSE(installer_->IsSodaInstalled(LanguageCode::kUkUa));
}

// The multilingual model reports each of its locales, so that `install()`
// settles a reply for any of them.
TEST_F(BraveSodaInstallerUnitTest,
       MultilingualModelArrivingReportsItsLocalesInstalled) {
  CreateInstaller();
  EXPECT_CALL(observer_, OnSodaInstalled).Times(MultilingualLocaleCount());

  InstallModel(kMultilingual);

  EXPECT_TRUE(installer_->IsSodaInstalled(LanguageCode::kUkUa));
  EXPECT_FALSE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
}

TEST_F(BraveSodaInstallerUnitTest, ModelRemovalClearsInstalled) {
  InstallModel();
  CreateInstaller();
  ASSERT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));

  RemoveModel();

  EXPECT_FALSE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
}

// Each model installs and removes only its own locales. Removing one model
// leaves the locales of the other installed.
TEST_F(BraveSodaInstallerUnitTest, EachModelInstallsOnlyItsOwnLocales) {
  CreateInstaller();
  EXPECT_CALL(observer_, OnSodaInstalled(LanguageCode::kEnUs));
  InstallModel(kEnglish);
  testing::Mock::VerifyAndClearExpectations(&observer_);

  // Only the multilingual locales are reported. en-US is not reported again.
  EXPECT_CALL(observer_, OnSodaInstalled(testing::Ne(LanguageCode::kEnUs)))
      .Times(MultilingualLocaleCount());
  InstallModel(kMultilingual);
  EXPECT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
  EXPECT_TRUE(installer_->IsSodaInstalled(LanguageCode::kUkUa));

  RemoveModel(kEnglish);
  EXPECT_FALSE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
  EXPECT_TRUE(installer_->IsSodaInstalled(LanguageCode::kUkUa));

  RemoveModel(kMultilingual);
  EXPECT_FALSE(installer_->IsSodaInstalled(LanguageCode::kUkUa));
}

// `InstallLanguage` must not start a registration for a language Brave does
// not serve. To see whether it did, the test waits on a request of its own,
// which joins any registration still pending. That registration ends without
// a model, so if `InstallLanguage` started it, the installer calls
// `OnSodaInstallError` on `observer_`, a strict mock that expects no calls,
// and the test fails.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageIgnoresOtherLanguages) {
  CreateInstaller();
  installer_->InstallLanguage("th-TH", local_state());

  AnswerDownloadWith(update_client::Error::UPDATE_CHECK_ERROR);
  base::test::TestFuture<bool> answered;
  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(kEnglish,
                                                       answered.GetCallback());
  EXPECT_FALSE(answered.Get());
}

// A model already on disk must stop `InstallLanguage` from starting a
// registration for what is already installed. To see whether it did, the test
// removes the model once `InstallLanguage` returns, since a registration that
// ends with a model calls nothing on `observer_`. It then waits on a request
// of its own, which joins any registration still pending. If `InstallLanguage`
// started that registration, the installer calls `OnSodaInstallError` on
// `observer_`, a strict mock that expects no calls, and the test fails.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageWithAModelDoesNothing) {
  InstallModel();
  CreateInstaller();
  ASSERT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
  installer_->InstallLanguage("en-US", local_state());
  RemoveModel();

  // Any answer but `UPDATE_IN_PROGRESS` settles the request, and the
  // `RemoveModel()` above is what makes it `false`, not the error. The answer
  // is a failure only because a success would publish the model again.
  AnswerDownloadWith(update_client::Error::UPDATE_CHECK_ERROR);
  base::test::TestFuture<bool> answered;
  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(kEnglish,
                                                       answered.GetCallback());
  EXPECT_FALSE(answered.Get());
}

// A request that ends without a model is reported as a failure rather than
// left to hang, since install() has already parked its reply.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageWithoutAModelReportsAnError) {
  CreateInstaller();
  AnswerDownloadWith(update_client::Error::UPDATE_CHECK_ERROR);
  base::test::TestFuture<void> reported;
  EXPECT_CALL(observer_,
              OnSodaInstallError(LanguageCode::kEnUs,
                                 SodaInstaller::ErrorCode::kUnspecifiedError))
      .WillOnce([&] { reported.SetValue(); });

  installer_->InstallLanguage("en-US", local_state());

  EXPECT_TRUE(reported.Wait());
}

// With only the English model on disk, a multilingual locale still asks for
// its own model. The error names the locale that `install()` waits on.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageAsksForTheModelItNeeds) {
  InstallModel(kEnglish);
  CreateInstaller();
  base::test::TestFuture<void> reported;
  EXPECT_CALL(observer_,
              OnSodaInstallError(LanguageCode::kUkUa,
                                 SodaInstaller::ErrorCode::kUnspecifiedError))
      .WillOnce([&] { reported.SetValue(); });

  installer_->InstallLanguage("uk-UA", local_state());

  EXPECT_TRUE(reported.Wait());
}

// Both `OnSpeechModelDirChanged` and `OnSpeechModelInstallFinished` run for one
// successful install. Only the first calls `NotifyOnSodaInstalled`, and only
// once, so `install()` is not told twice that the same model installed.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageReportsInstalledOnce) {
  CreateInstaller();
  EXPECT_CALL(observer_, OnSodaInstalled(LanguageCode::kEnUs));
  AnswerDownloadWith(update_client::Error::NONE);

  installer_->InstallLanguage("en-US", local_state());

  // A request of the test's own joins the one `InstallLanguage` started, and
  // the registrar answers them in order, so once this one is answered
  // `OnSpeechModelInstallFinished` has run too.
  base::test::TestFuture<bool> answered;
  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(kEnglish,
                                                       answered.GetCallback());
  EXPECT_TRUE(answered.Get());
}

}  // namespace speech
