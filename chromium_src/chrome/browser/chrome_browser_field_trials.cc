/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_field_trials.h"

#define ChromeBrowserFieldTrials ChromeBrowserFieldTrialsChromium

#include "src/chrome/browser/chrome_browser_field_trials.cc"

#undef ChromeBrowserFieldTrials

void ChromeBrowserFieldTrials::SetUpClientSideFieldTrials(
    bool has_seed,
    const variations::EntropyProviders& entropy_providers,
    base::FeatureList* feature_list) {
  // Don't setup upstream's client-side field trials.
}
