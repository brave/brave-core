/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/util/win_util.h"

#include <string>

#include "base/strings/strcat.h"
#include "chrome/updater/updater_scope.h"
#include "chrome/updater/util/util.h"
#include "chrome/updater/win/user_info.h"
#include "chrome/updater/win/win_constants.h"

#include <chrome/updater/util/win_util.cc>

namespace updater {

// Mirrors GetNamedObjectAttributes in omaha/base/utils.cc of our Omaha 3 fork.
// Used by SignalShutdownEvent and IsShutdownEventSignaled via
// rewrite/chrome/updater/util/win_util.cc.yaml.
std::wstring GetLegacyShutdownEventName(UpdaterScope scope) {
  std::wstring user_sid;
  if (!IsSystemInstall(scope)) {
    GetProcessUser(nullptr, nullptr, &user_sid);
  }
  return base::StrCat({L"Global\\BraveSoftware", user_sid, kShutdownEvent});
}

}  // namespace updater
