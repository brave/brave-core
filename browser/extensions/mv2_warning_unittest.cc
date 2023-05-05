/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/values.h"
#include "brave/browser/brave_browser_main_extra_parts.h"
#include "extensions/common/extension.h"
#include "testing/gtest/include/gtest/gtest.h"

class Mv2WarningUnitTest : public testing::Test {
 public:
  Mv2WarningUnitTest() {}
  ~Mv2WarningUnitTest() override = default;
};

TEST(Mv2WarningTest, ExtensionManifestVersions) {
  auto main_extra_parts = BraveBrowserMainExtraParts();
  main_extra_parts.PreProfileInit();

  auto get_manifest = [](absl::optional<int> manifest_version) {
    base::Value::Dict dict;
    dict.Set("name", "My Extension");
    dict.Set("version", "0.1");
    dict.Set("description", "An awesome extension");
    if (manifest_version) {
      dict.Set("manifest_version", *manifest_version);
    }
    return dict;
  };

  std::string error;
  scoped_refptr<extensions::Extension> extension =
      extensions::Extension::Create(
          base::FilePath(), extensions::mojom::ManifestLocation::kUnpacked,
          get_manifest(2), extensions::Extension::InitFromValueFlags::NO_FLAGS,
          &error);

  ASSERT_NE(extension, nullptr);

  auto& warnings = extension->install_warnings();
  EXPECT_EQ(warnings.size(), 0UL);
}
