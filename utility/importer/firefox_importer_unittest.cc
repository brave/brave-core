/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/firefox_importer.h"
#include "brave/common/brave_paths.h"
#include "brave/common/importer/brave_mock_importer_bridge.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/common/importer/mock_importer_bridge.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

base::FilePath GetTestFirefoxProfileDir(const std::string& profile) {
  base::FilePath test_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_dir);

  return test_dir.AppendASCII("import").AppendASCII("firefox")
      .AppendASCII(profile);
}

class FirefoxImporterTest : public ::testing::Test {
 protected:
  void SetUpFirefoxProfile() {
    // Creates a new profile in a new subdirectory in the temp directory.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath test_path = temp_dir_.GetPath().AppendASCII("FirefoxImporterTest");
    base::DeleteFile(test_path, true);
    base::CreateDirectory(test_path);
    profile_dir_ = test_path.AppendASCII("profile");

    base::FilePath data_dir = GetTestFirefoxProfileDir("default");
    ASSERT_TRUE(base::DirectoryExists(data_dir));
    ASSERT_TRUE(base::CopyDirectory(data_dir, profile_dir_, true));

    profile_.source_path = profile_dir_;
  }

  void SetUp() override {
    SetUpFirefoxProfile();
    importer_ = new brave::FirefoxImporter;
    bridge_ = new BraveMockImporterBridge;
  }

  base::ScopedTempDir temp_dir_;
  base::FilePath profile_dir_;
  importer::SourceProfile profile_;
  scoped_refptr<brave::FirefoxImporter> importer_;
  scoped_refptr<BraveMockImporterBridge> bridge_;
};

TEST_F(FirefoxImporterTest, ImportCookies) {
  std::vector<net::CanonicalCookie> cookies;

  EXPECT_CALL(*bridge_, NotifyStarted());
  EXPECT_CALL(*bridge_, NotifyItemStarted(importer::COOKIES));
  EXPECT_CALL(*bridge_, SetCookies(_))
      .WillOnce(::testing::SaveArg<0>(&cookies));
  EXPECT_CALL(*bridge_, NotifyItemEnded(importer::COOKIES));
  EXPECT_CALL(*bridge_, NotifyEnded());

  importer_->StartImport(profile_, importer::COOKIES, bridge_.get());

  ASSERT_EQ(1u, cookies.size());
  EXPECT_EQ(".localhost", cookies[0].Domain());
  EXPECT_EQ("test", cookies[0].Name());
  EXPECT_EQ("test", cookies[0].Value());
}
