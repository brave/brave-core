// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/common/importer/chrome_importer_utils.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "brave/common/importer/importer_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/common/importer/importer_data_types.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveChromeImporterUtilsTest : public testing::Test {
 public:
  BraveChromeImporterUtilsTest() {}

  void SetUp() override {
    EXPECT_TRUE(brave_profile_dir_.CreateUniqueTempDir());
    base::CreateDirectory(GetTestProfilePath());
  }
  base::FilePath GetTestProfilePath() {
    return brave_profile_dir_.GetPath().AppendASCII("Chrome").AppendASCII(
        "Default");
  }
  void CopyTestFileToProfile(const std::string& source,
                             const std::string& target) {
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    base::CopyFile(test_data_dir.AppendASCII("import")
                       .AppendASCII("chrome")
                       .AppendASCII("default")
                       .AppendASCII(source),
                   GetTestProfilePath().AppendASCII(target));
  }

 private:
  base::ScopedTempDir brave_profile_dir_;
};

TEST_F(BraveChromeImporterUtilsTest, GetChromeExtensionsListPreferences) {
  CopyTestFileToProfile(kChromePreferencesFile, kChromePreferencesFile);
  auto extensions_list =
      GetImportableChromeExtensionsList(GetTestProfilePath());
  EXPECT_TRUE(extensions_list);
  EXPECT_EQ(extensions_list->size(), 2u);
  EXPECT_EQ(extensions_list.value(),
            std::vector<std::string>({"jldhpllghnbhlbpcmnajkpdmadaolakh",
                                      "mefhakmgclhhfbdadeojlkbllmecialg"}));
}

TEST_F(BraveChromeImporterUtilsTest, GetChromeExtensionsListSecurePreferences) {
  CopyTestFileToProfile("Secure_Preferences_for_extension_import",
                        kChromeSecurePreferencesFile);
  auto extensions_list =
      GetImportableChromeExtensionsList(GetTestProfilePath());
  EXPECT_TRUE(extensions_list);
  EXPECT_EQ(extensions_list->size(), 1u);
  EXPECT_EQ(extensions_list.value(),
            std::vector<std::string>({"aeblfdkhhhdcdjpifhhbdiojplfjncoa"}));
}

TEST_F(BraveChromeImporterUtilsTest, ExtensionImportTest) {
  CopyTestFileToProfile("Secure_Preferences_for_extension_import",
                        kChromeSecurePreferencesFile);
  CopyTestFileToProfile("Preferences", "Preferences");
  auto extensions_list =
      GetImportableChromeExtensionsList(GetTestProfilePath());
  EXPECT_TRUE(extensions_list);
  // Only 3 extension installed from webstore is importing target extension.
  // We don't import theme, pre-installed extensions, disabled extensions and
  // installed by default (1 from Secure Preferences, 1 from Preferences)
  EXPECT_EQ(3u, extensions_list->size());
  EXPECT_EQ(extensions_list.value(),
            std::vector<std::string>({"aeblfdkhhhdcdjpifhhbdiojplfjncoa",
                                      "jldhpllghnbhlbpcmnajkpdmadaolakh",
                                      "mefhakmgclhhfbdadeojlkbllmecialg"}));
}

TEST_F(BraveChromeImporterUtilsTest, GetChromeUserDataFolder) {
  CopyTestFileToProfile("Local State", "Local State");

  EXPECT_EQ(
      GetChromeSourceProfiles(base::FilePath(FILE_PATH_LITERAL("fake"))),
      base::JSONReader::Read(R"([{"id": "", "name": "Default" }])")->GetList());

  EXPECT_EQ(GetChromeSourceProfiles(GetTestProfilePath().Append(
                base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
            base::JSONReader::Read(R"([
        {"id": "Default", "name": "Profile 1"},
        {"id": "Profile 2", "name": "Profile 2"}
      ])")
                ->GetList());
  CopyTestFileToProfile("No Profile Local State", "No Profile Local State");
  EXPECT_EQ(
      GetChromeSourceProfiles(
          GetTestProfilePath().Append(base::FilePath::StringType(
              FILE_PATH_LITERAL("No Profile Local State")))),
      base::JSONReader::Read(R"([{"id": "", "name": "Default" }])")->GetList());

  CopyTestFileToProfile("Local State With Avatar", "Local State With Avatar");
  EXPECT_EQ(GetChromeSourceProfiles(
                GetTestProfilePath().Append(base::FilePath::StringType(
                    FILE_PATH_LITERAL("Local State With Avatar")))),
            base::JSONReader::Read(R"([
        {
          "id": "Default",
          "name": "Profile 1",
          "last_active": true,
          "avatar_icon": "chrome://theme/IDR_PROFILE_AVATAR_26",
          "active_time": 1663746595.898419
        },
        {
          "id": "Profile 2",
          "name": "Profile 2",
          "last_active": false
        }
      ])")
                ->GetList());
}

TEST_F(BraveChromeImporterUtilsTest, ChromeImporterCanImport) {
  CopyTestFileToProfile("Secure_Preferences_for_extension_import",
                        kChromeSecurePreferencesFile);
  CopyTestFileToProfile(kChromePreferencesFile, kChromePreferencesFile);
  uint16_t services_supported = importer::NONE;
  EXPECT_TRUE(
      ChromeImporterCanImport(GetTestProfilePath(), &services_supported));
  EXPECT_EQ(services_supported, importer::EXTENSIONS);
}

TEST_F(BraveChromeImporterUtilsTest, BadFiles) {
  CopyTestFileToProfile("non_json_preferences", kChromeSecurePreferencesFile);
  CopyTestFileToProfile("non_json_preferences", kChromePreferencesFile);
  uint16_t services_supported = importer::NONE;
  EXPECT_FALSE(
      ChromeImporterCanImport(GetTestProfilePath(), &services_supported));
  EXPECT_EQ(services_supported, importer::NONE);

  CopyTestFileToProfile("non_dict_extension", kChromeSecurePreferencesFile);
  CopyTestFileToProfile("non_dict_extension", kChromePreferencesFile);
  services_supported = importer::NONE;
  // Empty list is anyway considered as something to import.
  EXPECT_TRUE(
      ChromeImporterCanImport(GetTestProfilePath(), &services_supported));
  EXPECT_EQ(services_supported, importer::EXTENSIONS);
}
