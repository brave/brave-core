/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_FIELD_TRIALS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_FIELD_TRIALS_H_

#define ChromeBrowserFieldTrials ChromeBrowserFieldTrialsChromium

#include "src/chrome/browser/chrome_browser_field_trials.h"  // IWYU pragma: export

#undef ChromeBrowserFieldTrials

// Use empty subclass to not set client-side field trials.
class ChromeBrowserFieldTrials : public ChromeBrowserFieldTrialsChromium {
 public:
  using ChromeBrowserFieldTrialsChromium::ChromeBrowserFieldTrialsChromium;
  ~ChromeBrowserFieldTrials() override = default;

  // ChromeBrowserFieldTrialsChromium overrides:
  void SetUpClientSideFieldTrials(
      bool has_seed,
      const variations::EntropyProviders& entropy_providers,
      base::FeatureList* feature_list) override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_FIELD_TRIALS_H_
