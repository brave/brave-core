/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_PREFS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_PREFS_H_

void SetPasswordSyncAllowed(bool allowed);

#define SetPasswordSyncAllowed                       \
  SetPasswordSyncAllowed_ChromiumImpl(bool allowed); \
  void SetPasswordSyncAllowed

#include "src/components/sync/service/sync_prefs.h"  // IWYU pragma: export

#undef SetPasswordSyncAllowed

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_PREFS_H_
