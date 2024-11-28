/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "chrome/updater/util/mac_util.h"

#include "chrome/updater/updater_scope.h"

#define RemoveWakeJobFromLaunchd RemoveWakeJobFromLaunchd_Unused
#include "src/chrome/updater/util/mac_util.mm"
#undef RemoveWakeJobFromLaunchd

namespace updater {

bool RemoveWakeJobFromLaunchd(UpdaterScope scope) {
  // We must prevent Omaha 4 from running at the same time as Sparkle. We can
  // control this in the browser, where we invoke either Omaha 4 or Sparkle. But
  // we have no control over when Omaha 4's background task runs. We therefore
  // disable it by not registering the wake job. This would make the original
  // implementation of this function fail. Return true to pretend that the job
  // (which was never registered in the first place) was successfully removed:
  return true;
}

}  // namespace updater
