/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/android/brave_extension_management.h"

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/notreached.h"
#include "build/android_buildflags.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/management_policy.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"

#if !BUILDFLAG(IS_DESKTOP_ANDROID)
#include "brave/browser/brave_browser_features.h"
#include "chrome/browser/flags/android/chrome_feature_list.h"
#include "content/public/common/content_features.h"
#endif  // !BUILDFLAG(IS_DESKTOP_ANDROID)

namespace extensions {

namespace {

class AndroidExtensionsPolicyProvider : public ManagementPolicy::Provider {
 public:
  AndroidExtensionsPolicyProvider() = default;
  ~AndroidExtensionsPolicyProvider() override = default;

  std::string GetDebugPolicyProviderName() const override {
#if defined(NDEBUG)
    NOTREACHED();
#else
    return "Brave Android Extensions Provider";
#endif  // defined(NDEBUG)
  }

  bool UserMayLoad(const Extension* extension,
                   std::u16string* error) const override {
    return IsAllowed(*extension);
  }

  // Keeps installed extensions on disk, so they come back once allowed.
  bool MustRemainDisabled(
      const Extension* extension,
      disable_reason::DisableReason* reason) const override {
    if (IsAllowed(*extension)) {
      return false;
    }
    if (reason) {
      *reason = disable_reason::DISABLE_BLOCKED_BY_POLICY;
    }
    return true;
  }

 private:
  static bool IsAllowed(const Extension& extension) {
    return AreAndroidExtensionsAllowed() ||
           Manifest::IsComponentLocation(extension.location());
  }
};

}  // namespace

bool AreAndroidExtensionsAllowed() {
#if BUILDFLAG(IS_DESKTOP_ANDROID)
  return true;
#else
  // Mirrors ChromeBrowserFieldTrials::RegisterFeatureOverrides: the tabs API
  // CHECKs that every tab has WebContents, which kLoadAllTabsAtStartup
  // provides.
  return base::FeatureList::IsEnabled(features::kBraveAndroidExtensions) &&
         base::FeatureList::IsEnabled(features::kWebContentsDiscard) &&
         base::FeatureList::IsEnabled(features::kLazyBrowserInterfaceBroker) &&
         base::FeatureList::IsEnabled(chrome::android::kLoadAllTabsAtStartup);
#endif  // BUILDFLAG(IS_DESKTOP_ANDROID)
}

BraveExtensionManagement::BraveExtensionManagement(Profile* profile)
    : ExtensionManagement(profile) {
  providers_.push_back(std::make_unique<AndroidExtensionsPolicyProvider>());
}

BraveExtensionManagement::~BraveExtensionManagement() = default;

}  // namespace extensions
