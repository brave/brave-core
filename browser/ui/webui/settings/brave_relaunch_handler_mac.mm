/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_relaunch_handler_mac.h"

#import "brave/browser/mac/sparkle_glue.h"
#include "brave/browser/mac_features.h"

namespace brave_relaunch_handler {

bool RelaunchOnMac() {
  return !brave::ShouldUseOmaha4() && [SparkleGlue sharedSparkleGlue] &&
         [[SparkleGlue sharedSparkleGlue] relaunch];
}

}  // namespace brave_relaunch_handler