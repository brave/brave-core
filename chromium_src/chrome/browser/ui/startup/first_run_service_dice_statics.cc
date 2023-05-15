/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/first_run_service.h"

#define SetUpClientSideFieldTrialIfNeeded \
  SetUpClientSideFieldTrialIfNeeded_UnUsed

#include "src/chrome/browser/ui/startup/first_run_service_dice_statics.cc"

#undef SetUpClientSideFieldTrialIfNeeded

// static
void FirstRunService::SetUpClientSideFieldTrialIfNeeded(
    const base::FieldTrial::EntropyProvider& entropy_provider,
    base::FeatureList* feature_list) {
  // Don't override for kForYouFre at client side.
  // Use default value.
}
