/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/brave_widevine_bundle_manager.h"

#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "brave/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/cdm_registry.h"
#include "content/public/common/cdm_info.h"
#include "content/public/test/browser_task_environment.h"
#include "content/test/test_content_client.h"
#include "media/base/decrypt_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"
#include "widevine_cdm_version.h"  // NOLINT

class TestClient : public content::TestContentClient {
 public:
  TestClient() {}
  ~TestClient() override {}

  void AddContentDecryptionModules(
      std::vector<content::CdmInfo>* cdms,
      std::vector<media::CdmHostFilePath>* cdm_host_file_paths) override {
    // Clear at every test case.
    // If not, previously set info is remained because CdmRegistry is global
    // instance.
    cdms->clear();

    if (empty_cdms_) return;

    content::CdmCapability capability;
    capability.encryption_schemes.insert(media::EncryptionMode::kCenc);
    cdms->push_back(
        content::CdmInfo(std::string(), base::Token(), base::Version(),
                         base::FilePath(), std::string(),
                         capability, kWidevineKeySystem, false));
  }

  void set_empty_cdms(bool empty) { empty_cdms_ = empty; }

  bool empty_cdms_ = true;
};

class BraveWidevineBundleManagerTest : public testing::Test {
 public:
  BraveWidevineBundleManagerTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {
  }
  ~BraveWidevineBundleManagerTest() override {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(testing_profile_manager_.SetUp(temp_dir_.GetPath()));
    manager_.is_test_ = true;
  }

  void PrepareCdmRegistry(bool empty_cdms) {
    client_.set_empty_cdms(empty_cdms);
    SetContentClient(&client_);
    content::CdmRegistry::GetInstance()->Init();
  }

  void PrepareTest(bool empty_cdms) {
    PrepareCdmRegistry(empty_cdms);
  }

  PrefService* pref_service() {
    return ProfileManager::GetActiveUserProfile()->GetPrefs();
  }

  void CheckPrefsStatesAreInitialState() {
    EXPECT_EQ(initial_opted_in_value_,
              pref_service()->GetBoolean(kWidevineOptedIn));
    EXPECT_EQ(initial_version_string_,
              pref_service()->GetString(kWidevineInstalledVersion));
  }

  void CheckPrefsStatesAreInstalledState() {
    EXPECT_EQ(true, pref_service()->GetBoolean(kWidevineOptedIn));
    EXPECT_EQ(WIDEVINE_CDM_VERSION_STRING,
              pref_service()->GetString(kWidevineInstalledVersion));
  }

  content::TestBrowserThreadBundle threads_;
  BraveWidevineBundleManager manager_;
  TestingProfileManager testing_profile_manager_;
  base::ScopedTempDir temp_dir_;
  TestClient client_;
  bool initial_opted_in_value_ = false;
  std::string initial_version_string_ =
      BraveWidevineBundleManager::kWidevineInvalidVersion;
};

TEST_F(BraveWidevineBundleManagerTest, InitialPrefsTest) {
  PrepareTest(true);

  CheckPrefsStatesAreInitialState();
}

TEST_F(BraveWidevineBundleManagerTest, InitialWithCdmRestoredTest) {
  PrepareTest(false);

  CheckPrefsStatesAreInitialState();

  // When widevine is registered, prefs are restored.
  manager_.StartupCheck();
  CheckPrefsStatesAreInstalledState();
}

TEST_F(BraveWidevineBundleManagerTest, PrefsResetTestWithEmptyCdmRegistry) {
  PrepareTest(true);

  // When only prefs are set w/o cdm library, reset prefs to initial state.
  pref_service()->SetBoolean(kWidevineOptedIn, true);
  pref_service()->SetString(kWidevineInstalledVersion,
                            WIDEVINE_CDM_VERSION_STRING);

  manager_.StartupCheck();
  CheckPrefsStatesAreInitialState();
}

TEST_F(BraveWidevineBundleManagerTest, InProgressTest) {
  PrepareTest(true);

  manager_.StartupCheck();
  manager_.InstallWidevineBundle(base::BindOnce([](const std::string&) {}),
                                 false);
  EXPECT_TRUE(manager_.in_progress());

  manager_.InstallDone("");
  EXPECT_FALSE(manager_.in_progress());
}

TEST_F(BraveWidevineBundleManagerTest, InstallSuccessTest) {
  PrepareTest(true);

  EXPECT_FALSE(manager_.needs_restart());

  manager_.StartupCheck();
  manager_.InstallWidevineBundle(base::BindOnce([](const std::string&) {}),
                                 false);

  manager_.InstallDone("");

  EXPECT_TRUE(manager_.needs_restart());
  CheckPrefsStatesAreInitialState();

  manager_.WillRestart();
  CheckPrefsStatesAreInstalledState();
}

