/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/strcat.h"
#include "base/test/test_file_util.h"
#include "chrome/browser/password_manager/android/mock_password_manager_util_bridge.h"
#include "chrome/browser/password_manager/android/password_manager_android_util.h"
#include "components/password_manager/core/browser/password_manager_buildflags.h"
#include "components/password_manager/core/browser/password_manager_constants.h"
#include "components/password_manager/core/browser/split_stores_and_local_upm.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/base/data_type.h"
#include "components/sync/base/pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_manager_android_util {
namespace {

class BravePasswordManagerAndroidUtilTest : public testing::Test {
 public:
  BravePasswordManagerAndroidUtilTest() {
    pref_service_.registry()->RegisterBooleanPref(
        password_manager::prefs::kCredentialsEnableService, false);
    pref_service_.registry()->RegisterBooleanPref(
        password_manager::prefs::kCredentialsEnableAutosignin, false);
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

    base::WriteFile(login_db_directory_.Append(
                        password_manager::kLoginDataForProfileFileName),
                    "");
  }

  TestingPrefServiceSimple* pref_service() { return &pref_service_; }

  const base::FilePath& login_db_directory() { return login_db_directory_; }

  std::unique_ptr<MockPasswordManagerUtilBridge>
  GetMockBridgeWithBackendPresent() {
    auto mock_bridge = std::make_unique<MockPasswordManagerUtilBridge>();
    ON_CALL(*mock_bridge, IsInternalBackendPresent)
        .WillByDefault(testing::Return(true));
    return mock_bridge;
  }

 private:
  TestingPrefServiceSimple pref_service_;
  const base::FilePath login_db_directory_ =
      base::CreateUniqueTempDirectoryScopedToTest();
};

// We want to ensure the Passwords DB files are not deleted on Android.
// This happens in MaybeDeleteLoginDataFiles under
// use_login_database_as_backend=false flag. We do set this flaf at
// build/commands/lib/config.js. This check is to make sure the flag is still
// set.
static_assert(BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND));

// We don't want password db to be deleted on Android
// Based on DeletesLoginDataFilesAfterUnmigratedPasswordsExported
TEST_F(BravePasswordManagerAndroidUtilTest, DoNotDeleteLoginDataFiles) {
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

  MaybeDeleteLoginDatabases(pref_service(), login_db_directory(),
                            GetMockBridgeWithBackendPresent());

  EXPECT_TRUE(PathExists(profile_db_path));
  EXPECT_TRUE(PathExists(account_db_path));
  EXPECT_TRUE(PathExists(profile_db_journal_path));
  EXPECT_TRUE(PathExists(account_db_journal_path));
}

}  // namespace
}  // namespace password_manager_android_util
