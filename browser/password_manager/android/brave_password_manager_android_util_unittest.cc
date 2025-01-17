/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/build_info.h"
#include "base/files/file_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_file_util.h"
#include "chrome/browser/password_manager/android/password_manager_android_util.h"
#include "components/password_manager/core/browser/features/password_features.h"
#include "components/password_manager/core/browser/password_manager_constants.h"
#include "components/password_manager/core/browser/split_stores_and_local_upm.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/base/data_type.h"
#include "components/sync/base/pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

using password_manager::GetLocalUpmMinGmsVersion;
using password_manager::prefs::kPasswordsUseUPMLocalAndSeparateStores;
using password_manager::prefs::UseUpmLocalAndSeparateStoresState;
using password_manager::prefs::UseUpmLocalAndSeparateStoresState::kOff;
using password_manager::prefs::UseUpmLocalAndSeparateStoresState::kOn;

namespace password_manager_android_util {
namespace {

class BravePasswordManagerAndroidUtilTest : public testing::Test {
 public:
  BravePasswordManagerAndroidUtilTest() {
    pref_service_.registry()->RegisterBooleanPref(
        password_manager::prefs::kUnenrolledFromGoogleMobileServicesDueToErrors,
        false);
    pref_service_.registry()->RegisterIntegerPref(
        password_manager::prefs::kCurrentMigrationVersionToGoogleMobileServices,
        0);
    pref_service_.registry()->RegisterIntegerPref(
        password_manager::prefs::kPasswordsUseUPMLocalAndSeparateStores,
        static_cast<int>(kOff));
    pref_service_.registry()->RegisterBooleanPref(
        password_manager::prefs::kCredentialsEnableService, false);
    pref_service_.registry()->RegisterBooleanPref(
        password_manager::prefs::kCredentialsEnableAutosignin, false);
    pref_service_.registry()->RegisterBooleanPref(
        password_manager::prefs::kEmptyProfileStoreLoginDatabase, false);
    pref_service_.registry()->RegisterBooleanPref(
        syncer::prefs::internal::kSyncInitialSyncFeatureSetupComplete, false);
    pref_service_.registry()->RegisterBooleanPref(
        syncer::prefs::internal::kSyncKeepEverythingSynced, false);
    pref_service_.registry()->RegisterBooleanPref(
        base::StrCat(
            {syncer::prefs::internal::
                 kSyncDataTypeStatusForSyncToSigninMigrationPrefix,
             ".", syncer::DataTypeToStableLowerCaseString(syncer::PASSWORDS)}),
        false);
    pref_service_.registry()->RegisterBooleanPref(
        password_manager::prefs::
            kUserAcknowledgedLocalPasswordsMigrationWarning,
        false);
    pref_service_.registry()->RegisterBooleanPref(
        password_manager::prefs::kSettingsMigratedToUPMLocal, false);

    base::WriteFile(login_db_directory_.Append(
                        password_manager::kLoginDataForProfileFileName),
                    "");

    // Most tests check the modern GmsCore case.
    base::android::BuildInfo::GetInstance()->set_gms_version_code_for_test(
        base::NumberToString(GetLocalUpmMinGmsVersion()));
  }

  TestingPrefServiceSimple* pref_service() { return &pref_service_; }

  const base::FilePath& login_db_directory() { return login_db_directory_; }

 private:
  TestingPrefServiceSimple pref_service_;
  const base::FilePath login_db_directory_ =
      base::CreateUniqueTempDirectoryScopedToTest();
};

// Based on Chromium's PasswordManagerAndroidUtilTest.
// SetUsesSplitStoresAndUPMForLocal_DeletesLoginDataFilesForMigratedUsers.
// We don't migrate and don't delete passwords DB.
TEST_F(BravePasswordManagerAndroidUtilTest,
       SetUsesSplitStoresAndUPMForLocal_DeletesLoginDataFilesForMigratedUsers) {
  base::test::ScopedFeatureList feature_list(
      password_manager::features::kClearLoginDatabaseForAllMigratedUPMUsers);

  // This is a state of a local user that has just been migrated.
  pref_service()->SetInteger(kPasswordsUseUPMLocalAndSeparateStores,
                             static_cast<int>(kOn));
  pref_service()->SetBoolean(
      password_manager::prefs::kUserAcknowledgedLocalPasswordsMigrationWarning,
      true);
  pref_service()->SetBoolean(
      password_manager::prefs::kEmptyProfileStoreLoginDatabase, false);

  // Creating the login data files for testing.
  base::FilePath profile_db_path = login_db_directory().Append(
      password_manager::kLoginDataForProfileFileName);
  base::FilePath account_db_path = login_db_directory().Append(
      password_manager::kLoginDataForAccountFileName);
  base::FilePath profile_db_journal_path = login_db_directory().Append(
      password_manager::kLoginDataJournalForProfileFileName);
  base::FilePath account_db_journal_path = login_db_directory().Append(
      password_manager::kLoginDataJournalForAccountFileName);

  base::WriteFile(profile_db_path, "Test content");
  base::WriteFile(account_db_path, "Test content");
  base::WriteFile(profile_db_journal_path, "Test content");
  base::WriteFile(account_db_journal_path, "Test content");

  EXPECT_TRUE(PathExists(profile_db_path));
  EXPECT_TRUE(PathExists(account_db_path));
  EXPECT_TRUE(PathExists(profile_db_journal_path));
  EXPECT_TRUE(PathExists(account_db_journal_path));

  SetUsesSplitStoresAndUPMForLocal(pref_service(), login_db_directory());

  // Pref should be kOff, as we want keep using profile store instead of the
  // account store, so the paswords will be synced from Android to Desktop
  EXPECT_EQ(pref_service()->GetInteger(kPasswordsUseUPMLocalAndSeparateStores),
            static_cast<int>(kOff));

  // SetUsesSplitStoresAndUPMForLocal must not delete DB on Brave
  EXPECT_TRUE(PathExists(profile_db_path));
  EXPECT_TRUE(PathExists(account_db_path));
  EXPECT_TRUE(PathExists(profile_db_journal_path));
  EXPECT_TRUE(PathExists(account_db_journal_path));
}

}  // namespace
}  // namespace password_manager_android_util
