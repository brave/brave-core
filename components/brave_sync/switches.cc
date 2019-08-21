/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/switches.h"

#include "base/command_line.h"

namespace brave_sync {
namespace switches {

bool IsBraveSyncAllowedByFlag() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(kDisableBraveSync);
}

// Allows disabling Brave Sync.
const char kDisableBraveSync[] = "disable-brave-sync";

}  // namespace switches
}  // namespace brave_sync
