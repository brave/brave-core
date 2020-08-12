/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/browser/android_util.h"

namespace android_util {

ledger::ClientInfoPtr GetAndroidClientInfo() {
  auto info = ledger::ClientInfo::New();
  info->platform = ledger::Platform::ANDROID_R;
  info->os = ledger::OperatingSystem::UNDEFINED;
  return info;
}

}  // namespace android_util
