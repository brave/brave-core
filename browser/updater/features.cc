/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/features.h"

#include <optional>

#include "base/time/time.h"

namespace {
// We cache the result of ShouldUseOmaha4() to ensure that it stays constant
// across multiple calls.
std::optional<bool> g_use_omaha4;
}  // namespace

namespace brave_updater {

// DO NOT TURN THIS FEATURE ON IN PRODUCTION. As of this writing, it only
// implements the happy path of switching from Sparkle to Omaha 4 on macOS. It
// does not handle switching from Omaha 4 back to Sparkle. When you do enable
// the feature in the future, make sure that it is not enabled for any clients
// that suffer from the above limitations.
BASE_FEATURE(kBraveUseOmaha4Alpha,
             "BraveUseOmaha4Alpha",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool ShouldUseOmaha4(base::Time now, std::optional<bool>& state) {
  if (!state.has_value()) {
    int days_since_null = (now - base::Time()).InDays();
    // Whether Omaha 4 should be used is mostly determined by the feature flag.
    // However, we also want to give the legacy implementation a chance to run
    // every 5 days. This lets us recover from a situation where updates with
    // Omaha 4 are broken because of a bug. Once Omaha 4 is stable, we can
    // remove the periodic fallback.
    state = base::FeatureList::IsEnabled(kBraveUseOmaha4Alpha) &&
            days_since_null % 5;
  }
  return state.value();
}

bool ShouldUseOmaha4() {
  return ShouldUseOmaha4(base::Time::Now(), g_use_omaha4);
}

}  // namespace brave_updater
