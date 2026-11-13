/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/brave_soda_installer.h"

#include <memory>
#include <string>
#include <string_view>
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
#include "brave/components/local_ai/core/utils.h"
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

std::string ComponentIdOf(OnDeviceSpeechModel model) {
  return model == kEnglish
             ? local_ai::kOnDeviceSpeechEnglishModelComponentId
             : local_ai::kOnDeviceSpeechMultilingualModelComponentId;
}

// A model, and a locale that only this model serves.
struct ModelCase {
  OnDeviceSpeechModel model;
  std::string_view locale;
  LanguageCode language_code;
};

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

class BraveSodaInstallerTestBase : public testing::Test {
 public:
  explicit BraveSodaInstallerTestBase(OnDeviceSpeechModel enabled_model)
      : enabled_model_(enabled_model) {
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
    // requests for it reach the download.
    local_state()->SetBoolean(
        local_ai::GetOnDeviceSpeechModelEnabledPref(enabled_model_), true);

    // Starting the registrar asks for a download of its own. Waiting for it
    // means a test's own request starts a registration rather than joining
    // this one.
    base::test::TestFuture<void> started;
    EXPECT_CALL(on_demand_updater_,
                EnsureInstalled(ComponentIdOf(enabled_model_), testing::_))
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

  // Answers the next download request for `model`, and expects to be asked
  // once. A success publishes the model first, as a real download does before
  // the request it answers is settled.
  void AnswerDownloadWith(OnDeviceSpeechModel model,
                          update_client::Error error) {
    EXPECT_CALL(on_demand_updater_,
                EnsureInstalled(ComponentIdOf(model), testing::_))
        .WillOnce([this, model, error](const std::string& id,
                                       component_updater::Callback callback) {
          if (error == update_client::Error::NONE) {
            InstallModel(model);
          }
          std::move(callback).Run(error);
        });
  }

  const OnDeviceSpeechModel enabled_model_;
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

class BraveSodaInstallerUnitTest : public BraveSodaInstallerTestBase {
 public:
  BraveSodaInstallerUnitTest() : BraveSodaInstallerTestBase(kEnglish) {}
};

// Runs once for each model, with that model enabled.
class BraveSodaInstallerModelUnitTest
    : public BraveSodaInstallerTestBase,
      public testing::WithParamInterface<ModelCase> {
 public:
  BraveSodaInstallerModelUnitTest()
      : BraveSodaInstallerTestBase(GetParam().model) {}
};

// With the multilingual feature on, the locales of both models are offered.
TEST_F(BraveSodaInstallerUnitTest, OffersSupportedLanguages) {
  CreateInstaller();
  const std::vector<std::string> languages =
      installer_->GetLiveCaptionEnabledLanguages();
  EXPECT_THAT(languages, testing::IsSupersetOf({"en-US", "uk-UA"}));
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

  // The multilingual model reports uk-UA once. It does not report en-US again.
  EXPECT_CALL(observer_, OnSodaInstalled(testing::Ne(LanguageCode::kEnUs)))
      .Times(testing::AnyNumber());
  EXPECT_CALL(observer_, OnSodaInstalled(LanguageCode::kUkUa));
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

  AnswerDownloadWith(kEnglish, update_client::Error::UPDATE_CHECK_ERROR);
  base::test::TestFuture<bool> answered;
  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(kEnglish,
                                                       answered.GetCallback());
  EXPECT_FALSE(answered.Get());
}

// The model of the language already on disk must stop `InstallLanguage` from
// starting a registration for what is already installed. The other model is
// not on disk. To see whether `InstallLanguage` started a registration, the
// test removes the model once `InstallLanguage` returns, since a registration
// that ends with a model calls nothing on `observer_`. It then waits on a
// request of its own, which joins any registration still pending. If
// `InstallLanguage` started that registration, the installer calls
// `OnSodaInstallError` on `observer_`, a strict mock that expects no calls,
// and the test fails.
TEST_P(BraveSodaInstallerModelUnitTest, InstallLanguageWithAModelDoesNothing) {
  InstallModel(GetParam().model);
  CreateInstaller();
  ASSERT_TRUE(installer_->IsSodaInstalled(GetParam().language_code));
  installer_->InstallLanguage(GetParam().locale, local_state());
  RemoveModel(GetParam().model);

  // Any answer but `UPDATE_IN_PROGRESS` settles the request, and the
  // `RemoveModel()` above is what makes it `false`, not the error. The answer
  // is a failure only because a success would publish the model again.
  AnswerDownloadWith(GetParam().model,
                     update_client::Error::UPDATE_CHECK_ERROR);
  base::test::TestFuture<bool> answered;
  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(GetParam().model,
                                                       answered.GetCallback());
  EXPECT_FALSE(answered.Get());
}

// `InstallLanguage` asks for the model of the language. A request that ends
// without a model is reported as a failure of the language rather than left to
// hang, since install() has already parked its reply.
TEST_P(BraveSodaInstallerModelUnitTest,
       InstallLanguageWithoutAModelReportsAnError) {
  CreateInstaller();
  AnswerDownloadWith(GetParam().model,
                     update_client::Error::UPDATE_CHECK_ERROR);
  base::test::TestFuture<void> reported;
  EXPECT_CALL(observer_,
              OnSodaInstallError(GetParam().language_code,
                                 SodaInstaller::ErrorCode::kUnspecifiedError))
      .WillOnce([&] { reported.SetValue(); });

  installer_->InstallLanguage(GetParam().locale, local_state());

  EXPECT_TRUE(reported.Wait());
}

// Both `OnSpeechModelDirChanged` and `OnSpeechModelInstallFinished` run for one
// successful install. Only the first calls `NotifyOnSodaInstalled`, and only
// once for the language, so `install()` is not told twice that the language
// installed. The model can report its other locales too.
TEST_P(BraveSodaInstallerModelUnitTest, InstallLanguageReportsInstalledOnce) {
  CreateInstaller();
  EXPECT_CALL(observer_, OnSodaInstalled(testing::_))
      .Times(testing::AnyNumber());
  EXPECT_CALL(observer_, OnSodaInstalled(GetParam().language_code));
  AnswerDownloadWith(GetParam().model, update_client::Error::NONE);

  installer_->InstallLanguage(GetParam().locale, local_state());

  // A request of the test's own joins the one `InstallLanguage` started, and
  // the registrar answers them in order, so once this one is answered
  // `OnSpeechModelInstallFinished` has run too.
  base::test::TestFuture<bool> answered;
  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(GetParam().model,
                                                       answered.GetCallback());
  EXPECT_TRUE(answered.Get());
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveSodaInstallerModelUnitTest,
    testing::Values(ModelCase{kEnglish, "en-US", LanguageCode::kEnUs},
                    ModelCase{kMultilingual, "uk-UA", LanguageCode::kUkUa}),
    [](const testing::TestParamInfo<ModelCase>& info) {
      return info.param.model == kEnglish ? "English" : "Multilingual";
    });

}  // namespace speech
