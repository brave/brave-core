/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_FIRST_RUN_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_FIRST_RUN_SERVICE_H_

#define SetUpClientSideFieldTrialIfNeeded                        \
  SetUpClientSideFieldTrialIfNeeded_UnUsed(                      \
      const base::FieldTrial::EntropyProvider& entropy_provider, \
      base::FeatureList* feature_list);                          \
  static void SetUpClientSideFieldTrialIfNeeded

#include "src/chrome/browser/ui/startup/first_run_service.h"  // IWYU pragma: export

#undef SetUpClientSideFieldTrialIfNeeded

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_FIRST_RUN_SERVICE_H_
