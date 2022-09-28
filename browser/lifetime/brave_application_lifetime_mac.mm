/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/lifetime/brave_application_lifetime_mac.h"

#include "brave/browser/sparkle_buildflags.h"

#if BUILDFLAG(ENABLE_SPARKLE)
#import "brave/browser/mac/sparkle_glue.h"
#endif

namespace brave {

bool AttemptRestartOnMac() {
#if BUILDFLAG(ENABLE_SPARKLE)
  if (!sparkle_glue::SparkleEnabled())
    return false;

  SparkleGlue* sparkle_glue = [SparkleGlue sharedSparkleGlue];
  // Only relaunch via sparkle when update is available.
  if ([sparkle_glue recentStatus] == kAutoupdateInstalled) {
    [sparkle_glue relaunch];
    return true;
  }
  return false;
#else
  return false;
#endif
}

}  // namespace brave
