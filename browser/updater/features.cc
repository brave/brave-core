/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/features.h"

#include <optional>

#include "base/logging.h"
#include "base/time/time.h"

namespace brave_updater {

BASE_FEATURE(kBraveUseOmaha4,
             "BraveUseOmaha4",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE_PARAM(int,
                   kLegacyFallbackIntervalDays,
                   &kBraveUseOmaha4,
                   "legacy-fallback-interval-days",
                   5);

namespace {

// We cache the result of ShouldUseOmaha4() to ensure that it stays constant
// across multiple calls.
std::optional<bool> g_use_omaha4;

bool ShouldUseOmaha4Impl(base::Time now, std::optional<bool>& state) {
  if (!state.has_value()) {
    // Whether Omaha 4 should be used is mostly determined by the feature flag.
    // However, we also want to give the legacy implementation a chance to run
    // every X days. This lets us recover from a situation where updates with
    // Omaha 4 are broken because of a bug. Once Omaha 4 is stable, we can
    // remove the periodic fallback.
    int days_since_null = (now - base::Time()).InDays();
    int legacy_fallback_interval_days = kLegacyFallbackIntervalDays.Get();
    if (days_since_null % legacy_fallback_interval_days == 0) {
      state = false;
    } else {
      state = base::FeatureList::IsEnabled(kBraveUseOmaha4);
    }
    VLOG(1) << "Using Omaha 4: " << state.value();
  }
  return state.value();
}

}  // namespace

bool ShouldUseOmaha4() {
  return ShouldUseOmaha4Impl(base::Time::Now(), g_use_omaha4);
}

bool ShouldUseOmaha4ForTesting(base::Time now, std::optional<bool>& state) {
  return ShouldUseOmaha4Impl(now, state);
}

}  // namespace brave_updater
