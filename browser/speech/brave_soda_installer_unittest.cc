/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/brave_soda_installer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/soda/constants.h"
#include "components/soda/pref_names.h"
#include "components/update_client/crx_update_item.h"
#include "components/update_client/update_client_errors.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speech {

namespace {

// Records what the installer reported. Upstream's own observer is
// OnDeviceSpeechRecognitionImpl, which settles the reply parked by
// SpeechRecognition.install() from exactly these notifications.
class TestSodaObserver : public SodaInstaller::Observer {
 public:
  // SodaInstaller::Observer:
  void OnSodaInstalled(LanguageCode language_code) override {
    ++installed_count_;
  }
  void OnSodaInstallError(LanguageCode language_code,
                          SodaInstaller::ErrorCode error_code) override {
    ++error_count_;
  }
  void OnSodaProgress(LanguageCode language_code, int progress) override {
    ++progress_count_;
  }

  int installed_count() const { return installed_count_; }
  int error_count() const { return error_count_; }
  int progress_count() const { return progress_count_; }

 private:
  int installed_count_ = 0;
  int error_count_ = 0;
  int progress_count_ = 0;
};

}  // namespace

class BraveSodaInstallerUnitTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        local_ai::kBraveOnDeviceSpeechRecognition);

    auto cus =
        std::make_unique<component_updater::MockComponentUpdateService>();
    cus_ = cus.get();
    TestingBrowserProcess::GetGlobal()->SetComponentUpdater(std::move(cus));
  }

  void TearDown() override {
    if (installer_) {
      installer_->RemoveObserver(&observer_);
      installer_.reset();
    }
    TestingBrowserProcess::GetGlobal()->SetComponentUpdater(nullptr);
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath());
  }

 protected:
  // Deferred so a test can publish a model, or arrange the component updater,
  // before the installer observes either.
  void CreateInstaller() {
    installer_ = std::make_unique<BraveSodaInstaller>();
    installer_->AddObserver(&observer_);
  }

  PrefService* local_state() {
    return TestingBrowserProcess::GetGlobal()->GetTestingLocalState();
  }

  void PublishModel() {
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  }

  void InstallLanguage() {
    installer_->InstallLanguage(GetLanguageName(LanguageCode::kEnUs),
                                local_state());
  }

  // Sends what the component updater reports about a download this installer
  // did not necessarily start. Delivered through the observer interface,
  // which is how the component updater itself reaches it.
  void SendEvent(update_client::ComponentState state, const std::string& id) {
    update_client::CrxUpdateItem item;
    item.state = state;
    item.id = id;
    static_cast<component_updater::ServiceObserver*>(installer_.get())
        ->OnEvent(item);
  }

  std::string ComponentId() {
    return std::string(local_ai::kOnDeviceSpeechModelsComponentId);
  }

  content::BrowserTaskEnvironment task_environment_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
  raw_ptr<component_updater::MockComponentUpdateService> cus_ = nullptr;
  TestSodaObserver observer_;
  std::unique_ptr<BraveSodaInstaller> installer_;
  base::test::ScopedFeatureList feature_list_;
};

// ---------- The language list ----------

// Tests that the only language offered is the one Brave's model transcribes.
// OnDeviceSpeechRecognitionImpl::Install rejects anything outside this list
// before it reaches InstallLanguage, which is what keeps a request for another
// language from installing an English model.
TEST_F(BraveSodaInstallerUnitTest, OffersEnglishOnly) {
  CreateInstaller();

  const std::vector<std::string> expected = {
      GetLanguageName(LanguageCode::kEnUs)};
  EXPECT_EQ(expected, installer_->GetLiveCaptionEnabledLanguages());
  // The same list, so that narrowing one narrows every reader of the other.
  EXPECT_EQ(installer_->GetLiveCaptionEnabledLanguages(),
            installer_->GetAvailableLanguages());
}

// Tests that no SODA path is published. Brave ships neither the SODA library
// nor a SODA language pack, and the consumers that should have the model read
// OnDeviceSpeechModelsState directly.
TEST_F(BraveSodaInstallerUnitTest, PublishesNoSodaPaths) {
  CreateInstaller();

  EXPECT_TRUE(installer_->GetSodaBinaryPath().empty());
  EXPECT_TRUE(installer_->GetLanguagePath(GetLanguageName(LanguageCode::kEnUs))
                  .empty());
}

// ---------- What the model arriving and going away mean ----------

