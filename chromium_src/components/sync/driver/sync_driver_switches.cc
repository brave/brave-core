/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define IsSyncAllowedByFlag IsSyncAllowedByFlag_ChromiumImpl
#include "../../../../../components/sync/driver/sync_driver_switches.cc"  // NOLINT
#undef IsSyncAllowedByFlag

#include "brave/common/brave_switches.h"

namespace switches {

bool IsSyncAllowedByFlag() {
  return IsSyncAllowedByFlag_ChromiumImpl() && IsBraveSyncAllowedByFlag();
}

}  // namespace switches
