/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/updater/updater_p3a_facade.h"

#include "base/time/time.h"
#include "brave/browser/mac_features.h"
#include "brave/updater/updater_p3a.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_constants.h"

namespace brave_updater {

void ReportLaunch() {
  ReportLaunch(
      base::Time::Now(),
      chrome::kChromeVersion,
      brave::ShouldUseOmaha4(),
      g_browser_process->local_state());
}

}  // namespace brave_updater