// Tests that the model arriving is reported as SODA being installed, which is
// how a parked SpeechRecognition.install() reply is settled as a success.
TEST_F(BraveSodaInstallerUnitTest, ModelArrivingReportsInstalled) {
  CreateInstaller();
  ASSERT_FALSE(installer_->IsSodaInstalled(LanguageCode::kEnUs));

  PublishModel();

  EXPECT_EQ(1, observer_.installed_count());
  EXPECT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
}

// Tests that the model going away stops it being reported as installed.
// Without this edge Install() would keep resolving true with no model.
TEST_F(BraveSodaInstallerUnitTest, ModelRemovalClearsInstalled) {
  PublishModel();
  CreateInstaller();
  ASSERT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));

  local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
      base::FilePath());

  EXPECT_FALSE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
  EXPECT_EQ(0, observer_.installed_count());
}

// Tests that an installer built after the model arrived already reports it,
// since the state replays to a consumer created after ComponentReady.
TEST_F(BraveSodaInstallerUnitTest, ConstructionAdoptsExistingInstall) {
  PublishModel();

  CreateInstaller();

  EXPECT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));
}

// Tests that a download is never reported as in progress. That state is
// process-global, so reporting it would tell every origin that some origin
// started a download.
TEST_F(BraveSodaInstallerUnitTest, NeverReportsDownloading) {
  CreateInstaller();
  EXPECT_FALSE(installer_->IsSodaLanguageDownloading(LanguageCode::kEnUs));

  EXPECT_CALL(on_demand_updater_, EnsureInstalled(ComponentId(), testing::_))
      .Times(testing::AnyNumber());
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .WillRepeatedly(testing::Return(true));
  InstallLanguage();
  task_environment_.RunUntilIdle();

  EXPECT_FALSE(installer_->IsSodaLanguageDownloading(LanguageCode::kEnUs));
  EXPECT_EQ(0, observer_.progress_count());

  PublishModel();
  EXPECT_FALSE(installer_->IsSodaLanguageDownloading(LanguageCode::kEnUs));
}

// ---------- SODA's own lifecycle, which Brave does not run ----------

// Tests that SODA's own language pack bookkeeping is left exactly as upstream
// registered it. RegisterLanguage would record a pack and arm its deletion,
// and UnregisterLanguage would erase the entry the pref is registered with, so
// running either would be visible here. Brave installs no such pack.
TEST_F(BraveSodaInstallerUnitTest, LeavesSodaLanguagePackPrefAlone) {
  CreateInstaller();
  const std::string language = GetLanguageName(LanguageCode::kEnUs);
  const base::ListValue before =
      local_state()->GetList(::prefs::kSodaRegisteredLanguagePacks).Clone();
  // Otherwise upstream's UnregisterLanguage would have nothing to erase and
  // this would pass without asserting anything.
  ASSERT_FALSE(before.empty());

  installer_->RegisterLanguage(language, local_state());
  installer_->UnregisterLanguage(language, local_state());
  installer_->Init(local_state(), local_state());

  EXPECT_EQ(before,
            local_state()->GetList(::prefs::kSodaRegisteredLanguagePacks));
}

// ---------- An install that is already done ----------

// Tests that a model already on disk stops the call there, instead of
// registering the component again to ask for a download of what is already
// installed. Nothing is reported either, because nothing changed, and
// upstream's SodaInstallerImpl::InstallLanguage returns the same way. The
// component updater is left unwatched too, since the early return comes before
// the observation and there is no download to hear about.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageWithModelDoesNothing) {
  PublishModel();
  CreateInstaller();
  ASSERT_TRUE(installer_->IsSodaInstalled(LanguageCode::kEnUs));

  EXPECT_CALL(*cus_, RegisterComponent(testing::_)).Times(0);
  EXPECT_CALL(*cus_, AddObserver(testing::_)).Times(0);
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(ComponentId(), testing::_))
      .Times(0);

  InstallLanguage();
  task_environment_.RunUntilIdle();

  EXPECT_EQ(0, observer_.installed_count());
  EXPECT_EQ(0, observer_.error_count());
}

// ---------- Settling a parked reply ----------

// Tests that a request that cannot be acted on is reported as a failure rather
// than left to hang. The reply is parked before InstallLanguage is called, so
// every path out of it has to report.
TEST_F(BraveSodaInstallerUnitTest, InstallLanguageWhenSwitchedOffReportsError) {
  CreateInstaller();
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(local_ai::kBraveOnDeviceSpeechRecognition);

  InstallLanguage();
  EXPECT_EQ(0, observer_.error_count());

  task_environment_.RunUntilIdle();
  EXPECT_EQ(1, observer_.error_count());
  EXPECT_EQ(0, observer_.installed_count());
}

// ---------- Watching the component updater ----------

