/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/util/mac_util.h"

// We must prevent Omaha 4 from running at the same time as Sparkle. We can
// control this in the browser, where we invoke either Omaha 4 or Sparkle. But
// we have no control over when Omaha 4's background updates run. We therefore
// disable them by not registering the wake job:
#define GetWakeTaskPlistPath(scope) \
  std::nullopt;                     \
  return true;
#include "src/chrome/updater/mac/setup/setup.mm"
#undef GetWakeTaskPlistPath
