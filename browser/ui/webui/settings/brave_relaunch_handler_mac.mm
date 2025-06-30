/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_relaunch_handler_mac.h"

#import "brave/browser/mac/sparkle_glue.h"
#include "brave/browser/updater/buildflags.h"

#if BUILDFLAG(ENABLE_OMAHA4)
#include "brave/browser/updater/features.h"
#endif

namespace brave_relaunch_handler {

bool RelaunchOnMac() {
#if BUILDFLAG(ENABLE_OMAHA4)
  if (brave_updater::ShouldUseOmaha4()) {
    return false;
  }
#endif
  return [SparkleGlue sharedSparkleGlue] &&
         [[SparkleGlue sharedSparkleGlue] relaunch];
}

}  // namespace brave_relaunch_handler