// Tests that the component updater is watched only once a download has been
// asked for. Doing it from the constructor would build the component updater
// inside BrowserProcessImpl::PreMainMessageLoopRun, earlier than upstream
// does.
TEST_F(BraveSodaInstallerUnitTest, ObservesOnlyFromInstallLanguage) {
  EXPECT_CALL(*cus_, AddObserver(testing::_)).Times(0);
  CreateInstaller();
  testing::Mock::VerifyAndClearExpectations(cus_);

  EXPECT_CALL(*cus_, AddObserver(testing::_)).Times(1);
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(ComponentId(), testing::_))
      .Times(testing::AnyNumber());
  InstallLanguage();
  task_environment_.RunUntilIdle();
}

// Tests that a second request does not observe twice, which would report one
// failed download as two failures.
TEST_F(BraveSodaInstallerUnitTest, ObservesOnlyOnce) {
  CreateInstaller();

  EXPECT_CALL(*cus_, AddObserver(testing::_)).Times(1);
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(ComponentId(), testing::_))
      .Times(testing::AnyNumber());

  InstallLanguage();
  task_environment_.RunUntilIdle();
  InstallLanguage();
  task_environment_.RunUntilIdle();
}

// Tests that events about other components are ignored. OnEvent is called for
// every component in the browser, so the id is the only thing separating this
// component's failures from everyone else's.
TEST_F(BraveSodaInstallerUnitTest, IgnoresOtherComponentsFailures) {
  CreateInstaller();

  SendEvent(update_client::ComponentState::kUpdateError,
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  task_environment_.RunUntilIdle();

  EXPECT_EQ(0, observer_.error_count());
}

// Tests that a failed download is reported, and that nothing else about it is.
// A success arrives through ComponentReady, and the states in between are
// deliberately not reported as progress.
TEST_F(BraveSodaInstallerUnitTest, ReportsOnlyTerminalFailure) {
  CreateInstaller();
  const std::string id(local_ai::kOnDeviceSpeechModelsComponentId);

  for (auto state : {update_client::ComponentState::kChecking,
                     update_client::ComponentState::kCanUpdate,
                     update_client::ComponentState::kDownloading,
                     update_client::ComponentState::kUpdating,
                     update_client::ComponentState::kUpdated,
                     update_client::ComponentState::kUpToDate}) {
    SendEvent(state, id);
  }
  task_environment_.RunUntilIdle();

  EXPECT_EQ(0, observer_.error_count());
  EXPECT_EQ(0, observer_.installed_count());
  EXPECT_EQ(0, observer_.progress_count());

  SendEvent(update_client::ComponentState::kUpdateError, id);
  task_environment_.RunUntilIdle();

  EXPECT_EQ(1, observer_.error_count());
}

// Tests the case the component updater is watched for at all: a download
// already in flight, started by startup registration, which this request only
// joins. Its answer says nothing about how that download ends, so the failure
// has to come from the component updater, and exactly once.
TEST_F(BraveSodaInstallerUnitTest, InProgressDownloadIsSettledByItsFailure) {
  CreateInstaller();
  EXPECT_CALL(*cus_, AddObserver(testing::_)).Times(1);
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(ComponentId(), testing::_))
      .WillOnce(
          [](const std::string& id, component_updater::Callback callback) {
            std::move(callback).Run(update_client::Error::UPDATE_IN_PROGRESS);
          });

  InstallLanguage();
  task_environment_.RunUntilIdle();

  // Joining a download in progress is not itself a failure.
  EXPECT_EQ(0, observer_.error_count());

  SendEvent(update_client::ComponentState::kUpdateError, ComponentId());
  task_environment_.RunUntilIdle();

  EXPECT_EQ(1, observer_.error_count());
}

// Tests the same download succeeding instead. The install dir being published
// is what reports it, so nothing is expected from the component updater.
TEST_F(BraveSodaInstallerUnitTest, InProgressDownloadIsSettledByItsSuccess) {
  CreateInstaller();
  EXPECT_CALL(*cus_, RegisterComponent(testing::_))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(on_demand_updater_, EnsureInstalled(ComponentId(), testing::_))
      .WillOnce(
          [](const std::string& id, component_updater::Callback callback) {
            std::move(callback).Run(update_client::Error::UPDATE_IN_PROGRESS);
          });

  InstallLanguage();
  task_environment_.RunUntilIdle();
  ASSERT_EQ(0, observer_.error_count());

  PublishModel();

  EXPECT_EQ(1, observer_.installed_count());
  EXPECT_EQ(0, observer_.error_count());
}

}  // namespace speech
