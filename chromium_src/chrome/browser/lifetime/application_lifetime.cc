/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
#include "brave/browser/lifetime/brave_application_lifetime_mac.h"

#define AttemptRestart AttemptRestart_ChromiumImpl
#define RelaunchIgnoreUnloadHandlers RelaunchIgnoreUnloadHandlers_ChromiumImpl
#endif

#include "src/chrome/browser/lifetime/application_lifetime.cc"

#if BUILDFLAG(IS_MAC)
#undef RelaunchIgnoreUnloadHandlers
#undef AttemptRestart

namespace chrome {

void AttemptRestart() {
  if (!brave::AttemptRestartOnMac())
    AttemptRestart_ChromiumImpl();
}

void RelaunchIgnoreUnloadHandlers() {
  if (!brave::AttemptRestartOnMac())
    RelaunchIgnoreUnloadHandlers_ChromiumImpl();
}

}  // namespace chrome

#endif
