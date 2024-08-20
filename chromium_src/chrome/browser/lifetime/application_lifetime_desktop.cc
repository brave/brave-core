/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/lifetime/application_lifetime_desktop.h"

#include "brave/browser/ui/webui/settings/brave_relaunch_handler_mac.h"

#define AttemptRestart AttemptRestart_ChromiumImpl
#include "src/chrome/browser/lifetime/application_lifetime_desktop.cc"
#undef AttemptRestart

namespace chrome {

void AttemptRestart() {
#if BUILDFLAG(IS_MAC)
  if (brave_relaunch_handler::RelaunchOnMac()) {
    return;
  }
#endif
  return AttemptRestart_ChromiumImpl();
}

}  // namespace chrome
