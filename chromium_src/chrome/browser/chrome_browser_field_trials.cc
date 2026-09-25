/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_field_trials.h"

#include "build/build_config.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(IS_ANDROID) && BUILDFLAG(ENABLE_EXTENSIONS_CORE)
#include "brave/browser/brave_browser_features.h"
#endif  // BUILDFLAG(IS_ANDROID) && BUILDFLAG(ENABLE_EXTENSIONS_CORE)

#define ChromeBrowserFieldTrials ChromeBrowserFieldTrialsChromium

#include <chrome/browser/chrome_browser_field_trials.cc>

#undef ChromeBrowserFieldTrials

void ChromeBrowserFieldTrials::SetUpClientSideFieldTrials(
    bool has_seed,
    const variations::EntropyProviders& entropy_providers,
    base::FeatureList* feature_list) {
  // Don't setup upstream's client-side field trials.
}

void ChromeBrowserFieldTrials::RegisterFeatureOverrides(
    base::FeatureList* feature_list) {
  ChromeBrowserFieldTrialsChromium::RegisterFeatureOverrides(feature_list);

#if BUILDFLAG(IS_ANDROID) && BUILDFLAG(ENABLE_EXTENSIONS_CORE)
  // The FeatureList isn't initialized yet, so read the flag as about:flags
  // passes it.
  if (!feature_list->IsFeatureOverriddenFromCommandLine(
          features::kBraveAndroidExtensions.name,
          base::FeatureList::OVERRIDE_ENABLE_FEATURE)) {
    return;
  }

  // The extensions tabs APIs expect every tab to have WebContents, as upstream
  // guarantees on desktop Android.
  variations::FeatureOverrides feature_overrides(*feature_list);
  feature_overrides.EnableFeature(features::kWebContentsDiscard);
  feature_overrides.EnableFeature(features::kLazyBrowserInterfaceBroker);
  feature_overrides.EnableFeature(chrome::android::kLoadAllTabsAtStartup);
#endif  // BUILDFLAG(IS_ANDROID) && BUILDFLAG(ENABLE_EXTENSIONS_CORE)
}
