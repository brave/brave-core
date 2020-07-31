/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/chrome_importer.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/brave_paths.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/common/importer/importer_url_row.h"
#include "chrome/common/importer/mock_importer_bridge.h"
#include "components/favicon_base/favicon_usage_data.h"
#include "components/os_crypt/os_crypt_mocker.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;
using base::UTF16ToASCII;
using ::testing::_;

// In order to test the Chrome import functionality effectively, we store a
// simulated Chrome profile directory containing dummy data files with the
// same structure as ~/Library/Application Support/Google/Chrome in the Brave
// test data directory. This function returns the path to that directory.
base::FilePath GetTestChromeProfileDir(const std::string& profile) {
  base::FilePath test_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_dir);

  return test_dir.AppendASCII("import").AppendASCII("chrome")
      .AppendASCII(profile);
}

class ChromeImporterTest : public ::testing::Test {
 protected:
  void SetUpChromeProfile() {
    // Creates a new profile in a new subdirectory in the temp directory.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath test_path =
        temp_dir_.GetPath().AppendASCII("ChromeImporterTest");
    base::DeletePathRecursively(test_path);
    base::CreateDirectory(test_path);
    profile_dir_ = test_path.AppendASCII("profile");

    base::FilePath data_dir = GetTestChromeProfileDir("default");
    ASSERT_TRUE(base::DirectoryExists(data_dir));
    ASSERT_TRUE(base::CopyDirectory(data_dir, profile_dir_, true));

    profile_.source_path = profile_dir_;
  }

  void SetUp() override {
    SetUpChromeProfile();
    importer_ = new ChromeImporter;
    bridge_ = new MockImporterBridge;
  }

  base::ScopedTempDir temp_dir_;
  base::FilePath profile_dir_;
  importer::SourceProfile profile_;
  scoped_refptr<ChromeImporter> importer_;
  scoped_refptr<MockImporterBridge> bridge_;
};

TEST_F(ChromeImporterTest, ImportHistory) {
  std::vector<ImporterURLRow> history;

  EXPECT_CALL(*bridge_, NotifyStarted());
  EXPECT_CALL(*bridge_, NotifyItemStarted(importer::HISTORY));
  EXPECT_CALL(*bridge_, SetHistoryItems(_, _))
      .WillOnce(::testing::SaveArg<0>(&history));
  EXPECT_CALL(*bridge_, NotifyItemEnded(importer::HISTORY));
  EXPECT_CALL(*bridge_, NotifyEnded());

  importer_->StartImport(profile_, importer::HISTORY, bridge_.get());

  ASSERT_EQ(3u, history.size());
  EXPECT_EQ("https://brave.com/", history[0].url.spec());
  EXPECT_EQ("https://github.com/brave", history[1].url.spec());
  EXPECT_EQ("https://www.nytimes.com/", history[2].url.spec());
}

TEST_F(ChromeImporterTest, ImportBookmarks) {
  std::vector<ImportedBookmarkEntry> bookmarks;

  EXPECT_CALL(*bridge_, NotifyStarted());
  EXPECT_CALL(*bridge_, NotifyItemStarted(importer::FAVORITES));
  EXPECT_CALL(*bridge_, AddBookmarks(_, _))
      .WillOnce(::testing::SaveArg<0>(&bookmarks));
  EXPECT_CALL(*bridge_, NotifyItemEnded(importer::FAVORITES));
  EXPECT_CALL(*bridge_, NotifyEnded());

  importer_->StartImport(profile_, importer::FAVORITES, bridge_.get());

  ASSERT_EQ(3u, bookmarks.size());

  EXPECT_EQ("https://www.nytimes.com/", bookmarks[0].url.spec());
  EXPECT_TRUE(bookmarks[0].in_toolbar);

  // Test importing an empty folder
  EXPECT_EQ("", bookmarks[1].url.spec());
  EXPECT_TRUE(bookmarks[1].in_toolbar);
  EXPECT_TRUE(bookmarks[1].is_folder);
  EXPECT_EQ(ASCIIToUTF16("Empty"), bookmarks[1].title);

  EXPECT_EQ("https://brave.com/", bookmarks[2].url.spec());
  EXPECT_FALSE(bookmarks[2].in_toolbar);
}

TEST_F(ChromeImporterTest, ImportFavicons) {
  favicon_base::FaviconUsageDataList favicons;

  EXPECT_CALL(*bridge_, NotifyStarted());
  EXPECT_CALL(*bridge_, NotifyItemStarted(importer::FAVORITES));
  EXPECT_CALL(*bridge_, SetFavicons(_))
      .WillOnce(::testing::SaveArg<0>(&favicons));
  EXPECT_CALL(*bridge_, NotifyItemEnded(importer::FAVORITES));
  EXPECT_CALL(*bridge_, NotifyEnded());

  importer_->StartImport(profile_, importer::FAVORITES, bridge_.get());

  ASSERT_EQ(4u, favicons.size());
  EXPECT_EQ("https://www.google.com/favicon.ico",
            favicons[0].favicon_url.spec());
  EXPECT_EQ("https://brave.com/images/cropped-brave_appicon_release-32x32.png",
            favicons[1].favicon_url.spec());
  EXPECT_EQ("https://assets-cdn.github.com/favicon.ico",
            favicons[2].favicon_url.spec());
  EXPECT_EQ("https://static.nytimes.com/favicon.ico",
            favicons[3].favicon_url.spec());
}

// The mock keychain only works on macOS, so only run this test on macOS (for
// now)
#if defined(OS_MACOSX)
TEST_F(ChromeImporterTest, ImportPasswords) {
  // Use mock keychain on mac to prevent blocking permissions dialogs.
  OSCryptMocker::SetUp();

  autofill::PasswordForm autofillable_login;
  autofill::PasswordForm blacklisted_login;

  EXPECT_CALL(*bridge_, NotifyStarted());
  EXPECT_CALL(*bridge_, NotifyItemStarted(importer::PASSWORDS));
  EXPECT_CALL(*bridge_, SetPasswordForm(_))
      .WillOnce(::testing::SaveArg<0>(&autofillable_login))
      .WillOnce(::testing::SaveArg<0>(&blacklisted_login));
  EXPECT_CALL(*bridge_, NotifyItemEnded(importer::PASSWORDS));
  EXPECT_CALL(*bridge_, NotifyEnded());

  importer_->StartImport(profile_, importer::PASSWORDS, bridge_.get());

  EXPECT_FALSE(autofillable_login.blacklisted_by_user);
  EXPECT_EQ("http://127.0.0.1:8080/",
            autofillable_login.signon_realm);
  EXPECT_EQ("test-autofillable-login",
            UTF16ToASCII(autofillable_login.username_value));
  EXPECT_EQ("autofillable-login-password",
            UTF16ToASCII(autofillable_login.password_value));

  EXPECT_TRUE(blacklisted_login.blacklisted_by_user);
  EXPECT_EQ("http://127.0.0.1:8081/",
            blacklisted_login.signon_realm);
  EXPECT_EQ("", UTF16ToASCII(blacklisted_login.username_value));
  EXPECT_EQ("", UTF16ToASCII(blacklisted_login.password_value));

  OSCryptMocker::TearDown();
}

#endif
