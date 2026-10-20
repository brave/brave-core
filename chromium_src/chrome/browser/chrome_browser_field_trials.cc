/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_field_trials.h"

#include "build/build_config.h"
#include "extensions/buildflags/buildflags.h"

#define ChromeBrowserFieldTrials ChromeBrowserFieldTrialsChromium

#include <chrome/browser/chrome_browser_field_trials.cc>

#undef ChromeBrowserFieldTrials

#if BUILDFLAG(IS_ANDROID) && BUILDFLAG(ENABLE_EXTENSIONS_CORE)
// Implemented in
// brave/browser/extensions/android/brave_extension_field_trials.cc, which
// owns the kBraveAndroidExtensions check and the Brave feature deps.
void BraveRegisterFeatureOverrides(base::FeatureList* feature_list);
#endif  // BUILDFLAG(IS_ANDROID) && BUILDFLAG(ENABLE_EXTENSIONS_CORE)

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
  BraveRegisterFeatureOverrides(feature_list);
#endif  // BUILDFLAG(IS_ANDROID) && BUILDFLAG(ENABLE_EXTENSIONS_CORE)
}
