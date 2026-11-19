/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "chrome/browser/flags/android/chrome_feature_list.h"
#include "components/variations/feature_overrides.h"
#include "content/public/common/content_features.h"

void BraveRegisterFeatureOverrides(base::FeatureList* feature_list) {
  // The FeatureList isn't initialized yet, so read the flag as about:flags
  // passes it.
  if (!feature_list->IsFeatureOverriddenFromCommandLine(
          features::kBraveAndroidExtensions.name,
          base::FeatureList::OVERRIDE_ENABLE_FEATURE)) {
    return;
  }

  // The extensions tabs APIs expect every tab to have WebContents, as
  // upstream guarantees on desktop Android.
  variations::FeatureOverrides feature_overrides(*feature_list);
  feature_overrides.EnableFeature(features::kWebContentsDiscard);
  feature_overrides.EnableFeature(features::kLazyBrowserInterfaceBroker);
  feature_overrides.EnableFeature(chrome::android::kLoadAllTabsAtStartup);
}
