/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/chrome_importer.h"
#include "brave/common/brave_paths.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/common/importer/importer_url_row.h"
#include "chrome/common/importer/mock_importer_bridge.h"
#include "components/favicon_base/favicon_usage_data.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;
using ::testing::_;

// In order to test the Chrome import functionality effectively, we store a
// simulated Chrome profile directory containing dummy data files with the
// same structure as ~/Library/Application Support/Google/Chrome in the Brave
// test data directory. This function returns the path to that directory.
base::FilePath GetTestChromeProfileDir(const std::string& profile) {
  base::FilePath test_dir;
  PathService::Get(brave::DIR_TEST_DATA, &test_dir);

  return test_dir.AppendASCII("import").AppendASCII("chrome")
      .AppendASCII(profile);
}

class ChromeImporterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    profile_dir_ = GetTestChromeProfileDir("default");
    ASSERT_TRUE(base::DirectoryExists(profile_dir_));
    profile_.source_path = profile_dir_;
    importer_ = new ChromeImporter;
    bridge_ = new MockImporterBridge;
  }

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

  ASSERT_EQ(4u, history.size());
  EXPECT_EQ("https://brave.com/", history[0].url.spec());
  EXPECT_EQ("https://github.com/brave", history[1].url.spec());
  EXPECT_EQ("https://nytimes.com/", history[2].url.spec());
  EXPECT_EQ("https://www.nytimes.com/", history[3].url.spec());
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
