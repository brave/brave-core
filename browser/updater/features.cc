/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/features.h"

#include "base/build_time.h"
#include "base/time/time.h"
#include "build/build_config.h"

namespace brave_updater {

// DO NOT TURN THIS FEATURE ON IN PRODUCTION. As of this writing, it only
// implements the happy path of switching from Sparkle to Omaha 4 on macOS. It
// does not handle switching from Omaha 4 back to Sparkle. When you do enable
// the feature in the future, make sure that it is not enabled for any clients
// that suffer from the above limitations.
BASE_FEATURE(kBraveUseOmaha4Alpha,
             "BraveUseOmaha4Alpha",
             base::FEATURE_DISABLED_BY_DEFAULT);

int GetBuildAgeInDays() {
  base::Time build_time = base::GetBuildTime();
  base::Time now = base::Time::Now();

  base::TimeDelta time_since_build = now - build_time;
  return time_since_build.InDays();
}

bool ShouldUseOmaha4(int build_age_days) {
  // Give the legacy implementation a chance to run every 5 days. As the
  // reference time, we use the build time (instead of, say the time of first
  // run or installation) for the following reasons:
  //   1. It can be obtained without I/O.
  //   2. It synchronizes users in a system-wide installation.
  // The second point helps prevent race conditions between the legacy and
  // Omaha 4 implementations. Further work is needed to achieve this fully.
  // See https://github.com/brave/brave-browser/issues/47795.

  // Use `== 4` instead of `== 0` to first give the Omaha 4 implementation a
  // chance to run after an update.
  if (build_age_days % 5 == 4) {
    return false;
  }
  return base::FeatureList::IsEnabled(kBraveUseOmaha4Alpha);
}

bool ShouldUseOmaha4() {
  return ShouldUseOmaha4(GetBuildAgeInDays());
}

}  // namespace brave_updater
