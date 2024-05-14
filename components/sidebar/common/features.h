/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_SIDEBAR_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace sidebar::features {

// Whether to show the sidebar always on stable channel.
BASE_DECLARE_FEATURE(kSidebarShowAlwaysOnStable);

// Whether to open the Leo panel only once.
extern const base::FeatureParam<bool> kOpenOneShotLeoPanel;

enum class SidebarDefaultMode {
  kOff = 0,
  kAlwaysOn,
  kOnOneShot,
  kMaxValue = kOnOneShot
};

SidebarDefaultMode GetSidebarDefaultMode();

}  // namespace sidebar::features

#endif  // BRAVE_COMPONENTS_SIDEBAR_COMMON_FEATURES_H_
