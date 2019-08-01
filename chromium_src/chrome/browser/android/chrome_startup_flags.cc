/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define SetChromeSpecificCommandLineFlags \
  SetChromeSpecificCommandLineFlags_ChromiumImpl
#include "../../../../../chrome/browser/android/chrome_startup_flags.cc"
#undef SetChromeSpecificCommandLineFlags
#include "base/base_switches.h"
#include "services/network/public/cpp/features.h"

void SetChromeSpecificCommandLineFlags() {
  SetChromeSpecificCommandLineFlags_ChromiumImpl();

  SetCommandLineSwitchASCII(switches::kDisableFeatures,
    network::features::kNetworkService.name);
}
