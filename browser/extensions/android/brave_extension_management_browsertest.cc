/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/android/brave_extension_management.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/flags/android/chrome_feature_list.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"

namespace extensions {

namespace {

base::FilePath GetTestExtensionPath() {
  base::ScopedAllowBlockingForTesting allow_blocking;
  return base::PathService::CheckedGet(brave::DIR_TEST_DATA)
      .AppendASCII("extensions")
      .AppendASCII("trivial_extension");
}

}  // namespace

// Only the Brave flag is enabled; the companion features must come from the
// startup overrides.
class BraveExtensionManagementAndroidEnabledBrowserTest
    : public ExtensionBrowserTest {
 public:
  BraveExtensionManagementAndroidEnabledBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveAndroidExtensions);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveExtensionManagementAndroidEnabledBrowserTest,
                       StartupOverridesEnableCompanionFeatures) {
  EXPECT_TRUE(base::FeatureList::IsEnabled(features::kWebContentsDiscard));
  EXPECT_TRUE(
      base::FeatureList::IsEnabled(features::kLazyBrowserInterfaceBroker));
  EXPECT_TRUE(
      base::FeatureList::IsEnabled(chrome::android::kLoadAllTabsAtStartup));
  EXPECT_TRUE(AreAndroidExtensionsAllowed());
}

IN_PROC_BROWSER_TEST_F(BraveExtensionManagementAndroidEnabledBrowserTest,
                       LoadsUnpackedExtension) {
  const Extension* extension = LoadExtension(GetTestExtensionPath());
  ASSERT_TRUE(extension);
  EXPECT_TRUE(ExtensionRegistry::Get(profile())->enabled_extensions().Contains(
      extension->id()));
}

class BraveExtensionManagementAndroidDisabledBrowserTest
    : public ExtensionBrowserTest {
 public:
  BraveExtensionManagementAndroidDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kBraveAndroidExtensions);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveExtensionManagementAndroidDisabledBrowserTest,
                       NotAllowed) {
  EXPECT_FALSE(AreAndroidExtensionsAllowed());
}

IN_PROC_BROWSER_TEST_F(BraveExtensionManagementAndroidDisabledBrowserTest,
                       UnpackedExtensionInstallIsBlocked) {
  ChromeTestExtensionLoader loader(profile());
  loader.set_should_fail(true);
  EXPECT_FALSE(loader.LoadExtension(GetTestExtensionPath()));
}

IN_PROC_BROWSER_TEST_F(BraveExtensionManagementAndroidDisabledBrowserTest,
                       ComponentExtensionStillLoads) {
  const Extension* extension = LoadExtensionAsComponent(GetTestExtensionPath());
  ASSERT_TRUE(extension);
  EXPECT_TRUE(ExtensionRegistry::Get(profile())->enabled_extensions().Contains(
      extension->id()));
}

}  // namespace extensions
