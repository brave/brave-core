/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/brave_importer.h"
#include "brave/common/brave_paths.h"
#include "brave/common/importer/brave_mock_importer_bridge.h"
#include "brave/common/importer/brave_stats.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "base/path_service.h"
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

// In order to test the Brave import functionality effectively, we store a
// simulated Brave profile directory containing dummy data files with the
// same structure as ~/Library/Application Support/brave in the Brave
// test data directory. This function returns the path to that directory.
base::FilePath GetTestBraveProfileDir(const std::string& profile) {
  base::FilePath test_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_dir);

  return test_dir.AppendASCII("import").AppendASCII("brave-browser-laptop")
      .AppendASCII(profile);
}

class BraveImporterTest : public ::testing::Test {
 protected:
  void SetUpBraveProfile() {
    // Creates a new profile in a new subdirectory in the temp directory.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath test_path = temp_dir_.GetPath().AppendASCII("BraveImporterTest");
    base::DeleteFile(test_path, true);
    base::CreateDirectory(test_path);
    profile_dir_ = test_path.AppendASCII("profile");

    base::FilePath data_dir = GetTestBraveProfileDir("default");
    ASSERT_TRUE(base::DirectoryExists(data_dir));
    ASSERT_TRUE(base::CopyDirectory(data_dir, profile_dir_, true));

    profile_.source_path = profile_dir_;
  }

  void SetUp() override {
    SetUpBraveProfile();
    importer_ = new BraveImporter;
    bridge_ = new BraveMockImporterBridge;
  }

  base::ScopedTempDir temp_dir_;
  base::FilePath profile_dir_;
  importer::SourceProfile profile_;
  scoped_refptr<BraveImporter> importer_;
  scoped_refptr<BraveMockImporterBridge> bridge_;
};

TEST_F(BraveImporterTest, ImportHistory) {
  std::vector<ImporterURLRow> history;

  EXPECT_CALL(*bridge_, NotifyStarted());
  EXPECT_CALL(*bridge_, NotifyItemStarted(importer::HISTORY));
  EXPECT_CALL(*bridge_, SetHistoryItems(_, _))
      .WillOnce(::testing::SaveArg<0>(&history));
  EXPECT_CALL(*bridge_, NotifyItemEnded(importer::HISTORY));
  EXPECT_CALL(*bridge_, NotifyEnded());

  importer_->StartImport(profile_, importer::HISTORY, bridge_.get());

  ASSERT_EQ(10u, history.size());

  // Order of imported history entries matches the order of keys in historySites
  EXPECT_EQ(history[0].url.spec(),
      "http://127.0.0.1:8080/trigger_password_save_prompt.html");
  EXPECT_EQ(history[0].title,
      ASCIIToUTF16("127.0.0.1:8080/trigger_password_save_prompt.html"));
  EXPECT_EQ(history[0].last_visit,
      base::Time::FromJsTime(1528742510286));
  EXPECT_EQ(history[0].hidden, false);
  EXPECT_EQ(history[0].typed_count, 0);

  EXPECT_EQ(history[9].url.spec(),
      "https://www.google.com/search?q=year%202048%20bug");
  EXPECT_EQ(history[9].title,
      ASCIIToUTF16("year 2048 bug - Google Search"));
  EXPECT_EQ(history[9].last_visit,
      base::Time::FromJsTime(1528745695067));
  EXPECT_EQ(history[9].hidden, false);
  EXPECT_EQ(history[9].typed_count, 0);
}

