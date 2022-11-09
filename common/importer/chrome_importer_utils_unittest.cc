/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/importer/chrome_importer_utils.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "brave/common/importer/importer_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kTestExtensionsPreferencesFile[] =
    "Secure_Preferences_for_extension_import";
base::FilePath GetTestProfilePath() {
  base::FilePath test_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_dir);
  return test_dir.AppendASCII("import").AppendASCII("chrome").AppendASCII(
      "default");
}
// This sample prefs file is fetched after installing two extensions and one
// theme from webstore with fresh profile.
base::FilePath GetTestPreferencesPath() {
  base::FilePath test_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_dir);
  return test_dir.AppendASCII("import")
      .AppendASCII("chrome")
      .AppendASCII("default")
      .AppendASCII(kTestExtensionsPreferencesFile);
}
}  // namespace

TEST(ChromeImporterUtilsTest, ExtensionImportTest) {
  base::FilePath data_path;
  ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, &data_path));
  base::FilePath secured_preference_path = GetTestPreferencesPath();

  std::string secured_preference_content;
  base::ReadFileToString(secured_preference_path, &secured_preference_content);
  absl::optional<base::Value> secured_preference =
      base::JSONReader::Read(secured_preference_content);
  ASSERT_TRUE(secured_preference);
  ASSERT_TRUE(secured_preference->is_dict());
  auto* extensions = secured_preference->GetDict().FindDictByDottedPath(
      kChromeExtensionsListPath);
  ASSERT_TRUE(extensions);
  auto extensions_list =
      GetImportableListFromChromeExtensionsList(*extensions);
  // Only 1 extension installed from webstore is importing target extension.
  // We don't import theme, pre-installed extensions, disabled extensions and
  // installed by default.
  EXPECT_EQ(1UL, extensions_list.size());
}

TEST(ChromeImporterUtilsTest, GetChromeUserDataFolder) {
  EXPECT_EQ(
      GetChromeSourceProfiles(base::FilePath(FILE_PATH_LITERAL("fake"))),
      base::JSONReader::Read(R"([{"id": "", "name": "Default" }])")->GetList());

  EXPECT_EQ(GetChromeSourceProfiles(GetTestProfilePath().Append(
                base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
            base::JSONReader::Read(R"([
        {"id": "Default", "name": "Profile 1"},
        {"id": "Profile 2",  "name": "Profile 2"}
      ])")
                ->GetList());

  EXPECT_EQ(
      GetChromeSourceProfiles(
          GetTestProfilePath().Append(base::FilePath::StringType(
              FILE_PATH_LITERAL("No Profile Local State")))),
      base::JSONReader::Read(R"([{"id": "", "name": "Default" }])")->GetList());

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