TEST_F(BraveWidevineBundleManagerTest, RetryInstallAfterFail) {
  PrepareTest(true);

  EXPECT_FALSE(manager_.needs_restart());
  manager_.StartupCheck();
  manager_.InstallWidevineBundle(base::BindOnce([](const std::string&) {}),
                                 false);

  manager_.InstallDone("failed");

  EXPECT_FALSE(manager_.needs_restart());
  CheckPrefsStatesAreInitialState();

  // Check request install again goes in-progress state.
  manager_.InstallWidevineBundle(base::BindOnce([](const std::string&) {}),
                                 false);
  EXPECT_TRUE(manager_.in_progress());
}

TEST_F(BraveWidevineBundleManagerTest, DownloadFailTest) {
  PrepareTest(true);

  manager_.StartupCheck();
  manager_.InstallWidevineBundle(base::BindOnce([](const std::string&) {}),
                                 false);
  EXPECT_TRUE(manager_.in_progress());

  // Empty path means download fail.
  manager_.OnBundleDownloaded(base::FilePath());
  EXPECT_FALSE(manager_.in_progress());
  CheckPrefsStatesAreInitialState();
}

TEST_F(BraveWidevineBundleManagerTest, UnzipFailTest) {
  PrepareTest(true);

  manager_.StartupCheck();
  manager_.InstallWidevineBundle(base::BindOnce([](const std::string&) {}),
                                 false);
  EXPECT_TRUE(manager_.in_progress());

  manager_.OnBundleUnzipped("unzip failed");
  EXPECT_FALSE(manager_.in_progress());
  CheckPrefsStatesAreInitialState();
}

TEST_F(BraveWidevineBundleManagerTest, UpdateTriggerTest) {
  PrepareTest(false);

  // Set installed state with different version to trigger update.
  pref_service()->SetBoolean(kWidevineOptedIn, true);
  pref_service()->SetString(kWidevineInstalledVersion, "1.0.0.0");


  EXPECT_FALSE(manager_.update_requested_);

  manager_.StartupCheck();
  EXPECT_TRUE(manager_.update_requested_);
  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("");

  CheckPrefsStatesAreInstalledState();
}

// Test whether prev prefs are persisted after update failure.
TEST_F(BraveWidevineBundleManagerTest, UpdateFailTest) {
  PrepareTest(false);

  initial_opted_in_value_ = true;
  initial_version_string_ = "1.0.0.0";

  // Set installed state with different version to trigger update.
  pref_service()->SetBoolean(kWidevineOptedIn, initial_opted_in_value_);
  pref_service()->SetString(kWidevineInstalledVersion, initial_version_string_);

  manager_.StartupCheck();
  manager_.DoDelayedBackgroundUpdate();
  // Non empty string means failure - it's error message.
  manager_.InstallDone("failed");

  CheckPrefsStatesAreInitialState();
}

TEST_F(BraveWidevineBundleManagerTest, UpdateRetryAndFinallyFailedTest) {
  PrepareTest(false);

  initial_opted_in_value_ = true;
  initial_version_string_ = "1.0.0.0";

  // Set installed state with different version to trigger update.
  pref_service()->SetBoolean(kWidevineOptedIn, initial_opted_in_value_);
  pref_service()->SetString(kWidevineInstalledVersion, initial_version_string_);

  manager_.StartupCheck();

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("failed");
  EXPECT_EQ(1, manager_.background_update_retry_);

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("failed");
  EXPECT_EQ(2, manager_.background_update_retry_);

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("failed");
  EXPECT_EQ(3, manager_.background_update_retry_);

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("failed");
  EXPECT_EQ(4, manager_.background_update_retry_);

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("failed");
  EXPECT_EQ(5, manager_.background_update_retry_);

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("failed");
  // No retry anymore after five trying.
  EXPECT_NE(6, manager_.background_update_retry_);

  CheckPrefsStatesAreInitialState();
}

TEST_F(BraveWidevineBundleManagerTest, UpdateRetryAndFinallySuccessTest) {
  PrepareTest(false);

  initial_opted_in_value_ = true;
  initial_version_string_ = "1.0.0.0";

  // Set installed state with different version to trigger update.
  pref_service()->SetBoolean(kWidevineOptedIn, initial_opted_in_value_);
  pref_service()->SetString(kWidevineInstalledVersion, initial_version_string_);

  manager_.StartupCheck();

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("failed");
  EXPECT_EQ(1, manager_.background_update_retry_);

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("failed");
  EXPECT_EQ(2, manager_.background_update_retry_);

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("failed");
  EXPECT_EQ(3, manager_.background_update_retry_);

  manager_.DoDelayedBackgroundUpdate();
  manager_.InstallDone("");

  // No retry after install success.
  EXPECT_EQ(3, manager_.background_update_retry_);

  CheckPrefsStatesAreInstalledState();
}

TEST_F(BraveWidevineBundleManagerTest, MessageStringTest) {
  PrepareTest(true);

  EXPECT_FALSE(manager_.needs_restart());
  EXPECT_EQ(IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_INSTALL,
            manager_.GetWidevinePermissionRequestTextFragment());

  manager_.set_needs_restart(true);
  EXPECT_EQ(IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_RESTART_BROWSER,
            manager_.GetWidevinePermissionRequestTextFragment());
}
