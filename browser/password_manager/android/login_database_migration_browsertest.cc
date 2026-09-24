/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/android/android_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_test.h"

class AndroidLoginDatabaseMigrationTest : public AndroidBrowserTest {
 public:
  AndroidLoginDatabaseMigrationTest() = default;
  ~AndroidLoginDatabaseMigrationTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    AndroidBrowserTest::SetUpInProcessBrowserTestFixture();
    create_services_subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterCreateServicesCallbackForTesting(
                base::BindRepeating(&AndroidLoginDatabaseMigrationTest::
                                        OnWillCreateBrowserContextServices,
                                    base::Unretained(this)));
  }

  void OnWillCreateBrowserContextServices(content::BrowserContext* context) {
    if (context->IsOffTheRecord()) {
      // Off the record is created 2nd, at that moment LoginDatabase cleaned up
      // the malformed file we created for test, so quit here
      return;
    }

    // At this point MigrateObsoleteProfilePrefs which used to delete Passwords
    // database on Android happened. Ensure the inerested files were not removed
    EXPECT_TRUE(PathExists(profile_db_path_));
    EXPECT_TRUE(PathExists(account_db_path_));
    EXPECT_TRUE(PathExists(profile_db_journal_path_));
    EXPECT_TRUE(PathExists(account_db_journal_path_));
  }

  base::CallbackListSubscription create_services_subscription_;

 protected:
  bool SetUpUserDataDirectory() override {
    // Create passwords database files with stub content to ensure later they
    // are not deleted
    base::FilePath profile_dir;
    base::PathService::Get(chrome::DIR_USER_DATA, &profile_dir);
    profile_dir = profile_dir.AppendASCII(TestingProfile::kTestUserProfileDir);
    base::CreateDirectory(profile_dir);

    profile_db_path_ = profile_dir.Append(FILE_PATH_LITERAL("Login Data"));
    account_db_path_ =
        profile_dir.Append(FILE_PATH_LITERAL("Login Data For Account"));
    profile_db_journal_path_ =
        profile_dir.Append(FILE_PATH_LITERAL("Login Data-journal"));
    account_db_journal_path_ =
        profile_dir.Append(FILE_PATH_LITERAL("Login Data For Account-journal"));

    base::WriteFile(profile_db_path_, "Test content");
    base::WriteFile(account_db_path_, "Test content");
    base::WriteFile(profile_db_journal_path_, "Test content");
    base::WriteFile(account_db_journal_path_, "Test content");
    EXPECT_TRUE(PathExists(profile_db_path_));
    EXPECT_TRUE(PathExists(account_db_path_));
    EXPECT_TRUE(PathExists(profile_db_journal_path_));
    EXPECT_TRUE(PathExists(account_db_journal_path_));
    return true;
  }

  base::FilePath profile_db_path_;
  base::FilePath account_db_path_;
  base::FilePath profile_db_journal_path_;
  base::FilePath account_db_journal_path_;
};

IN_PROC_BROWSER_TEST_F(AndroidLoginDatabaseMigrationTest,
                       LoginDbFilesAreKeptOnMigration) {
  // The actual test happens above at OnWillCreateBrowserContextServices.
  // passwords db files must not be not deleted after
  // MigrateObsoleteProfilePrefs
  EXPECT_TRUE(true);
}
