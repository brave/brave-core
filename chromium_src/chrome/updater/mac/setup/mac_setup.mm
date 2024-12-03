/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/util/mac_util.h"

// We are in the process of migrating from Sparkle to Omaha 4. The two operate
// differently: Sparkle checks for updates in the browser, while Omaha 4
// registers a wake job that runs periodically in the background, also when the
// browser isn't running. We must prevent Omaha 4 and Sparkle from running at
// the same time. We do this by not registering Omaha 4's wake job, and instead
// invoking its on-demand API from inside the browser instead of Sparkle. Once
// the migration to Sparkle is complete, we will re-enable the wake job by
// releasing a new version of Omaha 4 itself.
#define GetWakeTaskPlistPath(scope) \
  std::nullopt;                     \
  return true;
#include "src/chrome/updater/mac/setup/mac_setup.mm"
#undef GetWakeTaskPlistPath