TEST_F(BraveImporterTest, ImportBookmarks) {
  std::vector<ImportedBookmarkEntry> bookmarks;

  EXPECT_CALL(*bridge_, NotifyStarted());
  EXPECT_CALL(*bridge_, NotifyItemStarted(importer::FAVORITES));
  EXPECT_CALL(*bridge_, AddBookmarks(_, _))
      .WillOnce(::testing::SaveArg<0>(&bookmarks));
  EXPECT_CALL(*bridge_, NotifyItemEnded(importer::FAVORITES));
  EXPECT_CALL(*bridge_, NotifyEnded());

  importer_->StartImport(profile_, importer::FAVORITES, bridge_.get());

  ASSERT_EQ(6u, bookmarks.size());

  EXPECT_EQ(ASCIIToUTF16("Nested Folder 2 (Empty)"), bookmarks[0].title);
  EXPECT_EQ(2u, bookmarks[0].path.size());
  EXPECT_EQ(ASCIIToUTF16("Bookmarks Toolbar"), bookmarks[0].path[0]);
  EXPECT_EQ(ASCIIToUTF16("Nested Folder 1"), bookmarks[0].path[1]);
  EXPECT_TRUE(bookmarks[0].in_toolbar);
  EXPECT_TRUE(bookmarks[0].is_folder);

  EXPECT_EQ(ASCIIToUTF16("Features | Brave Browser"), bookmarks[1].title);
  EXPECT_EQ("https://brave.com/features/", bookmarks[1].url.spec());
  EXPECT_EQ(2u, bookmarks[1].path.size());
  EXPECT_EQ(ASCIIToUTF16("Bookmarks Toolbar"), bookmarks[1].path[0]);
  EXPECT_EQ(ASCIIToUTF16("Nested Folder 1"), bookmarks[1].path[1]);
  EXPECT_TRUE(bookmarks[1].in_toolbar);
  EXPECT_FALSE(bookmarks[1].is_folder);

  EXPECT_EQ(ASCIIToUTF16(
    "Secure, Fast & Private Web Browser with Adblocker | Brave Browser"),
    bookmarks[2].title);
  EXPECT_EQ("https://brave.com/", bookmarks[2].url.spec());
  EXPECT_EQ(1u, bookmarks[2].path.size());
  EXPECT_EQ(ASCIIToUTF16("Bookmarks Toolbar"), bookmarks[2].path[0]);
  EXPECT_TRUE(bookmarks[2].in_toolbar);
  EXPECT_FALSE(bookmarks[2].is_folder);

  EXPECT_EQ(ASCIIToUTF16("Nested Folder 2 (Empty)"), bookmarks[3].title);
  EXPECT_EQ(2u, bookmarks[3].path.size());
  EXPECT_EQ(ASCIIToUTF16("Other Bookmarks"), bookmarks[3].path[0]);
  EXPECT_EQ(ASCIIToUTF16("Nested Folder 1"), bookmarks[3].path[1]);
  EXPECT_FALSE(bookmarks[3].in_toolbar);
  EXPECT_TRUE(bookmarks[3].is_folder);

  EXPECT_EQ(ASCIIToUTF16(
    "Blog About Privacy, Adblocks & Best Browsers | Brave Browser"),
    bookmarks[4].title);
  EXPECT_EQ(2u, bookmarks[4].path.size());
  EXPECT_EQ(ASCIIToUTF16("Other Bookmarks"), bookmarks[4].path[0]);
  EXPECT_EQ(ASCIIToUTF16("Nested Folder 1"), bookmarks[4].path[1]);
  EXPECT_FALSE(bookmarks[4].in_toolbar);
  EXPECT_FALSE(bookmarks[4].is_folder);

  EXPECT_EQ(ASCIIToUTF16(
    "Make Money as a Publisher with Brave Payments | Brave Browser"),
    bookmarks[5].title);
  EXPECT_EQ(1u, bookmarks[5].path.size());
  EXPECT_EQ(ASCIIToUTF16("Other Bookmarks"), bookmarks[5].path[0]);
  EXPECT_FALSE(bookmarks[5].in_toolbar);
  EXPECT_FALSE(bookmarks[5].is_folder);
}

// The mock keychain only works on macOS, so only run this test on macOS (for now)
#if defined(OS_MACOSX)
TEST_F(BraveImporterTest, ImportPasswords) {
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
  EXPECT_EQ("test_username",
            UTF16ToASCII(autofillable_login.username_value));
  EXPECT_EQ("test_password",
            UTF16ToASCII(autofillable_login.password_value));

  EXPECT_TRUE(blacklisted_login.blacklisted_by_user);
  EXPECT_EQ("http://127.0.0.1:8081/",
            blacklisted_login.signon_realm);
  EXPECT_EQ("", UTF16ToASCII(blacklisted_login.username_value));
  EXPECT_EQ("", UTF16ToASCII(blacklisted_login.password_value));

  OSCryptMocker::TearDown();
}

TEST_F(BraveImporterTest, ImportCookies) {
  OSCryptMocker::SetUp();

  std::vector<net::CanonicalCookie> cookies;

  EXPECT_CALL(*bridge_, NotifyStarted());
  EXPECT_CALL(*bridge_, NotifyItemStarted(importer::COOKIES));
  EXPECT_CALL(*bridge_, SetCookies(_))
      .WillOnce(::testing::SaveArg<0>(&cookies));
  EXPECT_CALL(*bridge_, NotifyItemEnded(importer::COOKIES));
  EXPECT_CALL(*bridge_, NotifyEnded());

  importer_->StartImport(profile_, importer::COOKIES, bridge_.get());

  ASSERT_EQ(1u, cookies.size());
  EXPECT_EQ("localhost", cookies[0].Domain());
  EXPECT_EQ("test", cookies[0].Name());
  EXPECT_EQ("test", cookies[0].Value());

  OSCryptMocker::TearDown();
}
#endif

TEST_F(BraveImporterTest, ImportStats) {
  BraveStats stats;

  EXPECT_CALL(*bridge_, NotifyStarted());
  EXPECT_CALL(*bridge_, NotifyItemStarted(importer::STATS));
  EXPECT_CALL(*bridge_, UpdateStats(_))
      .WillOnce(::testing::SaveArg<0>(&stats));
  EXPECT_CALL(*bridge_, NotifyItemEnded(importer::STATS));
  EXPECT_CALL(*bridge_, NotifyEnded());

  importer_->StartImport(profile_, importer::STATS, bridge_.get());

  EXPECT_EQ(9, stats.adblock_count);
  EXPECT_EQ(0, stats.trackingProtection_count);
  EXPECT_EQ(0, stats.httpsEverywhere_count);
}

TEST_F(BraveImporterTest, ImportLedger) {
  // TODO
}
