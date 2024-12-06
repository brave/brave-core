/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync/service/sync_prefs.h"

#define SetPasswordSyncAllowed SetPasswordSyncAllowed_ChromiumImpl

#include "src/components/sync/service/sync_prefs.cc"

#undef SetPasswordSyncAllowed

namespace syncer {

void SyncPrefs::SetPasswordSyncAllowed(bool allowed) {}

}  // namespace syncer
