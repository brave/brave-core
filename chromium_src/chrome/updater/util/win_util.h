/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_UPDATER_UTIL_WIN_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_UPDATER_UTIL_WIN_UTIL_H_

#include <chrome/updater/util/win_util.h>  // IWYU pragma: export

#include <string>

#include "chrome/updater/registration_data.h"
#include "chrome/updater/updater_scope.h"

namespace updater {

// Derives `registration.cohort` from its Omaha 3 `ap` value during
// MigrateLegacyUpdaters. See chromium_src/chrome/updater/util/win_util.cc.
void MigrateApToCohort(RegistrationRequest& registration);

// The name under which our Omaha 3 fork creates the shutdown event. See
// SignalShutdownEvent in chromium_src/chrome/updater/util/win_util.cc.
std::wstring GetLegacyShutdownEventName(UpdaterScope scope);

}  // namespace updater

#endif  // BRAVE_CHROMIUM_SRC_CHROME_UPDATER_UTIL_WIN_UTIL_H_
