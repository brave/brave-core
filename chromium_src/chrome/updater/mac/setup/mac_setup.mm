/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/util/mac_util.h"

// We are in the process of migrating from Sparkle to Omaha 4. The two operate
// differently: Sparkle checks for updates in the browser, while Omaha 4
// registers a wake job that runs periodically in the background, also when the
// browser isn't running. We must prevent Omaha 4 and Sparkle from running at
// the same time. We do this here by preventing Omaha 4's wake job from being
// registered, and instead invoke its on-demand API from inside the browser
// instead of Sparkle. Once the migration to Sparkle is complete and we have
// background updates on macOS, we should re-enable the wake job by removing
// this code and releasing a new version of Omaha 4 itself. When we do this, we
// should also re-enable the outdated build detector in
// upgrade_detector_impl.cc.
#define GetWakeTaskPlistPath(scope) \
  std::nullopt;                     \
  return true;
#include <chrome/updater/mac/setup/mac_setup.mm>
#undef GetWakeTaskPlistPath
