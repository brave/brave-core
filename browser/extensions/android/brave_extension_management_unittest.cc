/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/android/brave_extension_management.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "chrome/browser/flags/android/chrome_feature_list.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/management_policy.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/mojom/manifest.mojom-shared.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

scoped_refptr<const Extension> BuildExtension(
    mojom::ManifestLocation location) {
  return ExtensionBuilder("test").SetLocation(location).Build();
}

}  // namespace

class BraveExtensionManagementAndroidTestBase : public testing::Test {
 protected:
  BraveExtensionManagementAndroidTestBase(
      const std::vector<base::test::FeatureRef>& enabled,
      const std::vector<base::test::FeatureRef>& disabled) {
    feature_list_.InitWithFeatures(enabled, disabled);
  }

  const ManagementPolicy::Provider& provider() {
    if (!management_) {
      management_ = std::make_unique<BraveExtensionManagement>(&profile_);
    }
    return *management_->GetProviders().back();
  }

  bool UserMayLoad(mojom::ManifestLocation location) {
    std::u16string error;
    return provider().UserMayLoad(BuildExtension(location).get(), &error);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  std::unique_ptr<BraveExtensionManagement> management_;
};

class BraveExtensionManagementAndroidDisabledTest
    : public BraveExtensionManagementAndroidTestBase {
 protected:
  BraveExtensionManagementAndroidDisabledTest()
      : BraveExtensionManagementAndroidTestBase(
            {},
            {features::kBraveAndroidExtensions}) {}
};

TEST_F(BraveExtensionManagementAndroidDisabledTest, BlocksExtensions) {
  EXPECT_FALSE(AreAndroidExtensionsAllowed());
  EXPECT_FALSE(UserMayLoad(mojom::ManifestLocation::kInternal));
  EXPECT_FALSE(UserMayLoad(mojom::ManifestLocation::kUnpacked));

  auto extension = BuildExtension(mojom::ManifestLocation::kInternal);
  disable_reason::DisableReason reason = disable_reason::DISABLE_NONE;
  EXPECT_TRUE(provider().MustRemainDisabled(extension.get(), &reason));
  EXPECT_EQ(disable_reason::DISABLE_BLOCKED_BY_POLICY, reason);
}

TEST_F(BraveExtensionManagementAndroidDisabledTest, AllowsComponents) {
  EXPECT_TRUE(UserMayLoad(mojom::ManifestLocation::kComponent));

  auto extension = BuildExtension(mojom::ManifestLocation::kComponent);
  EXPECT_FALSE(provider().MustRemainDisabled(extension.get(), nullptr));
}

class BraveExtensionManagementAndroidNoLoadAllTabsTest
    : public BraveExtensionManagementAndroidTestBase {
 protected:
  BraveExtensionManagementAndroidNoLoadAllTabsTest()
      : BraveExtensionManagementAndroidTestBase(
            {features::kBraveAndroidExtensions},
            {chrome::android::kLoadAllTabsAtStartup}) {}
};

TEST_F(BraveExtensionManagementAndroidNoLoadAllTabsTest, BlocksExtensions) {
  EXPECT_FALSE(AreAndroidExtensionsAllowed());
  EXPECT_FALSE(UserMayLoad(mojom::ManifestLocation::kInternal));
}

class BraveExtensionManagementAndroidEnabledTest
    : public BraveExtensionManagementAndroidTestBase {
 protected:
  BraveExtensionManagementAndroidEnabledTest()
      : BraveExtensionManagementAndroidTestBase(
            {features::kBraveAndroidExtensions, features::kWebContentsDiscard,
             features::kLazyBrowserInterfaceBroker,
             chrome::android::kLoadAllTabsAtStartup},
            {}) {}
};

TEST_F(BraveExtensionManagementAndroidEnabledTest, AllowsExtensions) {
  EXPECT_TRUE(AreAndroidExtensionsAllowed());
  EXPECT_TRUE(UserMayLoad(mojom::ManifestLocation::kInternal));
  EXPECT_TRUE(UserMayLoad(mojom::ManifestLocation::kUnpacked));

  auto extension = BuildExtension(mojom::ManifestLocation::kInternal);
  EXPECT_FALSE(provider().MustRemainDisabled(extension.get(), nullptr));
}

}  // namespace extensions
