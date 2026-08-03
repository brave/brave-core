/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_features.h"

#include <array>

#include "base/metrics/field_trial_params.h"

namespace features {

BASE_FEATURE(kBraveFocusMode, base::FEATURE_DISABLED_BY_DEFAULT);

namespace {

constexpr auto kFocusModeUrlDisplayOptions =
    std::to_array<base::FeatureParam<FocusModeUrlDisplay>::Option>(
        {{FocusModeUrlDisplay::kNone, "none"},
         {FocusModeUrlDisplay::kTitleBar, "title-bar"},
         {FocusModeUrlDisplay::kMiniToolbar, "mini-toolbar"}});

constexpr base::FeatureParam<FocusModeUrlDisplay> kFocusModeUrlDisplay{
    &kBraveFocusMode, "url-display", FocusModeUrlDisplay::kTitleBar,
    kFocusModeUrlDisplayOptions};

}  // namespace

FocusModeUrlDisplay GetFocusModeUrlDisplay() {
  return kFocusModeUrlDisplay.Get();
}

}  // namespace features
