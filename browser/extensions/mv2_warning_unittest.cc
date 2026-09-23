/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/values.h"
#include "brave/browser/brave_browser_main_extra_parts.h"
#include "chrome/browser/extensions/extension_service_test_with_install.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

class Mv2WarningUnitTest : public testing::Test {
 public:
  Mv2WarningUnitTest() {}
  ~Mv2WarningUnitTest() override = default;
};

TEST(Mv2WarningTest, ExtensionManifestVersions) {
  auto main_extra_parts = BraveBrowserMainExtraParts();
  main_extra_parts.PreProfileInit();

  auto get_manifest = [](std::optional<int> manifest_version) {
    base::DictValue dict;
    dict.Set("name", "My Extension");
    dict.Set("version", "0.1");
    dict.Set("description", "An awesome extension");
    if (manifest_version) {
      dict.Set("manifest_version", *manifest_version);
    }
    return dict;
  };

  std::u16string error;
  scoped_refptr<extensions::Extension> extension =
      extensions::Extension::Create(
          base::FilePath(), extensions::mojom::ManifestLocation::kUnpacked,
          get_manifest(2), extensions::Extension::InitFromValueFlags::NO_FLAGS,
          &error);

  ASSERT_NE(extension, nullptr);

  auto& warnings = extension->install_warnings();
  // MV2 deprecation warning is no longer suppressed for unplacked extensions.
  EXPECT_EQ(warnings.size(), 1UL);
  EXPECT_EQ(warnings.front().message,
            extensions::manifest_errors::kManifestV2IsDeprecatedWarning);
}

class Mv2WarningCrxUnitTest
    : public extensions::ExtensionServiceTestWithInstall {
 public:
  Mv2WarningCrxUnitTest() = default;
};

TEST_F(Mv2WarningCrxUnitTest, InstallMV2CrxWithoutWarnings) {
  InitializeEmptyExtensionService();

  base::FilePath path = data_dir().AppendASCII("good.crx");
  const extensions::Extension* extension = InstallCRX(path, INSTALL_NEW);
  ASSERT_TRUE(extension);
  ASSERT_EQ(extension->manifest_version(), 2);

  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(profile());
  ASSERT_TRUE(registry->enabled_extensions().Contains(extension->id()));

  // Verify there are no install warnings.
  EXPECT_TRUE(extension->install_warnings().empty());
  for (const auto& warning : extension->install_warnings()) {
    ADD_FAILURE() << "Install warning: " << warning.message;
  }
}
